#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "errors.h"
#include "keyDerivation.h"
#include "stream.h"
#include "cbor.h"
#include "hash.h"
#include "crc32.h"
#include "base58.h"
#include "utils.h"
#include "endian.h"
#include "cardano.h"

void derivePrivateKey(
        const bip44_path_t* pathSpec,
        chain_code_t* chainCode,
        privateKey_t* privateKey
)
{
	if (!bip44_hasValidPrefix(pathSpec)) {
		THROW(ERR_INVALID_BIP44_PATH);
	}
	// Sanity check
	ASSERT(pathSpec->length < ARRAY_LEN(pathSpec->path));

	uint8_t privateKeyRawBuffer[64];

	STATIC_ASSERT(SIZEOF(chainCode->code) == 32, "bad chain code length");
	os_memset(chainCode->code, 0, SIZEOF(chainCode->code));

	BEGIN_TRY {
		TRY {
			STATIC_ASSERT(CX_APILEVEL >= 5, "unsupported api level");
			STATIC_ASSERT(SIZEOF(privateKey->d) == 64, "bad private key length");

			os_perso_derive_node_bip32(
			        CX_CURVE_Ed25519,
			        pathSpec->path,
			        pathSpec->length,
			        privateKeyRawBuffer,
			        chainCode->code);

			// We should do cx_ecfp_init_private_key here, but it does not work in SDK < 1.5.4,
			// should work with the new SDK
			privateKey->curve = CX_CURVE_Ed25519;
			privateKey->d_len = 64;
			os_memmove(privateKey->d, privateKeyRawBuffer, 64);
		}
		FINALLY {
			os_memset(privateKeyRawBuffer, 0, SIZEOF(privateKeyRawBuffer));
		}
	} END_TRY;
}


void deriveRawPublicKey(
        const privateKey_t* privateKey,
        cx_ecfp_public_key_t* publicKey
)
{
	// We should do cx_ecfp_generate_pair here, but it does not work in SDK < 1.5.4,
	// should work with the new SDK
	cx_eddsa_get_public_key(
	        // cx_eddsa has a special case struct for Cardano's private keys
	        // but signature is standard
	        (const struct cx_ecfp_256_private_key_s *) privateKey,
	        CX_SHA512,
	        publicKey,
	        NULL, 0, NULL, 0);
}

void extractRawPublicKey(
        const cx_ecfp_public_key_t* publicKey,
        uint8_t* outBuffer, size_t outSize
)
{
	// copy public key little endian to big endian
	ASSERT(outSize == 32);
	STATIC_ASSERT(SIZEOF(publicKey->W) == 65, "bad public key length");

	uint8_t i;
	for (i = 0; i < 32; i++) {
		outBuffer[i] = publicKey->W[64 - i];
	}

	if ((publicKey->W[32] & 1) != 0) {
		outBuffer[31] |= 0x80;
	}
}


// pub_key + chain_code
void deriveExtendedPublicKey(
        const bip44_path_t* pathSpec,
        extendedPublicKey_t* out
)
{
	privateKey_t privateKey;
	chain_code_t chainCode;

	STATIC_ASSERT(SIZEOF(*out) == CHAIN_CODE_SIZE + PUBLIC_KEY_SIZE, "bad ext pub key size");

	BEGIN_TRY {
		TRY {
			derivePrivateKey(
			        pathSpec,
			        &chainCode,
			        &privateKey
			);

			// Pubkey part
			cx_ecfp_public_key_t publicKey;

			deriveRawPublicKey(&privateKey, &publicKey);

			STATIC_ASSERT(SIZEOF(out->pubKey) == PUBLIC_KEY_SIZE, "bad pub key size");

			extractRawPublicKey(&publicKey, out->pubKey, SIZEOF(out->pubKey));

			// Chain code (we copy it second to avoid mid-updates extractRawPublicKey throws
			STATIC_ASSERT(CHAIN_CODE_SIZE == SIZEOF(out->chainCode), "bad chain code size");
			STATIC_ASSERT(CHAIN_CODE_SIZE == SIZEOF(chainCode.code), "bad chain code size");
			os_memmove(out->chainCode, chainCode.code, CHAIN_CODE_SIZE);
		}
		FINALLY {
			os_memset(&privateKey, 0, SIZEOF(privateKey));
		}
	} END_TRY;
}


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

size_t cborEncodePubkeyAddress(
        const uint8_t* addressRoot, size_t addressRootSize,
        uint8_t* outBuffer, size_t outSize
        /* potential attributes */
)
{
	ASSERT(addressRootSize == 28); // should be result of blake2b_224
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	uint8_t* ptr = outBuffer;
	uint8_t* end = outBuffer + outSize;

	// Format is
	// [
	//     tag(24):bytes(CBOR{[bytes(rootAddress), map(attrs), type]}),
	//     checksum
	// ]

	// 1st level
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_ARRAY, 2);

	// 1
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);

	// Note(ppershing): here we expect we know the serialization
	// length. For now it is constant and we save some stack space
	// this way but in the future we might need to refactor this code
	const uint64_t INNER_SIZE= 33;
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_BYTES, INNER_SIZE);
	uint8_t* innerPtr;
	size_t innerSize = cborEncodePubkeyAddressInner(
	                           addressRoot, addressRootSize,
	                           ptr, end - ptr
	                   );

	ASSERT(innerSize == INNER_SIZE); // check that we have good serialization preamble!

	uint32_t checksum = crc32(innerPtr, innerSize);
	ptr += innerSize;

	// 2
	BUF_PTR_APPEND_TOKEN(ptr, end, CBOR_TYPE_UNSIGNED, checksum);

	size_t size = ptr - outBuffer;
	ASSERT(size <= outSize);
	return size;
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

	uint8_t addressRaw[100];
	size_t addressSize = cborEncodePubkeyAddress(
	                             addressRoot, SIZEOF(addressRoot),
	                             addressRaw, SIZEOF(addressRaw)
	                     );
	uint32_t length = encode_base58(
	                          addressRaw, addressSize,
	                          outBuffer, outSize
	                  );
	return length;
}
