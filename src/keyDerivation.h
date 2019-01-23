#ifndef H_CARDANO_APP_KEY_DERIVATION
#define H_CARDANO_APP_KEY_DERIVATION

#include "common.h"
#include "handlers.h"
#include "bip44.h"


static const size_t PUBLIC_KEY_SIZE =  32;
static const size_t CHAIN_CODE_SIZE =  32;
static const size_t EXTENDED_PUBKEY_SIZE = CHAIN_CODE_SIZE + PUBLIC_KEY_SIZE;

typedef cx_ecfp_256_extended_private_key_t privateKey_t;

typedef struct {
	uint8_t code[CHAIN_CODE_SIZE];
} chain_code_t;

typedef struct {
	uint8_t pubKey[PUBLIC_KEY_SIZE];
	uint8_t chainCode[CHAIN_CODE_SIZE];
} extendedPublicKey_t;

void derivePrivateKey(
        const bip44_path_t* pathSpec,
        chain_code_t* chainCode, // 32 byte output
        privateKey_t* privateKey // output
);


void deriveExtendedPublicKey(
        const bip44_path_t* pathSpec,
        extendedPublicKey_t* out
);

void deriveRawPublicKey(
        const privateKey_t* privateKey,
        cx_ecfp_public_key_t* publicKey // output
);

void extractRawPublicKey(
        const cx_ecfp_public_key_t* publicKey,
        uint8_t* outBuffer, size_t outSize
);


void run_key_derivation_test();
#endif
