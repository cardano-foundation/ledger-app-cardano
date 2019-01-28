#ifndef H_CARDANO_APP_DERIVE_ADDRESS
#define H_CARDANO_APP_DERIVE_ADDRESS

#include "common.h"
#include "bip44.h"
#include "handlers.h"

handler_fn_t deriveAddress_handleAPDU;

typedef struct {
	uint16_t responseReadyMagic;
	bip44_path_t pathSpec;
	uint8_t addressBuffer[128];
	size_t addressSize;
} ins_derive_address_context_t;

#endif
