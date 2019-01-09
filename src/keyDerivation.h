#ifndef H_CARDANO_APP_KEY_DERIVATION
#define H_CARDANO_APP_KEY_DERIVATION

#include <os.h>
#include "handlers.h"

static const uint32_t BIP_44 = 44;
static const uint32_t ADA_COIN_TYPE = 1815;
static const uint32_t HARDENED_BIP32 = ((uint32_t) 1 << 31);

typedef cx_ecfp_256_extended_private_key_t privateKey_t;

typedef struct {
	uint8_t code[32];
} chain_code_t;

void derivePrivateKey(
        uint32_t* bip32Path, uint32_t pathLength,
        chain_code_t* chainCode, // 32 byte output
        privateKey_t* privateKey // output
);

void derivePublicKey(
        privateKey_t* privateKey,
        cx_ecfp_public_key_t* publicKey // output
);

void extractRawPublicKey(
        cx_ecfp_public_key_t* publicKey,
        uint8_t* rawPublicKey // 32 byte output
);

void key_derivation_test();
#endif
