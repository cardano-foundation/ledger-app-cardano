#include "bufView.h"
#include "hash.h"
#include "keyDerivation.h"
#include "addressUtilsByron.h"
#include "addressUtilsShelley.h"
#include "bip44.h"

address_type_t getAddressType(uint8_t addressHeader)
{
	const uint8_t ADDRESS_TYPE_MASK = 0b11110000;
	return (addressHeader & ADDRESS_TYPE_MASK) >> 4;
}

bool isSupportedAddressType(uint8_t addressHeader)
{
	const address_type_t addressType = getAddressType(addressHeader);
	switch (addressType) {
	case BASE:
	case POINTER:
	case ENTERPRISE:
	case BYRON:
	case REWARD:
		return true;
	default:
		return false;
	}
}

uint8_t getNetworkId(uint8_t addressHeader)
{
	const uint8_t NETWORK_ID_MASK = 0b00001111;
	return addressHeader & NETWORK_ID_MASK;
}

bool isValidStakingChoice(uint8_t stakingChoice)
{
	switch (stakingChoice) {
	case NO_STAKING:
	case STAKING_KEY_PATH:
	case STAKING_KEY_HASH:
	case BLOCKCHAIN_POINTER:
		return true;
	default:
		return false;
	}
}

bool isStakingInfoConsistentWithHeader(const shelleyAddressParams_t* addressParams)
{
#define INCONSISTENT_WITH(STAKING_CHOICE) if (addressParams->stakingChoice == (STAKING_CHOICE)) return false
#define CONSISTENT_ONLY_WITH(STAKING_CHOICE) if (addressParams->stakingChoice != (STAKING_CHOICE)) return false

	const address_type_t addressType = getAddressType(addressParams->header);
	switch (addressType) {

	case BASE:
		INCONSISTENT_WITH(NO_STAKING);
		INCONSISTENT_WITH(BLOCKCHAIN_POINTER);
		break;

	case POINTER:
		CONSISTENT_ONLY_WITH(BLOCKCHAIN_POINTER);
		break;

	case ENTERPRISE:
	case BYRON:
	case REWARD:
		CONSISTENT_ONLY_WITH(NO_STAKING);
		break;

	default:
		ASSERT(false);
	}

	return true;

#undef INCONSISTENT_WITH
#undef CONSISTENT_ONLY_WITH
}

static size_t view_appendPublicKeyHash(write_view_t* view, const bip44_path_t* spendingKeyPath)
{
	extendedPublicKey_t extPubKey;
	deriveExtendedPublicKey(spendingKeyPath, &extPubKey);

	uint8_t hashedPubKey[PUBLIC_KEY_HASH_LENGTH];
	blake2b_224_hash(
	        extPubKey.pubKey, SIZEOF(extPubKey.pubKey),
	        hashedPubKey, SIZEOF(hashedPubKey)
	);

	view_appendData(view, hashedPubKey, SIZEOF(hashedPubKey));

	return PUBLIC_KEY_HASH_LENGTH;
}

static size_t deriveAddress_base_stakingKeyPath(
        uint8_t addressHeader, const bip44_path_t* spendingKeyPath,
        const bip44_path_t* stakingKeyPath,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(getAddressType(addressHeader) == BASE);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);
	{
		view_appendData(&out, &addressHeader, 1);
	}
	{
		view_appendPublicKeyHash(&out, spendingKeyPath);
	}
	{
		ASSERT(bip44_isValidStakingKeyPath(stakingKeyPath)); // TODO should we allow paths not corresponding to standard /2/0 staking keys?
		view_appendPublicKeyHash(&out, stakingKeyPath);
	}

	const int ADDRESS_LENGTH = 1 + 2 * PUBLIC_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
}

static size_t deriveAddress_base_stakingKeyHash(
        uint8_t addressHeader, const bip44_path_t* spendingKeyPath,
        const uint8_t* stakingKeyHash, size_t stakingKeyHashSize,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(getAddressType(addressHeader) == BASE);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);
	{
		view_appendData(&out, &addressHeader, 1);
	}
	{
		view_appendPublicKeyHash(&out, spendingKeyPath);
	}
	{
		ASSERT(stakingKeyHashSize == PUBLIC_KEY_HASH_LENGTH);
		view_appendData(&out, stakingKeyHash, PUBLIC_KEY_HASH_LENGTH);
	}

	const int ADDRESS_LENGTH = 1 + 2 * PUBLIC_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
}

static size_t deriveAddress_base(const shelleyAddressParams_t* addressParams, uint8_t* outBuffer, size_t outSize)
{
	ASSERT(getAddressType(addressParams->header) == BASE);

	switch (addressParams->stakingChoice) {
	case STAKING_KEY_PATH:
		return deriveAddress_base_stakingKeyPath(
		               addressParams->header,
		               &addressParams->spendingKeyPath,
		               &addressParams->stakingKeyPath,
		               outBuffer, outSize
		       );

	case STAKING_KEY_HASH:
		return deriveAddress_base_stakingKeyHash(
		               addressParams->header,
		               &addressParams->spendingKeyPath,
		               addressParams->stakingKeyHash,
		               SIZEOF(addressParams->stakingKeyHash),
		               outBuffer, outSize
		       );

	default:
		ASSERT(false);
	}
	return BUFFER_SIZE_PARANOIA + 1;
}

