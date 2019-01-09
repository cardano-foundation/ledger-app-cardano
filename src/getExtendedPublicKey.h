#ifndef H_CARDANO_APP_GET_PUB_KEY
#define H_CARDANO_APP_GET_PUB_KEY

#include <os.h>
#include "handlers.h"
#include "keyDerivation.h"

static const uint32_t MAX_BIP32_PATH =  10;

handler_fn_t handleGetExtendedPublicKey;

typedef struct {
	uint32_t bip32Path[MAX_BIP32_PATH];
	uint32_t pathLength;
	chain_code_t chainCode;
	cx_ecfp_public_key_t publicKey;
	privateKey_t privateKey;
} get_ext_pub_key_data_t;

#endif
