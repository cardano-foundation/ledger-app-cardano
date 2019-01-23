#include "common.h"
#include "addressUtils.h"
#include "keyDerivation.h"
#include "cbor.h"
#include "cardano.h"
#include "hash.h"
#include "crc32.h"

void addressRootFromExtPubKey(
        const extendedPublicKey_t* extPubKey,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(SIZEOF(*extPubKey) == EXTENDED_PUBKEY_SIZE);
	ASSERT(outSize == 28);

	uint8_t cborBuffer[64 + 10];
	uint8_t* ptr = cborBuffer;
	uint8_t* end = END(cborBuffer);


	// [0, [0, publicKey:chainCode], Map(0)]
	// TODO(ppershing): what are the first two 0 constants?
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_ARRAY, 3);
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_UNSIGNED, CARDANO_ADDRESS_TYPE_PUBKEY);

	// enter inner array
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_ARRAY, 2);
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_UNSIGNED, 0 /* this seems to be hardcoded to 0*/);
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_BYTES, EXTENDED_PUBKEY_SIZE);
	BUF_PTR_APPEND_DATA(ptr, end, extPubKey, EXTENDED_PUBKEY_SIZE);
	// exit inner array

	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_MAP, 0 /* addrAttributes is empty */);

	// cborBuffer is hashed twice. First by sha3_256 and then by blake2b_224
	uint8_t cborShaHash[32];
	sha3_256_hash(
	        cborBuffer, ptr - cborBuffer,
	        cborShaHash, SIZEOF(cborShaHash)
	);
	blake2b_224_hash(
	        cborShaHash, SIZEOF(cborShaHash),
	        outBuffer, outSize
	);
}


size_t cborEncodePubkeyAddressInner(
        const uint8_t* addressRoot, size_t addressRootSize,
        uint8_t* outBuffer, size_t outSize
        /* potential attributes */
)
{
	ASSERT(addressRootSize == 28); // should be result of blake2b_224
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	uint8_t* ptr = outBuffer;
	uint8_t* end = outBuffer + outSize;


	// 2nd level
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_ARRAY, 3);
	{
		// 1
		BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_BYTES, addressRootSize);
		BUF_PTR_APPEND_DATA(ptr, end, addressRoot, addressRootSize);
	} {
		// 2
		BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_MAP, 0 /* addrAttributes is empty */);
	} {
		// 3
		BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_UNSIGNED, CARDANO_ADDRESS_TYPE_PUBKEY);
	}
	ASSERT(ptr <= end);
	return ptr - outBuffer;
}


size_t cborPackRawAddressWithChecksum(
        const uint8_t* rawAddressBuffer, size_t rawAddressSize,
        uint8_t* outputBuffer, size_t outputSize
)
{
	ASSERT(rawAddressSize < BUFFER_SIZE_PARANOIA);
	ASSERT(outputSize < BUFFER_SIZE_PARANOIA);

	uint8_t* ptr = outputBuffer;
	uint8_t* end = outputBuffer + outputSize;

	// Format is
	// Array[
	//     tag(24):bytes(rawAddress),
	//     crc32(rawAddress)
	// ]
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_ARRAY, 2);
	{
		BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);
		BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_BYTES, rawAddressSize);
		BUF_PTR_APPEND_DATA(ptr, end, rawAddressBuffer, rawAddressSize);
	} {
		uint32_t checksum = crc32(rawAddressBuffer, rawAddressSize);
		BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_UNSIGNED, checksum);
	}
	ASSERT(ptr <= end);
	return ptr - outputBuffer;
}


size_t cborEncodePubkeyAddress(
        const uint8_t* addressRoot, size_t addressRootSize,
        uint8_t* outBuffer, size_t outSize
        /* potential attributes */
)
{
	ASSERT(addressRootSize == 28); // should be result of blake2b_224
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	uint8_t rawAddressBuffer[40];
	size_t rawAddressSize = cborEncodePubkeyAddressInner(
	                                addressRoot, addressRootSize,
	                                rawAddressBuffer, SIZEOF(rawAddressBuffer)
	                        );

	return cborPackRawAddressWithChecksum(
	               rawAddressBuffer, rawAddressSize,
	               outBuffer, outSize
	       );
}


uint32_t deriveAddress(
        const bip44_path_t* pathSpec,
        uint8_t* outBuffer, size_t outSize
)
{
	uint8_t addressRoot[28];
	{
		extendedPublicKey_t extPubKey;

		deriveExtendedPublicKey(pathSpec, &extPubKey);

		addressRootFromExtPubKey(
		        &extPubKey,
		        addressRoot, SIZEOF(addressRoot)
		);
	}

	size_t addressSize = cborEncodePubkeyAddress(
	                             addressRoot, SIZEOF(addressRoot),
	                             outBuffer, outSize);
	return addressSize;
}
