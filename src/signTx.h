#ifndef H_CARDANO_APP_SIGN_TX
#define H_CARDANO_APP_SIGN_TX

#include "common.h"
#include "hash.h"
#include "handlers.h"
#include "txHashBuilder.h"
#include "bip44.h"

typedef enum {
	SIGN_STAGE_NONE = 0,
	SIGN_STAGE_INPUTS = 24,
	SIGN_STAGE_OUTPUTS = 25,
	SIGN_STAGE_CONFIRM = 26,
	SIGN_STAGE_WITNESSES = 27,
} sign_tx_stage_t;

enum {
	SIGN_MAX_INPUTS = 1000,
	SIGN_MAX_OUTPUTS = 1000,
};

typedef struct {
	sign_tx_stage_t stage;
	uint16_t numInputs;
	uint16_t numOutputs;
	uint16_t numWitnesses;

	uint16_t currentInput;
	uint16_t currentOutput;
	uint16_t currentWitness;

	uint64_t sumAmountInputs;
	uint64_t sumAmountOutputs;
	tx_hash_builder_t txHashBuilder;
	uint8_t txHash[32];
	uint8_t currentWitnessData[64];
	uint64_t currentAmount;
	struct {
		uint8_t buffer[200];
		size_t size;
	} currentAddress;
	bip44_path_t currentPath;
	int ui_step;
} ins_sign_tx_context_t;

handler_fn_t signTx_handleAPDU;

#endif
