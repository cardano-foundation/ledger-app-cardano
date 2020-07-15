#ifndef H_CARDANO_APP_TX_HASH_BUILDER
#define H_CARDANO_APP_TX_HASH_BUILDER

#include "hash.h"

enum {
	TX_BODY_KEY_INPUTS = 0,
	TX_BODY_KEY_OUTPUTS = 1,
	TX_BODY_KEY_FEE = 2,
	TX_BODY_KEY_TTL = 3,
	TX_BODY_KEY_CERTIFICATES = 4,
	TX_BODY_KEY_WITHDRAWALS = 5,
	// TX_BODY_KEY_UPDATE = 6, // not used
	TX_BODY_KEY_METADATA = 7
};

// there are other types we do not support
enum {
	CERTIFICATE_TYPE_STAKE_REGISTRATION = 0,
	CERTIFICATE_TYPE_STAKE_DEREGISTRATION = 1,
	CERTIFICATE_TYPE_STAKE_DELEGATION = 2
};

typedef enum {
	TX_HASH_BUILDER_INIT = 1,
	TX_HASH_BUILDER_IN_INPUTS = 2,
	TX_HASH_BUILDER_IN_OUTPUTS = 3,
	TX_HASH_BUILDER_IN_FEE = 4,
	TX_HASH_BUILDER_IN_TTL = 5,
	TX_HASH_BUILDER_IN_CERTIFICATES = 6,
	TX_HASH_BUILDER_IN_WITHDRAWALS = 7,
	TX_HASH_BUILDER_IN_METADATA = 8,
	TX_HASH_BUILDER_FINISHED = 9,
} tx_hash_builder_state_t;

typedef struct {
	uint16_t numWithdrawals;
	uint16_t numCertificates;
	bool includeMetadata;

	tx_hash_builder_state_t state;
	blake2b_256_context_t txHash;
} tx_hash_builder_t;


void txHashBuilder_init(
        tx_hash_builder_t* builder,
        uint16_t numCertificates, uint16_t numWithdrawals, bool includeMetadata
);

void txHashBuilder_enterInputs(tx_hash_builder_t* builder, uint16_t numInputs);
void txHashBuilder_addInput(
        tx_hash_builder_t* builder,
        const uint8_t* utxoHashBuffer, size_t utxoHashSize,
        uint32_t utxoIndex
);

void txHashBuilder_enterOutputs(tx_hash_builder_t* builder, uint16_t numOutputs);
void txHashBuilder_addOutput(
        tx_hash_builder_t* builder,
        const uint8_t* addressBuffer, size_t addressSize,
        uint64_t amount
);

void txHashBuilder_addFee(tx_hash_builder_t* builder, uint64_t fee);

void txHashBuilder_addTtl(tx_hash_builder_t* builder, uint64_t ttl);

void txHashBuilder_enterCertificates(tx_hash_builder_t* builder);
void txHashBuilder_addCertificate_stakingKey(
        tx_hash_builder_t* builder,
        const int certificateType,
        const uint8_t* stakingKeyHash, size_t stakingKeyHashSize
);
void txHashBuilder_addCertificate_delegation(
        tx_hash_builder_t* builder,
        const uint8_t* stakingKeyHash, size_t stakingKeyHashSize,
        const uint8_t* poolKeyHash, size_t poolKeyHashSize
);

void txHashBuilder_enterWithdrawals(tx_hash_builder_t* builder);
void txHashBuilder_addWithdrawal(
        tx_hash_builder_t* builder,
        const uint8_t* rewardAccountBuffer, size_t rewardAccountSize,
        uint64_t amount
);

void txHashBuilder_addMetadata(
        tx_hash_builder_t* builder,
        const uint8_t* metadataHashBuffer, size_t metadataHashSize
);

void txHashBuilder_finalize(
        tx_hash_builder_t* builder,
        uint8_t* outBuffer, size_t outSize
);

void run_txHashBuilder_test();

#endif // H_CARDANO_APP_TX_HASH_BUILDER
