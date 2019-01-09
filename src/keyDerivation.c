#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "errors.h"
#include "keyDerivation.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

void validatePath(uint32_t* path, uint32_t pathLength)
{
	uint32_t bip44 = BIP_44 | HARDENED_BIP32;
	uint32_t adaCoinType = ADA_COIN_TYPE | HARDENED_BIP32;

	VALIDATE_PARAM(pathLength >= 3 && pathLength <= 10);

	VALIDATE_PARAM(path[0] ==  bip44);
	VALIDATE_PARAM(path[1] ==  adaCoinType);
	VALIDATE_PARAM(path[2] >= HARDENED_BIP32);
}

void derivePrivateKey(
        uint32_t* bip32Path, uint32_t pathLength,
        chain_code_t* chainCode,
        privateKey_t* privateKey
)
{
	validatePath(bip32Path, pathLength);

	uint8_t privateKeyRaw[64];

	STATIC_ASSERT(sizeof(chainCode->code) == 32, __bad_length);
	os_memset(chainCode->code, 0, sizeof(chainCode->code));

	BEGIN_TRY {
		TRY {
			STATIC_ASSERT(CX_APILEVEL >= 5, unsupported_api_level);
			os_perso_derive_node_bip32(
			        CX_CURVE_Ed25519,
			        bip32Path,
			        pathLength,
			        privateKeyRaw,
			        chainCode->code);

			// We should do cx_ecfp_init_private_key here, but it does not work in SDK < 1.5.4,
			// should work with the new SDK
			privateKey->curve = CX_CURVE_Ed25519;
			privateKey->d_len = 64;
			os_memmove(privateKey->d, privateKeyRaw, 64);
		}
		FINALLY {
			os_memset(privateKeyRaw, 0, sizeof(privateKeyRaw));
		}
	} END_TRY;
}


void derivePublicKey(privateKey_t* privateKey,
                     cx_ecfp_public_key_t* publicKey)
{
	// We should do cx_ecfp_generate_pair here, but it does not work in SDK < 1.5.4,
	// should work with the new SDK
	cx_eddsa_get_public_key(
	        privateKey,
	        CX_SHA512,
	        publicKey,
	        NULL, 0, NULL, 0);
}

void extractRawPublicKey(cx_ecfp_public_key_t* publicKey, uint8_t* rawPublicKey)
{
	// copy public key little endian to big endian
	uint8_t i;
	for (i = 0; i < 32; i++) {
		rawPublicKey[i] = publicKey->W[64 - i];
	}

	if ((publicKey->W[32] & 1) != 0) {
		rawPublicKey[31] |= 0x80;
	}
}