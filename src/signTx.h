#ifndef H_CARDANO_APP_SIGN_TX
#define H_CARDANO_APP_SIGN_TX

#include "common.h"
#include "hash.h"
#include "handlers.h"
#include "txHashBuilder.h"
#include "bip44.h"
#include "addressUtilsShelley.h"

typedef enum {
	SIGN_STAGE_NONE = 0,
	SIGN_STAGE_INIT = 23,
	SIGN_STAGE_INPUTS = 24,
	SIGN_STAGE_OUTPUTS = 25,
	SIGN_STAGE_FEE = 26,
	SIGN_STAGE_TTL = 27,
	SIGN_STAGE_CERTIFICATES = 28,
	SIGN_STAGE_WITHDRAWALS = 29,
	SIGN_STAGE_METADATA = 30,
	SIGN_STAGE_CONFIRM = 31,
	SIGN_STAGE_WITNESSES = 32,
} sign_tx_stage_t;

enum { // TODO enum with the same values?
	SIGN_MAX_INPUTS = 1000,
	SIGN_MAX_OUTPUTS = 1000,
	SIGN_MAX_CERTIFICATES = 1000,
	SIGN_MAX_REWARD_WITHDRAWALS = 1000
};

#define METADATA_HASH_LENGTH 32

typedef struct {
	sign_tx_stage_t stage;

	uint64_t fee;
	uint64_t ttl;
	uint8_t networkId; // part of Shelley address
	uint32_t protocolMagic; // part of Byron address

	uint16_t numInputs;
	uint16_t numOutputs;
	uint16_t numCertificates;
	uint16_t numWithdrawals; // reward withdrawals
	uint16_t numWitnesses;
	bool includeMetadata;

	uint16_t currentInput;
	uint16_t currentOutput;
	uint16_t currentCertificate;
	uint16_t currentWithdrawal;
	uint16_t currentWitness;

	tx_hash_builder_t txHashBuilder;
	uint8_t txHash[TX_HASH_LENGTH];

	uint64_t currentAmount;
	struct {
		uint8_t buffer[MAX_ADDRESS_SIZE];
		size_t size;
	} currentAddress;
	addressParams_t currentAddressParams;
	uint8_t metadataHash[METADATA_HASH_LENGTH];
	uint8_t certificateType;
	bip44_path_t certificateKeyPath;
	uint8_t certificatePoolKeyHash[POOL_KEY_HASH_LENGTH];

	uint8_t currentWitnessData[64];
	bip44_path_t currentWitnessPath;

	int ui_step;
} ins_sign_tx_context_t;

handler_fn_t signTx_handleAPDU;

#endif
