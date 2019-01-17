#ifndef H_CARDANO_APP_CARDANO
#define H_CARDANO_APP_CARDANO

#include <stdint.h>

enum {
	CARDANO_INPUT_TYPE_UTXO = 0,
	// No other known input types
};

enum {
	CARDANO_ADDRESS_TYPE_PUBKEY = 0,
	/*
	CARDANO_ADDRESS_TYPE_SCRIPT = 1,
	CARDANO_ADDRESS_TYPE_REDEEM = 2,
	*/
};

#endif
