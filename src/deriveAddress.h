#ifndef H_CARDANO_APP_DERIVE_ADDRESS
#define H_CARDANO_APP_DERIVE_ADDRESS

#include "common.h"
#include "bip44.h"
#include "handlers.h"
#include "addressUtilsShelley.h"

handler_fn_t deriveAddress_handleAPDU;

typedef struct {
	uint16_t responseReadyMagic;
	addressParams_t addressParams;
	struct {
		uint8_t buffer[MAX_ADDRESS_SIZE];
		size_t size;
	} address;
	int ui_step;
} ins_derive_address_context_t;

#endif
