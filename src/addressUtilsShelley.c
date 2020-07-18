#include "bufView.h"
#include "hash.h"
#include "keyDerivation.h"
#include "addressUtilsByron.h"
#include "addressUtilsShelley.h"
#include "bip44.h"
#include "base58.h"
#include "bech32.h"

address_type_t getAddressType(uint8_t addressHeader)
{
	const uint8_t ADDRESS_TYPE_MASK = 0b11110000;
	return (addressHeader & ADDRESS_TYPE_MASK) >> 4;
}

bool isSupportedAddressType(uint8_t addressType)
{
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

uint8_t constructShelleyAddressHeader(address_type_t type, uint8_t networkId)
{
	ASSERT(isSupportedAddressType(type));
	ASSERT(isValidNetworkId(networkId));

	return (type << 4) | networkId;
}

uint8_t getNetworkId(uint8_t addressHeader)
{
	ASSERT(isSupportedAddressType(getAddressType(addressHeader)));

	const uint8_t NETWORK_ID_MASK = 0b00001111;
	return addressHeader & NETWORK_ID_MASK;
}

bool isValidNetworkId(uint8_t networkId)
{
	return networkId <= 0b1111;
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

bool isStakingInfoConsistentWithAddressType(const addressParams_t* addressParams)
{
#define INCONSISTENT_WITH(STAKING_CHOICE) if (addressParams->stakingChoice == (STAKING_CHOICE)) return false
#define CONSISTENT_ONLY_WITH(STAKING_CHOICE) if (addressParams->stakingChoice != (STAKING_CHOICE)) return false

	switch (addressParams->type) {

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

// TODO perhaps move elsewhere? bip44?
size_t view_appendPublicKeyHash(write_view_t* view, const bip44_path_t* keyDerivationPath)
{
	extendedPublicKey_t extPubKey;
	deriveExtendedPublicKey(keyDerivationPath, &extPubKey);

	uint8_t hashedPubKey[ADDRESS_KEY_HASH_LENGTH];
	blake2b_224_hash(
	        extPubKey.pubKey, SIZEOF(extPubKey.pubKey),
	        hashedPubKey, SIZEOF(hashedPubKey)
	);

	view_appendData(view, hashedPubKey, SIZEOF(hashedPubKey));

	return ADDRESS_KEY_HASH_LENGTH;
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

	const int ADDRESS_LENGTH = 1 + 2 * ADDRESS_KEY_HASH_LENGTH;
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
		ASSERT(stakingKeyHashSize == ADDRESS_KEY_HASH_LENGTH);
		view_appendData(&out, stakingKeyHash, ADDRESS_KEY_HASH_LENGTH);
	}

	const int ADDRESS_LENGTH = 1 + 2 * ADDRESS_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
}

static size_t deriveAddress_base(const addressParams_t* addressParams, uint8_t* outBuffer, size_t outSize)
{
	ASSERT(addressParams->type == BASE);

	const uint8_t header = constructShelleyAddressHeader(
	                               addressParams->type, addressParams->networkId
	                       );

	switch (addressParams->stakingChoice) {
	case STAKING_KEY_PATH:
		return deriveAddress_base_stakingKeyPath(
		               header,
		               &addressParams->spendingKeyPath,
		               &addressParams->stakingKeyPath,
		               outBuffer, outSize
		       );

	case STAKING_KEY_HASH:
		return deriveAddress_base_stakingKeyHash(
		               header,
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

	const int ADDRESS_LENGTH = 1 + ADDRESS_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
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
		ASSERT(bip44_isValidStakingKeyPath(spendingKeyPath)); // TODO check for unusual account?
		view_appendPublicKeyHash(&out, spendingKeyPath);
	}
	{
		// no staking data
	}

	const int ADDRESS_LENGTH = 1 + ADDRESS_KEY_HASH_LENGTH;
	ASSERT(view_processedSize(&out) == ADDRESS_LENGTH);

	return ADDRESS_LENGTH;
}

size_t deriveAddress(const addressParams_t* addressParams, uint8_t* outBuffer, size_t outSize)
{
	const bip44_path_t* spendingPath = &addressParams->spendingKeyPath;

	if (addressParams->type == BYRON) {
		return deriveAddress_byron(spendingPath, addressParams->protocolMagic, outBuffer, outSize);
	}

	// shelley
	const uint8_t header = constructShelleyAddressHeader(addressParams->type, addressParams->networkId);

	switch (addressParams->type) {
	case BASE:
		return deriveAddress_base(addressParams, outBuffer, outSize);
	case POINTER:
		ASSERT(addressParams->stakingChoice == BLOCKCHAIN_POINTER);
		return deriveAddress_pointer(header, spendingPath, &addressParams->stakingKeyBlockchainPointer, outBuffer, outSize);
	case ENTERPRISE:
		return deriveAddress_enterprise(header, spendingPath, outBuffer, outSize);
	case REWARD:
		return deriveAddress_reward(header, spendingPath, outBuffer, outSize);
	case BYRON:
		ASSERT(false);
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

	STATIC_ASSERT(sizeof(int) >= sizeof(blockchainIndex_t), "bad blockchainIndex_t size");
	STATIC_ASSERT(sizeof(int) == 4, "bad int size"); // because of the checks below
	ASSERT(blockchainPointer.blockIndex <= INT32_MAX);
	ASSERT(blockchainPointer.txIndex <= INT32_MAX);
	ASSERT(blockchainPointer.certificateIndex <= INT32_MAX);
	WRITE(
	        "(%d, %d, %d)",
	        (int) blockchainPointer.blockIndex,
	        (int) blockchainPointer.txIndex,
	        (int) blockchainPointer.certificateIndex
	);

	ASSERT(ptr < end);
}

size_t humanReadableAddress(const uint8_t* address, size_t addressSize, char* out, size_t outSize)
{
	ASSERT(addressSize > 0);
	const uint8_t addressType = getAddressType(address[0]);
	ASSERT(isSupportedAddressType(addressType));

	switch (addressType) {
	case BYRON:
		return base58_encode(address, addressSize, out, outSize);

	default: // shelley addresses
		return bech32_encode("addr", address, addressSize, out, outSize);
	}
}

/*
 * Apart from parsing, we validate that the input contains nothing more than the params.
 *
 * The serialization format:
 *
 * address type 1B
 * if address type == BYRON
 *     protocol magic 4B
 * else
 *     network id 1B
 * spending public key derivation path (1B for length + [0-10] x 4B)
 * staking choice 1B
 *     if NO_STAKING:
 *         nothing more
 *     if STAKING_KEY_PATH:
 *         staking public key derivation path (1B for length + [0-10] x 4B)
 *     if STAKING_KEY_HASH:
 *         staking key hash 28B
 *     if BLOCKCHAIN_POINTER:
 *         certificate blockchain pointer 3 x 4B
 *
 * (see also enums in addressUtilsShelley.h)
 */
void parseAddressParams(const uint8_t *wireDataBuffer, size_t wireDataSize, addressParams_t* params)
{
	read_view_t view = make_read_view(wireDataBuffer, wireDataBuffer + wireDataSize);

	// address type
	VALIDATE(view_remainingSize(&view) >= 1, ERR_INVALID_DATA);
	params->type = parse_u1be(&view);
	TRACE("Address type: 0x%x", params->type);
	VALIDATE(isSupportedAddressType(params->type), ERR_UNSUPPORTED_ADDRESS_TYPE);

	// protocol magic / network id
	if (params->type == BYRON) {
		VALIDATE(view_remainingSize(&view) >= 4, ERR_INVALID_DATA);
		params->protocolMagic = parse_u4be(&view);
		TRACE("Protocol magic: 0x%x", params->protocolMagic);
		// TODO is there something to validate?
	} else {
		VALIDATE(view_remainingSize(&view) >= 1, ERR_INVALID_DATA);
		params->networkId = parse_u1be(&view);
		TRACE("Network id: 0x%x", params->networkId);
		VALIDATE(params->networkId <= 0b1111, ERR_INVALID_DATA);
	}

	// spending public key derivation path
	view_skipBytes(&view, bip44_parseFromWire(&params->spendingKeyPath, VIEW_REMAINING_TO_TUPLE_BUF_SIZE(&view)));
	bip44_PRINTF(&params->spendingKeyPath);

	// staking choice
	VALIDATE(view_remainingSize(&view) >= 1, ERR_INVALID_DATA);
	params->stakingChoice = parse_u1be(&view);
	TRACE("Staking choice: 0x%x", params->stakingChoice);
	VALIDATE(isValidStakingChoice(params->stakingChoice), ERR_INVALID_DATA);

	// staking choice determines what to parse next
	switch (params->stakingChoice) {

	case NO_STAKING:
		break;

	case STAKING_KEY_PATH:
		view_skipBytes(&view, bip44_parseFromWire(&params->stakingKeyPath, VIEW_REMAINING_TO_TUPLE_BUF_SIZE(&view)));
		bip44_PRINTF(&params->stakingKeyPath);
		break;

	case STAKING_KEY_HASH:
		VALIDATE(view_remainingSize(&view) == ADDRESS_KEY_HASH_LENGTH, ERR_INVALID_DATA);
		ASSERT(SIZEOF(params->stakingKeyHash) == ADDRESS_KEY_HASH_LENGTH);
		os_memmove(params->stakingKeyHash, view.ptr, ADDRESS_KEY_HASH_LENGTH);
		view_skipBytes(&view, ADDRESS_KEY_HASH_LENGTH);
		TRACE("Staking key hash: ");
		TRACE_BUFFER(params->stakingKeyHash, SIZEOF(params->stakingKeyHash));
		break;

	case BLOCKCHAIN_POINTER:
		VALIDATE(view_remainingSize(&view) == 12, ERR_INVALID_DATA);
		params->stakingKeyBlockchainPointer.blockIndex = parse_u4be(&view);
		params->stakingKeyBlockchainPointer.txIndex = parse_u4be(&view);
		params->stakingKeyBlockchainPointer.certificateIndex = parse_u4be(&view);
		TRACE("Staking pointer: [%d, %d, %d]\n", params->stakingKeyBlockchainPointer.blockIndex,
		      params->stakingKeyBlockchainPointer.txIndex, params->stakingKeyBlockchainPointer.certificateIndex);
		break;

	default:
		ASSERT(false);
	}

	VALIDATE(view_remainingSize(&view) == 0, ERR_INVALID_DATA);
}
