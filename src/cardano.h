#ifndef H_CARDANO_APP_CARDANO
#define H_CARDANO_APP_CARDANO

#include "common.h"

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

// Just a trick to make the numbers readable
#define __CONCAT4(A,B,C,D) A ## B ## C ## D

static const uint64_t LOVELACE_MAX_SUPPLY = __CONCAT4(45, 000, 000, 000) * 1000000;
static const uint64_t LOVELACE_INVALID =    __CONCAT4(47, 000, 000, 000) * 1000000;

STATIC_ASSERT(LOVELACE_MAX_SUPPLY < LOVELACE_INVALID, "bad LOVELACE_INVALID");


#endif
