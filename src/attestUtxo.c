#include "attestUtxo.h"
#include "assert.h"
#include <os.h>
#include "cbor.h"
#include "test_utils.h"
#include "endian.h"
#include "io.h"
#include "ux.h"
#include "utils.h"
#include "hash.h"
#include "hmac.h"
#include "attestKey.h"

const uint64_t CARDANO_ADDRESS_TYPE_PUBKEY = 0;

// We do not parse addresses, just keep streaming over then
void skipThroughAddressBytes(attestUtxoState_t* state)
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
void advanceInputState(attestUtxoState_t* state)
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
		cbor_takeTokenWithValue(stream, CBOR_TYPE_UNSIGNED, CARDANO_ADDRESS_TYPE_PUBKEY);
		state->inputState = INPUT_EXPECT_TAG;
		break;

	case INPUT_EXPECT_TAG:
		cbor_takeTokenWithValue(stream, CBOR_TYPE_TAG, 24);
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
			skipThroughAddressBytes(state);
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
void advanceOutputState(attestUtxoState_t* state)
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
		cbor_takeTokenWithValue(stream, CBOR_TYPE_TAG, 24);
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
			skipThroughAddressBytes(state);
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

void initInputParser(attestUtxoState_t *state)
{
	MEMCLEAR(& state->inputState, txInputDecoderState_t);
	state->inputState = INPUT_PARSING_NOT_STARTED;
}

void initOutputParser(attestUtxoState_t *state)
{
	MEMCLEAR(& state->outputState, txOutputDecoderState_t);
	state->outputState = OUTPUT_PARSING_NOT_STARTED;
}

// Makes one parsing step or throws ERR_NOT_ENOUGH_INPUT
void advanceMainState(attestUtxoState_t *state)
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
			initInputParser(state);
			state->mainState = MAIN_IN_INPUT;
		}
		break;

	case MAIN_IN_INPUT:
		if (state->inputState == INPUT_FINISHED) {
			state->mainState = MAIN_EXPECT_END_OR_INPUT;
		} else {
			advanceInputState(state);
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
			state->mainState = MAIN_IN_OUTPUT;
			initOutputParser(state);
		}
		break;

	case MAIN_IN_OUTPUT:
		if (state->outputState == OUTPUT_FINISHED) {
			state->mainState = MAIN_EXPECT_END_OR_OUTPUT;
			state->currentOutputIndex += 1;
		} else {
			advanceOutputState(state);
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

void initAttestUtxo(
        attestUtxoState_t* state,
        int outputIndex
)
{
	MEMCLEAR(state, attestUtxoState_t);
	state->mainState = MAIN_PARSING_NOT_STARTED;
	state->outputAmount = LOVELACE_INVALID;
	state->attestedOutputIndex = outputIndex;
	stream_init(& state->stream);
	state->isInitialized = ATTEST_INIT_MAGIC;
	blake2b_256_init(& state->txHashCtx);
}

// TODO(ppershing): revisit these conditions
uint64_t getAttestedAmount(attestUtxoState_t* state)
{
	if (state->mainState != MAIN_FINISHED) return LOVELACE_INVALID;
	if (state->currentOutputIndex <= state->attestedOutputIndex) return LOVELACE_INVALID;
	if (state->outputAmount > LOVELACE_MAX_SUPPLY) return LOVELACE_INVALID;
	return state->outputAmount;
}

bool isInitialized(attestUtxoState_t* state)
{
	return state->isInitialized == ATTEST_INIT_MAGIC;
}

// throws ERR_NOT_ENOUGH_INPUT when cannot proceed further
void keepParsing(attestUtxoState_t* state)
{
	ASSERT(isInitialized(state));

	while(state -> mainState != MAIN_FINISHED) {
		advanceMainState(state);
	}
}


// TODO: move this to the global state union
attestUtxoState_t global_state;

enum {
	P1_INITIAL = 0x01,
	P1_CONTINUE = 0x02,
};


void attestUtxo_sendResponse(attestUtxoState_t* state)
{
	// Response is (txHash, outputNumber, outputAmount, HMAC)
	uint8_t responseBuffer[32 + 4 + 8 + 16]; // Note: we have short HMAC
	uint8_t hmacBuffer[32]; // Full HMAC

	size_t pos = 0;

	// txHash
	blake2b_256_finalize(&state ->txHashCtx, responseBuffer + pos, 32);
	pos += 32;

	// outputNumber
	u4be_write(responseBuffer + pos, state->attestedOutputIndex);
	pos += 4;

	// outputAmount
	uint64_t amount = getAttestedAmount(state);
	if (amount == LOVELACE_INVALID) {
		THROW(ERR_INVALID_DATA);
	}
	u8be_write(responseBuffer + pos, amount);
	pos += 8;

	// HMAC
	hmac_sha256(
	        attestKeyData.key, SIZEOF(attestKeyData.key),
	        responseBuffer, pos, // all of response so far
	        hmacBuffer, 32
	);
	os_memmove(responseBuffer + pos, hmacBuffer, 16);
	pos += 16;

	ASSERT(pos == SIZEOF(responseBuffer));
	io_send_buf(SUCCESS, responseBuffer, pos);
}

void handle_attestUtxo(
        uint8_t p1,
        uint8_t p2,
        uint8_t* dataBuffer,
        uint16_t dataLength
)
{
	attestUtxoState_t* state = &global_state;

	VALIDATE_REQUEST_PARAM(p1 == P1_INITIAL || p1 == P1_CONTINUE);

	if (p1 == P1_INITIAL) {
		VALIDATE_REQUEST_PARAM(p2 == 0);
		VALIDATE_REQUEST_PARAM(dataLength >= 4);
		uint32_t outputIndex = u4be_read(dataBuffer);
		initAttestUtxo(state, outputIndex);
		// Skip outputIndex
		dataBuffer += 4;
		dataLength -= 4;
	} else {
		if (!isInitialized(state)) {
			THROW(ERR_INVALID_STATE);
		}
	}

	BEGIN_TRY {
		TRY {
			stream_appendData(&state->stream, dataBuffer, dataLength);
			blake2b_256_append(& state->txHashCtx, dataBuffer, dataLength);
			keepParsing(state);
			ASSERT(state->mainState == MAIN_FINISHED);
			attestUtxo_sendResponse(state);
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
