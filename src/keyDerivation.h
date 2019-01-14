#ifndef H_CARDANO_APP_KEY_DERIVATION
#define H_CARDANO_APP_KEY_DERIVATION

#include <os.h>
#include "handlers.h"

static const uint32_t BIP_44 = 44;
static const uint32_t ADA_COIN_TYPE = 1815;
static const uint32_t HARDENED_BIP32 = ((uint32_t) 1 << 31);

static const size_t PUBLIC_KEY_SIZE =  32;
static const size_t CHAIN_CODE_SIZE =  32;
static const size_t EXTENDED_PUBKEY_SIZE = CHAIN_CODE_SIZE + PUBLIC_KEY_SIZE;

typedef cx_ecfp_256_extended_private_key_t privateKey_t;

typedef struct {
	uint8_t code[CHAIN_CODE_SIZE];
} chain_code_t;

void derivePrivateKey(
        const uint32_t* bip32Path, uint32_t pathLength,
        chain_code_t* chainCode, // 32 byte output
        privateKey_t* privateKey // output
);

void deriveRawPublicKey(
        const privateKey_t* privateKey,
        cx_ecfp_public_key_t* publicKey // output
);

void extractRawPublicKey(
        const cx_ecfp_public_key_t* publicKey,
        // 32 byte output
        uint8_t* rawPublicKey, size_t rawPublicKeySize
);

void derivePublicKey(
        const uint32_t* bip32Path, uint32_t pathLength,
        uint8_t* output, size_t outputSize
);

uint32_t deriveAddress(
        const uint32_t* bip32Path, uint32_t pathLength,
        uint8_t* output, size_t outputSize
);

void run_key_derivation_test();
#endif
