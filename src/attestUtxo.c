#include "common.h"
#include "attestUtxo.h"
#include "cbor.h"
#include "test_utils.h"
#include "endian.h"
#include "hash.h"
#include "hmac.h"
#include "attestKey.h"
#include "state.h"
#include "cardano.h"
#include "securityPolicy.h"

static ins_attest_utxo_context_t* ctx = &(instructionState.attestUtxoContext);

// We do not parse addresses, just keep streaming over then
void parser_skipThroughAddressBytes(attest_utxo_parser_state_t* state)
{
	stream_t* stream = &(state->stream); // shorthand
	stream_ensureAvailableBytes(stream, 1); // We have to consume at least something
	uint64_t bytesToConsume = stream_availableBytes(stream);
	if (bytesToConsume > state->addressDataRemainingBytes) {
		bytesToConsume = state->addressDataRemainingBytes;
	}
	stream_advancePos(stream, bytesToConsume);
	state->addressDataRemainingBytes -= bytesToConsume;
}

// Performs a single transition on the Input parser
void parser_advanceInputState(attest_utxo_parser_state_t* state)
{
	stream_t* stream = &(state->stream); // shorthand
	switch (state->inputState) {

	case INPUT_PARSING_NOT_STARTED:
		state->inputState = INPUT_EXPECT_PREAMBLE;
		break;

	case INPUT_EXPECT_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 2);
		state->inputState = INPUT_EXPECT_TYPE;
		break;

	case INPUT_EXPECT_TYPE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_UNSIGNED, CARDANO_INPUT_TYPE_UTXO);
		state->inputState = INPUT_EXPECT_TAG;
		break;

	case INPUT_EXPECT_TAG:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);
		state->inputState = INPUT_EXPECT_ADDRESS_DATA_PREAMBLE;
		break;

	case INPUT_EXPECT_ADDRESS_DATA_PREAMBLE:

		// Warning: Following two lines have to happen
		// exactly in this order as takeCborBytesPreamble might throw
		state->addressDataRemainingBytes = cbor_takeToken(stream, CBOR_TYPE_BYTES);

		state->inputState = INPUT_IN_ADDRESS_DATA;
		break;

	case INPUT_IN_ADDRESS_DATA:
		if (state->addressDataRemainingBytes == 0) {
			state->inputState = INPUT_FINISHED;
		} else {
			parser_skipThroughAddressBytes(state);
		}
		break;

	case INPUT_FINISHED:
		// should be handled by the caller
		ASSERT(false);

	default:
		// unknown state
		ASSERT(false);

	}
}

// Makes one step on the transaction Output parser
void parser_advanceOutputState(attest_utxo_parser_state_t* state)
{
	stream_t* stream = & state->stream;
	switch(state-> outputState) {

	case OUTPUT_PARSING_NOT_STARTED:
		state->outputState = OUTPUT_EXPECT_PREAMBLE;
		break;

	case OUTPUT_EXPECT_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 2);
		state->outputState = OUTPUT_EXPECT_ADDRESS_PREAMBLE;
		break;

	case OUTPUT_EXPECT_ADDRESS_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 2);
		state->outputState = OUTPUT_EXPECT_ADDRESS_TAG;
		break;

	case OUTPUT_EXPECT_ADDRESS_TAG:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);
		state->outputState = OUTPUT_EXPECT_ADDRESS_DATA_PREAMBLE;
		break;

	case OUTPUT_EXPECT_ADDRESS_DATA_PREAMBLE:
		// Warning: Following two lines have to happen
		// exactly in this order as takeCborBytesPreamble might throw
		state->addressDataRemainingBytes = cbor_takeToken(stream, CBOR_TYPE_BYTES);

		state->outputState = OUTPUT_IN_ADDRESS_DATA;
		break;

	case OUTPUT_IN_ADDRESS_DATA:
		if (state->addressDataRemainingBytes == 0) {
			state->outputState = OUTPUT_EXPECT_ADDRESS_CHECKSUM;
		} else {
			parser_skipThroughAddressBytes(state);
		}
		break;

	case OUTPUT_EXPECT_ADDRESS_CHECKSUM:
		// Note: we do not check checksum
		cbor_takeToken(stream, CBOR_TYPE_UNSIGNED);
		state->outputState = OUTPUT_EXPECT_AMOUNT;
		break;

	case OUTPUT_EXPECT_AMOUNT:
		;
		{
			uint64_t value = cbor_takeToken(stream, CBOR_TYPE_UNSIGNED);
			if (state->currentOutputIndex == state->attestedOutputIndex) {
				state->outputAmount = value;
			}
		}
		state->outputState = OUTPUT_FINISHED;
		break;

	case OUTPUT_FINISHED:
		// this should be handled by the caller

		ASSERT(false);
		break;

	default:
		// unexpected state
		ASSERT(false);
	}
}

