#ifndef H_CARDANO_APP_CARDANO
#define H_CARDANO_APP_CARDANO

#include "common.h"

enum {
	CARDANO_INPUT_TYPE_UTXO = 0,
	// No other known input types
};


// Just a trick to make the numbers readable
#define __CONCAT4(A,B,C,D) A ## B ## C ## D

static const uint64_t LOVELACE_MAX_SUPPLY = __CONCAT4(45, 000, 000, 000) * 1000000;
static const uint64_t LOVELACE_INVALID =    __CONCAT4(47, 000, 000, 000) * 1000000;

STATIC_ASSERT(LOVELACE_MAX_SUPPLY < LOVELACE_INVALID, "bad LOVELACE_INVALID");

#define ADDRESS_KEY_HASH_LENGTH 28
#define POOL_KEY_HASH_LENGTH 28
#define TX_HASH_LENGTH 32


// for Shelley, address is at most 1 + 28 + 28 = 57 bytes,
// encoded in bech32 as 11 + 8/5 * 57 = 103 chars

// for Byron, the address can contain 64B of data (according to Duncan),
// plus 46B with empty data; 100B in base58 has
// length at most 139
// (previously, we used 128 bytes)
// https://stackoverflow.com/questions/48333136/size-of-buffer-to-hold-base58-encoded-data
// TODO verify
#define MAX_ADDRESS_SIZE 128
#define MAX_HUMAN_ADDRESS_LENGTH 150


#endif
