#include "common.h"
#include "addressUtils.h"
#include "keyDerivation.h"
#include "cbor.h"
#include "cardano.h"
#include "hash.h"
#include "crc32.h"
#include "bufView.h"

void addressRootFromExtPubKey(
        const extendedPublicKey_t* extPubKey,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(SIZEOF(*extPubKey) == EXTENDED_PUBKEY_SIZE);
	ASSERT(outSize == 28);

	uint8_t cborBuffer[64 + 10];
	write_view_t cbor = make_write_view(cborBuffer, END(cborBuffer));

	{
		// [0, [0, publicKey:chainCode], Map(0)]
		// TODO(ppershing): what are the first two 0 constants?
		view_appendToken(&cbor, CBOR_TYPE_ARRAY, 3);
		{
			view_appendToken(&cbor, CBOR_TYPE_UNSIGNED, CARDANO_ADDRESS_TYPE_PUBKEY);
		}
		{
			view_appendToken(&cbor, CBOR_TYPE_ARRAY, 2);
			{
				view_appendToken(&cbor, CBOR_TYPE_UNSIGNED, 0 /* this seems to be hardcoded to 0*/);
			}
			{
				view_appendToken(&cbor, CBOR_TYPE_BYTES, EXTENDED_PUBKEY_SIZE);
				view_appendData(&cbor, (const uint8_t*) extPubKey, EXTENDED_PUBKEY_SIZE);
			}
		}
		{
			view_appendToken(&cbor, CBOR_TYPE_MAP, 0 /* addrAttributes is empty */);
		}
	}

	// cborBuffer is hashed twice. First by sha3_256 and then by blake2b_224
	uint8_t cborShaHash[32];
	sha3_256_hash(
	        VIEW_PROCESSED_TO_TUPLE_BUF_SIZE(&cbor),
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

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);
	{
		view_appendToken(&out, CBOR_TYPE_ARRAY, 3);
		{
			// 1
			view_appendToken(&out, CBOR_TYPE_BYTES, addressRootSize);
			view_appendData(&out, addressRoot, addressRootSize);
		} {
			// 2
			view_appendToken(&out, CBOR_TYPE_MAP, 0 /* addrAttributes is empty */);
		} {
			// 3
			view_appendToken(&out, CBOR_TYPE_UNSIGNED, CARDANO_ADDRESS_TYPE_PUBKEY);
		}
	}
	return view_processedSize(&out);
}


size_t cborPackRawAddressWithChecksum(
        const uint8_t* rawAddressBuffer, size_t rawAddressSize,
        uint8_t* outputBuffer, size_t outputSize
)
{
	ASSERT(rawAddressSize < BUFFER_SIZE_PARANOIA);
	ASSERT(outputSize < BUFFER_SIZE_PARANOIA);

	write_view_t output = make_write_view(outputBuffer, outputBuffer + outputSize);

	{
		// Format is
		// Array[
		//     tag(24):bytes(rawAddress),
		//     crc32(rawAddress)
		// ]
		view_appendToken(&output, CBOR_TYPE_ARRAY, 2);
		{
			view_appendToken(&output, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);
			view_appendToken(&output, CBOR_TYPE_BYTES, rawAddressSize);
			view_appendData(&output, rawAddressBuffer, rawAddressSize);
		} {
			uint32_t checksum = crc32(rawAddressBuffer, rawAddressSize);
			view_appendToken(&output, CBOR_TYPE_UNSIGNED, checksum);
		}
	}
	return view_processedSize(&output);
}

// TODO(ppershing): unite parsing with attestUtxo streams ...

// TODO(ppershing): Should we somehow deal with not enough input currently
// throwing ERR_NOT_ENOUGH_INPUT instead of ERR_INVALID_DATA?
static uint64_t parseToken(read_view_t* view, uint8_t type)
{
	const cbor_token_t token = view_readToken(view);
	VALIDATE(token.type == type, ERR_INVALID_DATA);
	return token.value;
}

static void parseTokenWithValue(read_view_t* view, uint8_t type, uint64_t value)
{
	const cbor_token_t token = view_readToken(view);
	VALIDATE(token.type == type, ERR_INVALID_DATA);
	VALIDATE(token.value  == value, ERR_INVALID_DATA);
}


size_t unboxChecksummedAddress(
        const uint8_t* addressBuffer, size_t addressSize,
        uint8_t* outputBuffer, size_t outputSize
)
{
	ASSERT(addressSize < BUFFER_SIZE_PARANOIA);
	ASSERT(outputSize < BUFFER_SIZE_PARANOIA);

	read_view_t view = make_read_view(addressBuffer, addressBuffer + addressSize);

	uint32_t checksum;
	size_t unboxedSize;
	const uint8_t* unboxedBuffer;
	{
		parseTokenWithValue(&view, CBOR_TYPE_ARRAY, 2);
		{
			parseTokenWithValue(&view, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);

			uint64_t tmp = parseToken(&view, CBOR_TYPE_BYTES);
			// Validate that we can down-cast
			STATIC_ASSERT(sizeof(tmp) >= sizeof(SIZE_MAX), "bad int size");
			VALIDATE(tmp < (uint64_t) SIZE_MAX, ERR_INVALID_DATA);

			unboxedSize = (size_t) tmp;

			unboxedBuffer = view.ptr;
			// overflow pre-check, should not be needed with view_skipBytes but ...
			VALIDATE(unboxedSize < BUFFER_SIZE_PARANOIA, ERR_INVALID_DATA);
			VALIDATE(unboxedSize <= view_remainingSize(&view), ERR_INVALID_DATA);

			view_skipBytes(&view, unboxedSize);
		}
		{
			checksum = crc32(unboxedBuffer, unboxedSize);
			parseTokenWithValue(&view, CBOR_TYPE_UNSIGNED, checksum);
		}
	}

	VALIDATE(view_remainingSize(&view) == 0, ERR_INVALID_DATA);

	VALIDATE(unboxedSize <= outputSize, ERR_DATA_TOO_LARGE);
	os_memmove(outputBuffer, unboxedBuffer, unboxedSize);
	return unboxedSize;
}


size_t deriveRawAddress(
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

	return cborEncodePubkeyAddressInner(
	               addressRoot, SIZEOF(addressRoot),
	               outBuffer, outSize
	       );
}

size_t deriveAddress(
        const bip44_path_t* pathSpec,
        uint8_t* outBuffer, size_t outSize
)
{
	uint8_t rawAddressBuffer[40];
	size_t rawAddressSize = deriveRawAddress(
	                                pathSpec,
	                                rawAddressBuffer, SIZEOF(rawAddressBuffer)
	                        );

	return cborPackRawAddressWithChecksum(
	               rawAddressBuffer, rawAddressSize,
	               outBuffer, outSize
	       );

}
