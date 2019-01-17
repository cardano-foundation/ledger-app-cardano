#include <os.h>
#include "bip44.h"
#include "errors.h"
#include "utils.h"
#include <stdbool.h>
#include "endian.h"
#include <string.h>

size_t bip44_parseFromWire(
        bip44_path_t* pathSpec,
        uint8_t *dataBuffer, size_t dataSize
)
{
#define VALIDATE(cond) if (!(cond)) THROW(ERR_INVALID_DATA)

	// Ensure we have length
	VALIDATE(dataSize >= 1);

	// Cast length to size_t
	size_t length = dataBuffer[0];

	// Ensure length is valid
	VALIDATE(length <= ARRAY_LEN(pathSpec->path));
	VALIDATE(length * 4 + 1 <= dataSize);

	pathSpec->length = length;

	size_t offset = 1;
	for (size_t i = 0; i < length; i++) {
		pathSpec->path[i] = u4be_read(dataBuffer + offset);
		offset += 4;
	}
	return offset;
#undef VALIDATE
}

bool isHardened(uint32_t value)
{
	return value == (value | HARDENED_BIP32);
}

bool bip44_hasValidPrefix(const bip44_path_t* pathSpec)
{
	const uint32_t HD = HARDENED_BIP32; // shorthand
#define CHECK(cond) if (!(cond)) return false
	// Have at least account
	CHECK(pathSpec->length > BIP44_I_ACCOUNT);

	CHECK(pathSpec->path[BIP44_I_PURPOSE] == (BIP_44 | HD));
	CHECK(pathSpec->path[BIP44_I_COIN_TYPE] == (ADA_COIN_TYPE | HD));
	// Account is hardened
	CHECK(isHardened(pathSpec->path[BIP44_I_ACCOUNT]));
	return true;

#undef CHECK
}


// Account
static const uint32_t MAX_ACCEPTED_ACCOUNT = 10;

bool bip44_containsAccount(const bip44_path_t* pathSpec)
{
	return pathSpec->length > BIP44_I_ACCOUNT;
}

uint32_t bip44_getAccount(const bip44_path_t* pathSpec)
{
	ASSERT(pathSpec->length > BIP44_I_ACCOUNT);
	return pathSpec->path[BIP44_I_ACCOUNT];
}

bool bip44_hasValidAccount(const bip44_path_t* pathSpec)
{
	if (!bip44_containsAccount(pathSpec)) return false;
	uint32_t account = bip44_getAccount(pathSpec);
	if (!isHardened(account)) return false;
	// Un-harden
	account = account & (!HARDENED_BIP32);
	return account < MAX_ACCEPTED_ACCOUNT;
}

// ChainType
static const uint32_t CARDANO_CHAIN_INTERNAL = 1;
static const uint32_t CARDANO_CHAIN_EXTERNAL = 0;

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


// TODO(ppershing): this function needs to be thoroughly tested
// on small outputSize
void bip44_format(const bip44_path_t* pathSpec, char* out, size_t outSize)
{
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);
	// We have to have space for terminating null
	ASSERT(outSize > 0);
	char* ptr = out;
	char* end = (out + outSize);

#define WRITE(fmt, ...) \
	{ \
		ASSERT(ptr <= end); \
		size_t availableSize = end - ptr; \
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
