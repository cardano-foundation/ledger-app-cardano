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
#include "uiHelpers.h"

static void parser_advanceMainState(attest_utxo_parser_state_t *state);


static ins_attest_utxo_context_t* ctx = &(instructionState.attestUtxoContext);
static const uint16_t ATTEST_INIT_MAGIC = 4547;
static const uint16_t ATTEST_PARSER_INIT_MAGIC = 4647;

static inline uint64_t uint64_min(uint64_t x, uint64_t y)
{
	return (x < y) ? x : y;
}


// We do not parse addresses, just keep streaming over then
static void parser_skipThroughAddressBytes(attest_utxo_parser_state_t* state)
{
	stream_t* stream = &(state->stream); // shorthand
	stream_ensureAvailableBytes(stream, 1); // We have to consume at least something
	uint64_t bytesToConsume =
	        uint64_min(
	                stream_availableBytes(stream),
	                state->addressDataRemainingBytes
	        );
	ASSERT(bytesToConsume < BUFFER_SIZE_PARANOIA);
	stream_advancePos(stream, (size_t) bytesToConsume);
	state->addressDataRemainingBytes -= (size_t) bytesToConsume;
}

// Performs a single transition on the Input parser
static void parser_advanceInputState(attest_utxo_parser_state_t* state)
{
	stream_t* stream = &(state->stream); // shorthand

// Note(ppershing): Foolowing macros are ugly and break about any sane macro rule
// We use them to make the rest of the code way more clearer than it would be otherwise...

// Warning: There is one important rule -- code between TRANSITION_TO *must* be atomic
// if throwing ERR_NOT_ENOUGH_INPUT (or manage its side-effect manually).
// If the code wants to yield before transition, it can use break/return statement
// Otherwise it implicitly yields at every transition

// Note(ppershing): As strange as it seems, switch cases *can* be mixed inside scopes.
// See https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
// Here we use scopes to layout CBOR parsing level

// Sidenote: PARSER_BEGIN && PARSER_END have unmatched braces. This is a intended as
// it forces reasonable indentation
#define PARSER_BEGIN switch (state->inputState) { case INPUT_PARSING_NOT_STARTED:
#define PARSER_END   default: ASSERT(false); }
#define TRANSITION_TO(NEXT_STATE) state->inputState = NEXT_STATE; break; case NEXT_STATE:

	// Array(2)[
	//    Unsigned[0],
	//    Tag(24):Bytes[utxo cbor]
	// ]
	PARSER_BEGIN;

	TRANSITION_TO(INPUT_EXPECT_PREAMBLE);

	cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 2);
	{
		TRANSITION_TO(INPUT_EXPECT_TYPE);

		cbor_takeTokenWithValue(stream, CBOR_TYPE_UNSIGNED, CARDANO_INPUT_TYPE_UTXO);
	}
	{
		TRANSITION_TO(INPUT_EXPECT_TAG);

		cbor_takeTokenWithValue(stream, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);

		TRANSITION_TO(INPUT_EXPECT_ADDRESS_DATA_PREAMBLE);

		state->addressDataRemainingBytes = cbor_takeToken(stream, CBOR_TYPE_BYTES);

		TRANSITION_TO(INPUT_IN_ADDRESS_DATA);

		if (state->addressDataRemainingBytes != 0) {
			parser_skipThroughAddressBytes(state);
			// Warning: this break has to be here as we might not skip through
			// all bytes in one go
			break;
		}

	}
	TRANSITION_TO(INPUT_FINISHED);
	{
		// should be handled by the caller
		ASSERT(false);
	}

	PARSER_END;

#undef  PARSER_BEGIN
#undef  PARSER_END
#undef  TRANSITION_TO
}

