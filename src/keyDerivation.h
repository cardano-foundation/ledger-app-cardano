#ifndef H_CARDANO_APP_KEY_DERIVATION
#define H_CARDANO_APP_KEY_DERIVATION

#include <os.h>
#include "handlers.h"

#define BIP_44 44
#define ADA_COIN_TYPE 1815
#define HARDENED_BIP32 0x80000000

void derivePrivateKey(
        uint32_t* bip32Path, uint32_t pathLength,
        uint8_t* chainCode, // 32 byte output
        cx_ecfp_256_extended_private_key_t* privateKey // output
);

void derivePublicKey(
        cx_ecfp_256_extended_private_key_t* privateKey,
        cx_ecfp_public_key_t* publicKey // output
);

void getRawPublicKey(
        cx_ecfp_public_key_t* publicKey,
        uint8_t* rawPublicKey // 32 byte output
);

void key_derivation_test();
#endif
