#include "attestUtxo.h"
#include "assert.h"
#include <os.h>
#include "cbor.h"
#include "test_utils.h"

// Given that memset is root of many problems, a bit of paranoia is good.
// If you don't believe, just check out https://www.viva64.com/en/b/0360/
//
// TL;DR; We want to avoid cases such as:
//
// int[10] x; void fn(int* ptr) { memset(ptr, 0, sizeof(ptr)); }
// int[10][20] x; memset(x, 0, sizeof(x));
// struct_t* ptr; memset(ptr, 0, sizeof(ptr));
// int[10] x; memset(x, 0, 10);
//
// The best way is to provide an expected type and make sure expected and
// inferred type have the same size.
#define MEMCLEAR(ptr, expected_type) \
{ \
    STATIC_ASSERT(sizeof(expected_type) == sizeof(*(ptr)), __safe_memclear); \
    os_memset(ptr, 0, sizeof(expected_type)); \
}

// Expect & consume CBOR token with specific type and value
void takeCborTokenWithValue(stream_t* stream, uint8_t expectedType, uint64_t expectedValue)
{
	const token_t token = cbor_peekToken(stream);
	if (token.type != expectedType || token.value != expectedValue) {
		THROW(ERR_UNEXPECTED_TOKEN);
	}
	cbor_advanceToken(stream);
}

// Expect & consume CBOR token with specific type, return value
uint64_t takeCborToken(stream_t* stream, uint8_t expectedType)
{
	const token_t token = cbor_peekToken(stream);
	if (token.type != expectedType) {
		THROW(ERR_UNEXPECTED_TOKEN);
	}
	cbor_advanceToken(stream);
	return token.value;
}

// Is next CBOR token indefinite array/map end?
bool nextIsIndefEnd(stream_t* stream)
{
	token_t head = cbor_peekToken(stream);
	return (head.type == TYPE_INDEF_END);
}

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
		takeCborTokenWithValue(stream, TYPE_ARRAY, 2);
		state->inputState = INPUT_EXPECT_TYPE;
		break;

	case INPUT_EXPECT_TYPE:
		// type 0 == privkey address
		takeCborTokenWithValue(stream, TYPE_UNSIGNED, 0);
		state->inputState = INPUT_EXPECT_TAG;
		break;

	case INPUT_EXPECT_TAG:
		takeCborTokenWithValue(stream, TYPE_TAG, 24);
		state->inputState = INPUT_EXPECT_ADDRESS_DATA_PREAMBLE;
		break;

	case INPUT_EXPECT_ADDRESS_DATA_PREAMBLE:

		// Warning: Following two lines have to happen
		// exactly in this order as takeCborBytesPreamble might throw
		state->addressDataRemainingBytes = takeCborToken(stream, TYPE_BYTES);

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
		takeCborTokenWithValue(stream, TYPE_ARRAY, 2);
		state->outputState = OUTPUT_EXPECT_ADDRESS_PREAMBLE;
		break;

	case OUTPUT_EXPECT_ADDRESS_PREAMBLE:
		takeCborTokenWithValue(stream, TYPE_ARRAY, 2);
		state->outputState = OUTPUT_EXPECT_ADDRESS_TAG;
		break;

	case OUTPUT_EXPECT_ADDRESS_TAG:
		takeCborTokenWithValue(stream, TYPE_TAG, 24);
		state->outputState = OUTPUT_EXPECT_ADDRESS_DATA_PREAMBLE;
		break;

	case OUTPUT_EXPECT_ADDRESS_DATA_PREAMBLE:
		// Warning: Following two lines have to happen
		// exactly in this order as takeCborBytesPreamble might throw
		state->addressDataRemainingBytes = takeCborToken(stream, TYPE_BYTES);

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
		takeCborToken(stream, TYPE_UNSIGNED);
		state->outputState = OUTPUT_EXPECT_AMOUNT;
		break;

	case OUTPUT_EXPECT_AMOUNT:
		;
		{
			uint64_t value = takeCborToken(stream, TYPE_UNSIGNED);
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
		takeCborTokenWithValue(stream, TYPE_ARRAY, 3);
		state->mainState = MAIN_EXPECT_INPUTS_PREAMBLE;
		break;

	case MAIN_EXPECT_INPUTS_PREAMBLE:
		takeCborTokenWithValue(stream, TYPE_ARRAY_INDEF, 0);
		state->mainState = MAIN_EXPECT_END_OR_INPUT;
		break;

	case MAIN_EXPECT_END_OR_INPUT:
		if (nextIsIndefEnd(stream)) {
			takeCborTokenWithValue(stream, TYPE_INDEF_END, 0);
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
		takeCborTokenWithValue(stream, TYPE_ARRAY_INDEF, 0);
		state->mainState = MAIN_EXPECT_END_OR_OUTPUT;
		state->currentOutputIndex = 0;
		break;

	case MAIN_EXPECT_END_OR_OUTPUT:
		if (nextIsIndefEnd(stream)) {
			takeCborTokenWithValue(stream, TYPE_INDEF_END, 0);
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
		takeCborTokenWithValue(stream, TYPE_MAP, 0);
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
}

// TODO(ppershing): revisit these conditions
uint64_t getAttestedAmount(attestUtxoState_t* state)
{
	if (state->mainState != MAIN_FINISHED) return LOVELACE_INVALID;
	if (state->currentOutputIndex <= state->attestedOutputIndex) return LOVELACE_INVALID;
	if (state->outputAmount > LOVELACE_MAX_SUPPLY) return LOVELACE_INVALID;
	return state->outputAmount;
}

// throws ERR_NOT_ENOUGH_INPUT when cannot proceed further
void keepParsing(attestUtxoState_t* state)
{
	while(state -> mainState != MAIN_FINISHED) {
		advanceMainState(state);
	}
}
