#include "common.h"
#include "bip44.h"
#include "endian.h"

static const uint32_t MAX_REASONABLE_ACCOUNT = 10;
static const uint32_t CARDANO_CHAIN_INTERNAL = 1;
static const uint32_t CARDANO_CHAIN_EXTERNAL = 0;
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

bool bip44_hasValidCardanoPrefix(const bip44_path_t* pathSpec)
{
#define CHECK(cond) if (!(cond)) return false
	const uint32_t HD = HARDENED_BIP32; // shorthand
	// Have at least account
	CHECK(pathSpec->length > BIP44_I_COIN_TYPE);

	CHECK(pathSpec->path[BIP44_I_PURPOSE] == (BIP_44 | HD));
	CHECK(pathSpec->path[BIP44_I_COIN_TYPE] == (ADA_COIN_TYPE | HD));
	return true;
#undef CHECK
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

bool bip44_hasValidChainType(const bip44_path_t* pathSpec)
{
	if (!bip44_containsChainType(pathSpec)) return false;
	uint32_t chainType = bip44_getChainTypeValue(pathSpec);

	return (chainType == CARDANO_CHAIN_INTERNAL) || (chainType == CARDANO_CHAIN_EXTERNAL);
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
	uint32_t address = bip44_getAddressValue(pathSpec);
	return (address <= MAX_REASONABLE_ADDRESS);
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
		uint32_t value = pathSpec->path[i];

		if ((value & HARDENED_BIP32) == HARDENED_BIP32) {
			WRITE("/%d'", (int) (value & ~HARDENED_BIP32));
		} else {
			WRITE("/%d", (int) value);
		}
	}

	ASSERT(ptr < end);
}
