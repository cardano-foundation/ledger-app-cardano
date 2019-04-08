#ifndef H_CARDANO_APP_ATTEST_UTXO
#define H_CARDANO_APP_ATTEST_UTXO

#include "common.h"
#include "stream.h"
#include "hash.h"
#include "handlers.h"

// Note: For the safety concerns, we start enums
// at non-overlapping ranges
typedef enum {
	MAIN_PARSING_UNINITIALIZED_PARANOIA = 47,
	MAIN_PARSING_NOT_STARTED,
	MAIN_EXPECT_TX_PREAMBLE, // array(3)
	MAIN_EXPECT_INPUTS_PREAMBLE, // array(*)
	MAIN_EXPECT_END_OR_INPUT,
	MAIN_IN_INPUT,
	// end of inputs
	MAIN_EXPECT_OUTPUTS_PREAMBLE, // array(*)
	MAIN_EXPECT_END_OR_OUTPUT,
	MAIN_IN_OUTPUT,
	// end of outputs
	MAIN_EXPECT_METADATA_PREAMBLE, // {}
	// no metadata currently supported
	MAIN_FINISHED,
} txMainDecoderState_t;

typedef enum {
	INPUT_PARSING_UNINITIALIZED_PARANOIA = 74,
	INPUT_PARSING_NOT_STARTED,
	INPUT_EXPECT_PREAMBLE, // array(2)
	INPUT_EXPECT_TYPE, // unsigned(0)
	INPUT_EXPECT_TAG, // tag(24)
	INPUT_EXPECT_ADDRESS_DATA_PREAMBLE, // bytes(len)
	INPUT_IN_ADDRESS_DATA, // bytestring of len
	INPUT_FINISHED,
} txInputDecoderState_t;

typedef enum {
	OUTPUT_PARSING_UNINITIALIZED_PARANOIA = 147,
	OUTPUT_PARSING_NOT_STARTED,
	OUTPUT_EXPECT_PREAMBLE, // array(2)

	OUTPUT_EXPECT_ADDRESS_PREAMBLE, // array(2)
	OUTPUT_EXPECT_ADDRESS_TAG, // tag(24)
	OUTPUT_EXPECT_ADDRESS_DATA_PREAMBLE, // bytes(len)
	OUTPUT_IN_ADDRESS_DATA, // bytestring of len
	// end of array(2)
	OUTPUT_EXPECT_ADDRESS_CHECKSUM, // unsigned() variable len
	// end of address
	OUTPUT_EXPECT_AMOUNT, // unsigned() (variable len)
	// end of output
	OUTPUT_FINISHED,
} txOutputDecoderState_t;

typedef enum {
	INITIALIZED = 174,
} attestUtxoStage_t;


typedef struct {
	uint16_t parserInitializedMagic;
	// parser state
	txMainDecoderState_t mainState;
	txInputDecoderState_t inputState;
	txOutputDecoderState_t outputState;
	uint64_t addressDataRemainingBytes;

	stream_t stream;
	// bookkeeping data
	uint32_t currentOutputIndex;
	uint32_t attestedOutputIndex;
	uint64_t outputAmount;
} attest_utxo_parser_state_t ;

typedef struct {
	uint16_t initializedMagic;
	attest_utxo_parser_state_t parserState;
	blake2b_256_context_t txHashCtx;
} ins_attest_utxo_context_t;

void parser_keepParsing(attest_utxo_parser_state_t *state);
void parser_init(attest_utxo_parser_state_t *state, uint32_t outputIndex);
uint64_t parser_getAttestedAmount(attest_utxo_parser_state_t* state);

handler_fn_t attestUTxO_handleAPDU;

void run_test_attestUtxo();

#endif
