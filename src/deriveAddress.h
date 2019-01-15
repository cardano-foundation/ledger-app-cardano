#ifndef H_CARDANO_APP_DERIVE_ADDRESS
#define H_CARDANO_APP_DERIVE_ADDRESS

#include <os.h>
#include "keyDerivation.h"

void handleDeriveAddress(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength
);

typedef struct {
	path_spec_t pathSpec;
	uint8_t address[128];
	uint8_t addressLength;
} derive_address_data_t;

#endif
