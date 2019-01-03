#ifndef H_CARDANO_APP_GET_PUB_KEY
#define H_CARDANO_APP_GET_PUB_KEY

#include "handlers.h"

#define MAX_BIP32_PATH 10
#define BIP_44 44
#define ADA_COIN_TYPE 1815
#define HARDENED_BIP32 0x80000000

handler_fn_t handleGetExtendedPublicKey;

typedef struct {
	uint32_t bip32Path[MAX_BIP32_PATH];
	uint8_t pathLength;
	uint8_t privateKeyData[64];
	uint8_t chainCode[32];
	cx_ecfp_public_key_t publicKey;
	cx_ecfp_256_extended_private_key_t privateKey;
	// cx_ecfp_private_key_t privateKey;
} get_ext_pub_key_data_t;

#endif
