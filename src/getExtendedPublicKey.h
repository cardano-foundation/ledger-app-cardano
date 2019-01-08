#ifndef H_CARDANO_APP_GET_PUB_KEY
#define H_CARDANO_APP_GET_PUB_KEY

#include <os.h>
#include "handlers.h"

#define MAX_BIP32_PATH 10

handler_fn_t handleGetExtendedPublicKey;

typedef struct {
	uint32_t bip32Path[MAX_BIP32_PATH];
	uint32_t pathLength;
	uint8_t chainCode[32];
	cx_ecfp_public_key_t publicKey;
	cx_ecfp_256_extended_private_key_t privateKey;
} get_ext_pub_key_data_t;

#endif