// Makes one step on the transaction Output parser
static void parser_advanceOutputState(attest_utxo_parser_state_t* state)
{
	stream_t* stream = &state->stream;

// Note(ppershing): see note above for parser macros
#define PARSER_BEGIN switch (state->outputState) { case OUTPUT_PARSING_NOT_STARTED:
#define PARSER_END   default: ASSERT(false); }
#define TRANSITION_TO(NEXT_STATE) state->outputState = NEXT_STATE; break; case NEXT_STATE:

	// Array(2)[
	//   Array(2)[
	//      Tag(24):Bytes[raw address],
	//      Unsigned[checksum]
	//   ],
	//   Unsigned[amount]
	// ]
	PARSER_BEGIN;

	TRANSITION_TO(OUTPUT_EXPECT_PREAMBLE);

	cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 2);

	{
		TRANSITION_TO(OUTPUT_EXPECT_ADDRESS_PREAMBLE);

		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 2);

		{
			TRANSITION_TO(OUTPUT_EXPECT_ADDRESS_TAG);

			cbor_takeTokenWithValue(stream, CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);

			TRANSITION_TO(OUTPUT_EXPECT_ADDRESS_DATA_PREAMBLE);

			state->addressDataRemainingBytes = cbor_takeToken(stream, CBOR_TYPE_BYTES);

			TRANSITION_TO(OUTPUT_IN_ADDRESS_DATA);

			if (state->addressDataRemainingBytes != 0) {
				parser_skipThroughAddressBytes(state);
				// see remark on input parser
				break;
			}
		}
		{
			TRANSITION_TO(OUTPUT_EXPECT_ADDRESS_CHECKSUM);

			cbor_takeToken(stream, CBOR_TYPE_UNSIGNED);

		}
	}
	{
		TRANSITION_TO(OUTPUT_EXPECT_AMOUNT);

		uint64_t value = cbor_takeToken(stream, CBOR_TYPE_UNSIGNED);
		if (state->currentOutputIndex == state->attestedOutputIndex) {
			state->outputAmount = value;
		}

	}
	TRANSITION_TO(OUTPUT_FINISHED);

	// this should be handled by the caller
	ASSERT(false);


	PARSER_END;

#undef  PARSER_BEGIN
#undef  PARSER_END
#undef  TRANSITION_TO
}

// Makes one parsing step or throws ERR_NOT_ENOUGH_INPUT
static void parser_advanceMainState(attest_utxo_parser_state_t *state)
{
	stream_t* stream = &(state->stream); // shorthand
// Note(ppershing): see note above for parser macros
#define PARSER_BEGIN switch (state->mainState) { case MAIN_PARSING_NOT_STARTED:
#define PARSER_END   default: ASSERT(false); }
#define TRANSITION_TO(NEXT_STATE) state->mainState = NEXT_STATE; break; case NEXT_STATE:
#define JUMP(NEXT_STATE) state->mainState = NEXT_STATE; break;

	PARSER_BEGIN;

	TRANSITION_TO(MAIN_EXPECT_TX_PREAMBLE);

	cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY, 3);
	{
		TRANSITION_TO(MAIN_EXPECT_INPUTS_PREAMBLE);

		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY_INDEF, 0);
		{
			TRANSITION_TO(MAIN_EXPECT_END_OR_INPUT);

			if (cbor_peekNextIsIndefEnd(stream)) {
				cbor_takeTokenWithValue(stream, CBOR_TYPE_INDEF_END, 0);
				JUMP(MAIN_EXPECT_OUTPUTS_PREAMBLE);
			}

			memset(&state->inputState, 0, SIZEOF(state->inputState));
			state->inputState = INPUT_PARSING_NOT_STARTED;

			TRANSITION_TO(MAIN_IN_INPUT);

			if (state->inputState != INPUT_FINISHED) {
				parser_advanceInputState(state);
				break;
			}

			JUMP(MAIN_EXPECT_END_OR_INPUT);
		}
	}
	{
		TRANSITION_TO(MAIN_EXPECT_OUTPUTS_PREAMBLE);

		cbor_takeTokenWithValue(stream, CBOR_TYPE_ARRAY_INDEF, 0);
		state->currentOutputIndex = 0;

		{
			TRANSITION_TO(MAIN_EXPECT_END_OR_OUTPUT);

			if (cbor_peekNextIsIndefEnd(stream)) {
				cbor_takeTokenWithValue(stream, CBOR_TYPE_INDEF_END, 0);
				JUMP(MAIN_EXPECT_METADATA_PREAMBLE);
			}

			MEMCLEAR(& state->outputState, txOutputDecoderState_t);
			state->outputState = OUTPUT_PARSING_NOT_STARTED;

			TRANSITION_TO(MAIN_IN_OUTPUT);

			if (state->outputState != OUTPUT_FINISHED) {
				parser_advanceOutputState(state);
				break;
			}

			state->currentOutputIndex += 1;
			JUMP(MAIN_EXPECT_END_OR_OUTPUT);
		}
	}
	{
		TRANSITION_TO(MAIN_EXPECT_METADATA_PREAMBLE)

		cbor_takeTokenWithValue(stream, CBOR_TYPE_MAP, 0);
	}
	TRANSITION_TO(MAIN_FINISHED)

	// should be handled separately by the outer loop
	ASSERT(false);

	PARSER_END;

}

