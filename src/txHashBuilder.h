#ifndef H_CARDANO_APP_TX_HASH_BUILDER
#define H_CARDANO_APP_TX_HASH_BUILDER

#include "hash.h"

typedef enum {
	TX_HASH_BUILDER_INIT = 1,
	TX_HASH_BUILDER_IN_INPUTS = 2,
	TX_HASH_BUILDER_IN_OUTPUTS = 3,
	TX_HASH_BUILDER_IN_METADATA = 4,
	TX_HASH_BUILDER_FINISHED = 5,
} tx_hash_builder_state_t;

typedef struct {
	tx_hash_builder_state_t state;
	blake2b_256_context_t txHash;
} tx_hash_builder_t;


void txHashBuilder_init(tx_hash_builder_t* builder);

void txHashBuilder_enterInputs(tx_hash_builder_t* builder);

void txHashBuilder_addUtxoInput(
        tx_hash_builder_t* builder,
        const uint8_t* utxoHashBuffer, size_t utxoHashSize,
        uint32_t utxoIndex
);

void txHashBuilder_enterOutputs(tx_hash_builder_t* builder);

void txHashBuilder_addOutput(
        tx_hash_builder_t* builder,
        const uint8_t* rawAddressBuffer, size_t rawAddressSize,
        uint64_t amount
);

void txHashBuilder_enterMetadata(tx_hash_builder_t* builder);

void txHashBuilder_finalize(
        tx_hash_builder_t* builder,
        uint8_t* outBuffer, size_t outSize
);

void run_txHashBuilder_test();
#endif
