#ifndef H_CARDANO_APP_GET_PUB_KEY
#define H_CARDANO_APP_GET_PUB_KEY

#include "common.h"
#include "handlers.h"
#include "bip44.h"
#include "keyDerivation.h"

handler_fn_t getExtendedPublicKey_handleAPDU;

typedef struct {
	int16_t responseReadyMagic;
	bip44_path_t pathSpec;
	extendedPublicKey_t extPubKey;
	int ui_step;
} ins_get_ext_pubkey_context_t;

#endif
