#include "common.h"
#include "bip44.h"
#include "endian.h"

static const uint32_t CARDANO_CHAIN_EXTERNAL = 0;
static const uint32_t CARDANO_CHAIN_INTERNAL = 1;
static const uint32_t CARDANO_CHAIN_STAKING_KEY = 2;

static const uint32_t MAX_REASONABLE_ACCOUNT = 20;
static const uint32_t MAX_REASONABLE_ADDRESS = 1000000;

size_t bip44_parseFromWire(
        bip44_path_t* pathSpec,
        const uint8_t *dataBuffer, size_t dataSize
)
{
	// Ensure we have length
	VALIDATE(dataSize >= 1, ERR_INVALID_DATA);

	// Cast length to size_t
	size_t length = dataBuffer[0];

	// Ensure length is valid
	VALIDATE(length <= ARRAY_LEN(pathSpec->path), ERR_INVALID_DATA);
	VALIDATE(length * 4 + 1 <= dataSize, ERR_INVALID_DATA);

	pathSpec->length = length;

	size_t offset = 1;
	for (size_t i = 0; i < length; i++) {
		pathSpec->path[i] = u4be_read(dataBuffer + offset);
		offset += 4;
	}
	return offset;
}

bool isHardened(uint32_t value)
{
	return value == (value | HARDENED_BIP32);
}


// Byron: /44'/1815'
bool bip44_hasByronPrefix(const bip44_path_t* pathSpec)
{
#define CHECK(cond) if (!(cond)) return false
	CHECK(pathSpec->length > BIP44_I_COIN_TYPE);
	CHECK(pathSpec->path[BIP44_I_PURPOSE] == (PURPOSE_BYRON | HARDENED_BIP32));
	CHECK(pathSpec->path[BIP44_I_COIN_TYPE] == (ADA_COIN_TYPE | HARDENED_BIP32));
	return true;
#undef CHECK
}

// Shelley: /1852'/1815'
bool bip44_hasShelleyPrefix(const bip44_path_t* pathSpec)
{
#define CHECK(cond) if (!(cond)) return false
	CHECK(pathSpec->length > BIP44_I_COIN_TYPE);
	CHECK(pathSpec->path[BIP44_I_PURPOSE] == (PURPOSE_SHELLEY | HARDENED_BIP32));
	CHECK(pathSpec->path[BIP44_I_COIN_TYPE] == (ADA_COIN_TYPE | HARDENED_BIP32));
	return true;
#undef CHECK
}

bool bip44_hasValidCardanoPrefix(const bip44_path_t* pathSpec)
{
	return bip44_hasByronPrefix(pathSpec) || bip44_hasShelleyPrefix(pathSpec);
}

// Account

bool bip44_containsAccount(const bip44_path_t* pathSpec)
{
	return pathSpec->length > BIP44_I_ACCOUNT;
}

uint32_t bip44_getAccount(const bip44_path_t* pathSpec)
{
	ASSERT(pathSpec->length > BIP44_I_ACCOUNT);
	return pathSpec->path[BIP44_I_ACCOUNT];
}

bool bip44_hasReasonableAccount(const bip44_path_t* pathSpec)
{
	if (!bip44_containsAccount(pathSpec)) return false;
	uint32_t account = bip44_getAccount(pathSpec);
	if (!isHardened(account)) return false;
	// Un-harden
	account = account & (~HARDENED_BIP32);
	return account < MAX_REASONABLE_ACCOUNT;
}

// ChainType
bool bip44_containsChainType(const bip44_path_t* pathSpec)
{
	return pathSpec->length > BIP44_I_CHAIN;
}

uint32_t bip44_getChainTypeValue(const bip44_path_t* pathSpec)
{
	ASSERT(pathSpec->length > BIP44_I_CHAIN);
	return pathSpec->path[BIP44_I_CHAIN];
}

bool bip44_hasValidChainTypeForAddress(const bip44_path_t* pathSpec)
{
	if (!bip44_containsChainType(pathSpec)) return false;
	const uint32_t chainType = bip44_getChainTypeValue(pathSpec);

	return (chainType == CARDANO_CHAIN_EXTERNAL) || (chainType == CARDANO_CHAIN_INTERNAL);
}