// Makes one parsing step or throws ERR_NOT_ENOUGH_INPUT
void parser_advanceMainState(attest_utxo_parser_state_t *state)
{
	stream_t* stream = &(state->stream); // shorthand
	switch (state->mainState) {

	case MAIN_PARSING_NOT_STARTED:
		state->mainState = MAIN_EXPECT_TX_PREAMBLE;
		break;

	case MAIN_EXPECT_TX_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 3);
		state->mainState = MAIN_EXPECT_INPUTS_PREAMBLE;
		break;

	case MAIN_EXPECT_INPUTS_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY_INDEF, 0);
		state->mainState = MAIN_EXPECT_END_OR_INPUT;
		break;

	case MAIN_EXPECT_END_OR_INPUT:
		if (cbor_peekNextIsIndefEnd(stream)) {
			cbor_takeTokenWithValue(stream, CBOR_TYPE_INDEF_END, 0);
			state->mainState = MAIN_EXPECT_OUTPUTS_PREAMBLE;
		} else {
			MEMCLEAR(& state->inputState, txInputDecoderState_t);
			state->inputState = INPUT_PARSING_NOT_STARTED;
			state->mainState = MAIN_IN_INPUT;
		}
		break;

	case MAIN_IN_INPUT:
		if (state->inputState == INPUT_FINISHED) {
			state->mainState = MAIN_EXPECT_END_OR_INPUT;
		} else {
			parser_advanceInputState(state);
		}
		break;

	case MAIN_EXPECT_OUTPUTS_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY_INDEF, 0);
		state->mainState = MAIN_EXPECT_END_OR_OUTPUT;
		state->currentOutputIndex = 0;
		break;

	case MAIN_EXPECT_END_OR_OUTPUT:
		if (cbor_peekNextIsIndefEnd(stream)) {
			cbor_takeTokenWithValue(stream, CBOR_TYPE_INDEF_END, 0);
			state->mainState = MAIN_EXPECT_METADATA_PREAMBLE;
		} else {
			MEMCLEAR(& state->outputState, txOutputDecoderState_t);
			state->outputState = OUTPUT_PARSING_NOT_STARTED;
			state->mainState = MAIN_IN_OUTPUT;
		}
		break;

	case MAIN_IN_OUTPUT:
		if (state->outputState == OUTPUT_FINISHED) {
			state->mainState = MAIN_EXPECT_END_OR_OUTPUT;
			state->currentOutputIndex += 1;
		} else {
			parser_advanceOutputState(state);
		}
		break;

	case MAIN_EXPECT_METADATA_PREAMBLE:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_MAP, 0);
		state->mainState = MAIN_FINISHED;
		break;

	case MAIN_FINISHED:
		// should be handled separately by the outer loop
		ASSERT(false);
		break;

	default:
		// unexpected state
		ASSERT(false);
	}
}

void parser_init(
        attest_utxo_parser_state_t* state,
        int outputIndex
)
{
	MEMCLEAR(state, attest_utxo_parser_state_t);
	state->mainState = MAIN_PARSING_NOT_STARTED;
	state->outputAmount = LOVELACE_INVALID;
	state->attestedOutputIndex = outputIndex;
	stream_init(& state->stream);
	state->parserInitializedMagic = ATTEST_PARSER_INIT_MAGIC;
}

// TODO(ppershing): revisit these conditions
uint64_t parser_getAttestedAmount(attest_utxo_parser_state_t* state)
{
	if (state->mainState != MAIN_FINISHED) return LOVELACE_INVALID;
	if (state->currentOutputIndex <= state->attestedOutputIndex) return LOVELACE_INVALID;
	if (state->outputAmount > LOVELACE_MAX_SUPPLY) return LOVELACE_INVALID;
	return state->outputAmount;
}

