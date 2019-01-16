#ifndef H_CARDANO_APP_GET_PUB_KEY
#define H_CARDANO_APP_GET_PUB_KEY

#include <os.h>
#include "handlers.h"
#include "keyDerivation.h"

handler_fn_t handleGetExtendedPublicKey;

typedef struct {
	path_spec_t pathSpec;
	extendedPublicKey_t extPubKey;
} getExtendedPublicKeyGlobal_t;

#endif