// Address

bool bip44_containsAddress(const bip44_path_t* pathSpec)
{
	return pathSpec->length > BIP44_I_ADDRESS;
}

uint32_t bip44_getAddressValue(const bip44_path_t* pathSpec)
{
	ASSERT(pathSpec->length > BIP44_I_ADDRESS);
	return pathSpec->path[BIP44_I_ADDRESS];
}

bool bip44_hasReasonableAddress(const bip44_path_t* pathSpec)
{
	if (!bip44_containsAddress(pathSpec)) return false;
	const uint32_t address = bip44_getAddressValue(pathSpec);
	return (address <= MAX_REASONABLE_ADDRESS);
}

// Staking keys (one per account, should end with /2/0 after account)

bool bip44_isValidStakingKeyPath(const bip44_path_t* pathSpec)
{
	if (!bip44_containsAddress(pathSpec)) return false;
	if (bip44_containsMoreThanAddress(pathSpec)) return false;
	if (!bip44_hasShelleyPrefix(pathSpec)) return false;

	const uint32_t chainType = bip44_getChainTypeValue(pathSpec);
	if (chainType != CARDANO_CHAIN_STAKING_KEY) return false;

	return (bip44_getAddressValue(pathSpec) == 0);
}

void bip44_stakingKeyPathFromAddresPath(const bip44_path_t* addressPath, bip44_path_t* stakingKeyPath)
{
	ASSERT(bip44_hasShelleyPrefix(addressPath));
	ASSERT(bip44_containsAddress(addressPath) && !bip44_containsMoreThanAddress(addressPath));

	stakingKeyPath->length = 5;
	stakingKeyPath->path[BIP44_I_PURPOSE] = addressPath->path[BIP44_I_PURPOSE];
	stakingKeyPath->path[BIP44_I_COIN_TYPE] = addressPath->path[BIP44_I_COIN_TYPE];
	stakingKeyPath->path[BIP44_I_ACCOUNT] = addressPath->path[BIP44_I_ACCOUNT];
	stakingKeyPath->path[BIP44_I_CHAIN] = CARDANO_CHAIN_STAKING_KEY;
	stakingKeyPath->path[BIP44_I_ADDRESS] = 0;

	ASSERT(bip44_isValidStakingKeyPath(stakingKeyPath));
}

// Futher
bool bip44_containsMoreThanAddress(const bip44_path_t* pathSpec)
{
	return (pathSpec->length > BIP44_I_REST);
}


// TODO(ppershing): this function needs to be thoroughly tested
// on small outputSize
void bip44_printToStr(const bip44_path_t* pathSpec, char* out, size_t outSize)
{
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);
	// We have to have space for terminating null
	ASSERT(outSize > 0);
	char* ptr = out;
	char* end = (out + outSize);

#define WRITE(fmt, ...) \
	{ \
		ASSERT(ptr <= end); \
		STATIC_ASSERT(sizeof(end - ptr) == sizeof(size_t), "bad size_t size"); \
		size_t availableSize = (size_t) (end - ptr); \
		/* Note(ppershing): We do not bother checking return */ \
		/* value of snprintf as it always returns 0. */ \
		/* Go figure out ... */ \
		snprintf(ptr, availableSize, fmt, ##__VA_ARGS__); \
		size_t res = strlen(ptr); \
		/* TODO(better error handling) */ \
		ASSERT(res + 1 < availableSize); \
		ptr += res; \
	}

	WRITE("m");

	ASSERT(pathSpec->length < ARRAY_LEN(pathSpec->path));

	for (size_t i = 0; i < pathSpec->length; i++) {
		const uint32_t value = pathSpec->path[i];

		if ((value & HARDENED_BIP32) == HARDENED_BIP32) {
			WRITE("/%d'", (int) (value & ~HARDENED_BIP32));
		} else {
			WRITE("/%d", (int) value);
		}
	}

	ASSERT(ptr < end);
}

#ifdef DEVEL
void bip44_PRINTF(const bip44_path_t* pathSpec)
{
	char tmp[100];
	SIZEOF(*pathSpec);
	bip44_printToStr(pathSpec, tmp, SIZEOF(tmp));
	PRINTF("%s", tmp);
};
#endif
