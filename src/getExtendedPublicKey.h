#ifndef H_CARDANO_APP_GET_PUB_KEY
#define H_CARDANO_APP_GET_PUB_KEY

#include <os.h>
#include "handlers.h"
#include "keyDerivation.h"

handler_fn_t handleGetExtendedPublicKey;

typedef struct {
	int16_t responseReadyMagic;
	bip44_path_t pathSpec;
	extendedPublicKey_t extPubKey;
} ins_get_ext_pubkey_context_t;

#endif