static bool parser_isInitialized(attest_utxo_parser_state_t* state)
{
	return state->parserInitializedMagic == ATTEST_PARSER_INIT_MAGIC;
}

// throws ERR_NOT_ENOUGH_INPUT when cannot proceed further
void parser_keepParsing(attest_utxo_parser_state_t* state)
{
	ASSERT(parser_isInitialized(state));

	while(state -> mainState != MAIN_FINISHED) {
		parser_advanceMainState(state);
	}
}


// TODO: move this to the global state union
attest_utxo_parser_state_t global_state;

enum {
	P1_INITIAL = 0x01,
	P1_CONTINUE = 0x02,
};


void attestUtxo_sendResponse()
{
	// Response is (txHash, outputNumber, outputAmount, HMAC)
	// outputAmount
	uint64_t amount = parser_getAttestedAmount(&ctx->parserState);
	if (amount == LOVELACE_INVALID) {
		THROW(ERR_INVALID_DATA);
	}

	struct {
		struct {
			uint8_t txHash[32];
			uint8_t index[4];
			uint8_t amount[8];
		} data;
		uint8_t hmac[16];
	} response;

	STATIC_ASSERT(SIZEOF(response) == 32+4+8+16, "response is packed");

	blake2b_256_finalize(&ctx ->txHashCtx, response.data.txHash, SIZEOF(response.data.txHash));
	u4be_write(response.data.index, ctx->parserState.attestedOutputIndex);
	u8be_write(response.data.amount, amount);

	attest_writeHmac(
	        (uint8_t*) &response.data, SIZEOF(response.data),
	        response.hmac, SIZEOF(response.hmac)
	);
	io_send_buf(SUCCESS, (uint8_t*) &response, SIZEOF(response));
}


void processNextChunk(uint8_t* dataBuffer, size_t dataSize)
{
	BEGIN_TRY {
		TRY {
			stream_appendData(& ctx->parserState.stream, dataBuffer, dataSize);
			blake2b_256_append(& ctx->txHashCtx, dataBuffer, dataSize);
			parser_keepParsing(& ctx->parserState);
			ASSERT(ctx->parserState.mainState == MAIN_FINISHED);
			attestUtxo_sendResponse();
			ui_idle();
		}
		CATCH(ERR_NOT_ENOUGH_INPUT)
		{
			// Respond that we need more data
			io_send_buf(SUCCESS, NULL, 0);
			// Note(ppershing): no ui_idle() as we continue exchange...
		}
		CATCH(ERR_UNEXPECTED_TOKEN)
		{
			// Convert to ERR_INVALID_DATA
			// which is handled at the main() level
			THROW(ERR_INVALID_DATA);
		}
		FINALLY {
		}
	} END_TRY;
}


void handle_attestUtxo(
        uint8_t p1,
        uint8_t p2,
        uint8_t* dataBuffer,
        size_t dataSize
)
{
	VALIDATE(p1 == P1_INITIAL || p1 == P1_CONTINUE, ERR_INVALID_REQUEST_PARAMETERS);

	if (p1 == P1_INITIAL) {
		VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);
		VALIDATE(dataSize >= 4, ERR_INVALID_DATA);

		security_policy_t policy = policyForAttestUtxo();
		if (policy == POLICY_DENY) {
			THROW(ERR_REJECTED_BY_POLICY);
		} else if (policy == POLICY_ALLOW) {
			// pass
		} else {
			ASSERT(false); // not implemented
		}

		uint32_t outputIndex = u4be_read(dataBuffer);
		parser_init( &ctx->parserState, outputIndex);
		blake2b_256_init(& ctx->txHashCtx);
		ctx->initializedMagic = ATTEST_INIT_MAGIC;

		// Skip outputIndex
		processNextChunk(dataBuffer + 4, dataSize - 4);
	} else if (p1 == P1_CONTINUE) {
		if (ctx->initializedMagic != ATTEST_INIT_MAGIC) {
			THROW(ERR_INVALID_STATE);
		}
		processNextChunk(dataBuffer, dataSize);
	} else {
		ASSERT(false);
	}

}
