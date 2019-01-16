#include <os.h>
#include "bip44.h"
#include "errors.h"
#include "utils.h"
#include <stdbool.h>
#include "endian.h"

size_t pathSpec_parseFromWire(path_spec_t* pathSpec, uint8_t *dataBuffer, size_t dataSize)
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

bool isValidBIP44Prefix(const path_spec_t* pathSpec)
{
	const uint32_t HD = HARDENED_BIP32; // shorthand
#define CHECK(cond) if (!(cond)) return false

	CHECK(pathSpec->length <= ARRAY_LEN(pathSpec->path));
	// Have at least account
	CHECK(pathSpec->length > BIP44_I_ACCOUNT);

	CHECK(pathSpec->path[BIP44_I_PURPOSE] == (BIP_44 | HD));
	CHECK(pathSpec->path[BIP44_I_COIN_TYPE] == (ADA_COIN_TYPE | HD));
	// Account is hardened
	CHECK(isHardened(pathSpec->path[BIP44_I_ACCOUNT]));
	return true;

#undef CHECK
}

// change addresses
static const uint32_t CARDANO_CHAIN_INTERNAL = 1;
// public addresses
static const uint32_t CARDANO_CHAIN_EXTERNAL = 0;

bool isValidBIP44ChainValue(uint32_t value)
{
	return (
	               (value == CARDANO_CHAIN_INTERNAL) ||
	               (value == CARDANO_CHAIN_EXTERNAL)
	       );
}

static const uint32_t MAX_ACCEPTED_ACCOUNT = 10;

bool isAcceptableBIP44AccountValue(uint32_t value)
{
	if (!isHardened(value)) return false;
	uint32_t raw = value & (!HARDENED_BIP32);
	return raw < MAX_ACCEPTED_ACCOUNT;
}