static size_t view_appendVariableLengthUInt(write_view_t* view, uint64_t value)
{
	ASSERT((value & (1llu << 63)) == 0); // avoid accidental cast from negative signed value

	if (value == 0) {
		uint8_t byte = 0;
		view_appendData(view, &byte, 1);
		return 1;
	}

	ASSERT(value > 0);

	uint8_t chunks[10]; // 7-bit chunks of the input bits, at most 10 in uint64
	size_t outputSize = 0;
	{
		blockchainIndex_t bits = value;
		while (bits > 0) {
			// take next 7 bits from the right
			chunks[outputSize++] = bits & 0b01111111;
			bits >>= 7;
		}
	}
	ASSERT(outputSize > 0);
	for (size_t i = outputSize - 1; i > 0; --i) {
		// highest bit set to 1 since more bytes follow
		uint8_t nextByte = chunks[i] | 0b10000000;
		view_appendData(view, &nextByte, 1);
	}
	// write the remaining byte, highest bit 0
	view_appendData(view, &chunks[0], 1);

	return outputSize;
}

static size_t deriveAddress_pointer(
        uint8_t addressHeader, const bip44_path_t* spendingKeyPath,
        const blockchainPointer_t* stakingKeyBlockchainPointer,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(getAddressType(addressHeader) == POINTER);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);
	{
		view_appendData(&out, &addressHeader, 1);
	}
	{
		view_appendPublicKeyHash(&out, spendingKeyPath);
	}
	{
		view_appendVariableLengthUInt(&out, stakingKeyBlockchainPointer->blockIndex);
		view_appendVariableLengthUInt(&out, stakingKeyBlockchainPointer->txIndex);
		view_appendVariableLengthUInt(&out, stakingKeyBlockchainPointer->certificateIndex);
	}

	return view_processedSize(&out);
}

static size_t deriveAddress_enterprise(
        uint8_t addressHeader, const bip44_path_t* spendingKeyPath,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(getAddressType(addressHeader) == ENTERPRISE);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);
	{
		view_appendData(&out, &addressHeader, 1);
	}
	{
		view_appendPublicKeyHash(&out, spendingKeyPath);
	}
	{
		// no staking data
	}

	const int ADDRESS_LENGTH = 1 + PUBLIC_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
}

static size_t deriveAddress_byron(
        uint8_t addressHeader, const bip44_path_t* spendingKeyPath,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(getAddressType(addressHeader) == BYRON);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	// the old Byron version
	// TODO network_id is ignored, should be?
	return deriveAddress(spendingKeyPath, outBuffer, outSize);
}

static size_t deriveAddress_reward(
        uint8_t addressHeader, const bip44_path_t* spendingKeyPath,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(getAddressType(addressHeader) == REWARD);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);
	{
		view_appendData(&out, &addressHeader, 1);
	}
	{
		// staking key path expected (corresponds to reward account)
		ASSERT(bip44_isValidStakingKeyPath(spendingKeyPath));
		view_appendPublicKeyHash(&out, spendingKeyPath);
	}
	{
		// no staking data
	}

	const int ADDRESS_LENGTH = 1 + PUBLIC_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
}

size_t deriveAddress_shelley(const shelleyAddressParams_t* addressParams, uint8_t* outBuffer, size_t outSize)
{
	const address_type_t addressType = getAddressType(addressParams->header);
	const uint8_t header = addressParams->header;
	const bip44_path_t* spendingPath = &addressParams->spendingKeyPath;

	switch (addressType) {
	case BASE:
		return deriveAddress_base(addressParams, outBuffer, outSize);
	case POINTER:
		ASSERT(addressParams->stakingChoice == BLOCKCHAIN_POINTER);
		return deriveAddress_pointer(header, spendingPath, &addressParams->stakingKeyBlockchainPointer, outBuffer, outSize);
	case ENTERPRISE:
		return deriveAddress_enterprise(header, spendingPath, outBuffer, outSize);
	case BYRON:
		return deriveAddress_byron(header, spendingPath, outBuffer, outSize);
	case REWARD:
		return deriveAddress_reward(header, spendingPath, outBuffer, outSize);
	default:
		THROW(ERR_UNSUPPORTED_ADDRESS_TYPE);
	}
	return BUFFER_SIZE_PARANOIA + 1;
}

// mostly copied from bip44_printToStr
// TODO(ppershing): this function needs to be thoroughly tested
// on small outputSize
void printBlockchainPointerToStr(blockchainPointer_t blockchainPointer, char* out, size_t outSize)
{
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);
	// We have to have space for terminating null
	ASSERT(outSize > 0);
	char* ptr = out;
	char* end = (out + outSize);

// TODO unify with the same macro used in bip44.c ?
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

	STATIC_ASSERT(sizeof(unsigned) >= sizeof(blockchainIndex_t), "bad blockchainIndex_t size");
	WRITE(
	        "(%u, %u, %u)",
	        (unsigned) blockchainPointer.blockIndex,
	        (unsigned) blockchainPointer.txIndex,
	        (unsigned) blockchainPointer.certificateIndex
	);

	ASSERT(ptr < end);
}