void parser_init(
        attest_utxo_parser_state_t* state,
        uint32_t outputIndex
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

// throws ERR_NOT_ENOUGH_INPUT when cannot proceed further
void parser_keepParsing(attest_utxo_parser_state_t* state)
{
	ASSERT(state->parserInitializedMagic == ATTEST_PARSER_INIT_MAGIC);

	while(state -> mainState != MAIN_FINISHED) {
		TRACE();
		parser_advanceMainState(state);
	}
	TRACE();
}



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
	} wireResponse;

	STATIC_ASSERT(SIZEOF(wireResponse) == 32 + 4 + 8 + 16, "response is packed");

	{
		blake2b_256_finalize(
		        &ctx->txHashCtx,
		        wireResponse.data.txHash, SIZEOF(wireResponse.data.txHash)
		);

		u4be_write(wireResponse.data.index, ctx->parserState.attestedOutputIndex);

		u8be_write(wireResponse.data.amount, amount);

		attest_writeHmac(
		        ATTEST_PURPOSE_BIND_UTXO_AMOUNT,
		        (uint8_t*) &wireResponse.data, SIZEOF(wireResponse.data),
		        wireResponse.hmac, SIZEOF(wireResponse.hmac)
		);
	}

	io_send_buf(SUCCESS, (uint8_t*) &wireResponse, SIZEOF(wireResponse));
}



void attestUtxo_handleInitAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	// Note(ppershing): If this is ever implemented, it probably
	// should be moved to handleDataAPDU
	security_policy_t policy = policyForAttestUtxo();
	if (policy != POLICY_ALLOW_WITHOUT_PROMPT) {
		THROW(ERR_NOT_IMPLEMENTED);
	}

	VALIDATE(wireDataSize == 4, ERR_INVALID_DATA);
	uint32_t outputIndex = u4be_read(wireDataBuffer);

	{
		// initializations
		parser_init(&ctx->parserState, outputIndex);
		blake2b_256_init(&ctx->txHashCtx);
		ctx->initializedMagic = ATTEST_INIT_MAGIC;
	}

	TRACE("io");
	io_send_buf(SUCCESS, NULL, 0);
	TRACE("done");
	ui_displayBusy();
};

void attestUtxo_handleDataAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	ASSERT(ctx->initializedMagic == ATTEST_INIT_MAGIC);
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	BEGIN_TRY {
		TRY {
			TRACE();
			stream_appendData(&ctx->parserState.stream, wireDataBuffer, wireDataSize);
			TRACE();
			blake2b_256_append(&ctx->txHashCtx, wireDataBuffer, wireDataSize);
			TRACE();
			parser_keepParsing(&ctx->parserState);

			TRACE();
			ASSERT(ctx->parserState.mainState == MAIN_FINISHED);
			// We should not have any data left
			VALIDATE(stream_availableBytes(&ctx->parserState.stream) == 0, ERR_INVALID_DATA);
			attestUtxo_sendResponse();
			ui_idle();
		}
		CATCH(ERR_NOT_ENOUGH_INPUT)
		{
			// Respond that we need more data
			TRACE("io");
			io_send_buf(SUCCESS, NULL, 0);
			TRACE("done");
			// Note(ppershing): no ui_idle() as we continue exchange...
		}
		CATCH(ERR_UNEXPECTED_TOKEN)
		{
			TRACE();
			// Convert to ERR_INVALID_DATA
			THROW(ERR_INVALID_DATA);
		}
		FINALLY {
		}
	} END_TRY;
}




void attestUTxO_handleAPDU(
        uint8_t p1,
        uint8_t p2,
        uint8_t* wireDataBuffer,
        size_t wireDataSize,
        bool isNewCall
)
{
	TRACE("P1: %d", (int) p1);
	enum {
		P1_INIT = 0x01,
		P1_DATA = 0x02,
	};

	VALIDATE(isNewCall == (p1 == P1_INIT), ERR_INVALID_STATE);

	if (isNewCall) {
		os_memset(ctx, 0, SIZEOF(*ctx));
	}

	switch(p1) {
#	define  CASE(P1, HANDLER) case P1: {HANDLER(p2, wireDataBuffer, wireDataSize); break; }
		CASE(P1_INIT, attestUtxo_handleInitAPDU);
		CASE(P1_DATA, attestUtxo_handleDataAPDU);
#	undef   CASE
	default:
		THROW(ERR_INVALID_REQUEST_PARAMETERS);
	}
}
