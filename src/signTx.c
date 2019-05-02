#include "common.h"
#include "signTx.h"
#include "cbor.h"
#include "state.h"
#include "cardano.h"
#include "keyDerivation.h"
#include "ux.h"
#include "attestKey.h"
#include "endian.h"
#include "addressUtils.h"
#include "uiHelpers.h"
#include "crc32.h"
#include "txHashBuilder.h"
#include "textUtils.h"
#include "base58.h"
#include "messageSigning.h"
#include "bip44.h"
#include "bufView.h"

enum {
	SIGN_TX_INPUT_TYPE_UTXO = 1,
	/* Not supported for now but maybe in the future...
	SIGN_TX_INPUT_TYPE_TXHASH = 2,
	*/
};

enum {
	SIGN_TX_OUTPUT_TYPE_ADDRESS = 1,
	SIGN_TX_OUTPUT_TYPE_PATH = 2,
};



static ins_sign_tx_context_t* ctx = &(instructionState.signTxContext);

static inline void CHECK_STAGE(sign_tx_stage_t expected)
{
	VALIDATE(ctx->stage == expected, ERR_INVALID_STATE);
}

static void signTx_handleInit_ui_runStep();

enum {
	HANDLE_INIT_STEP_CONFIRM = 100,
	HANDLE_INIT_STEP_RESPOND,
	HANDLE_INIT_STEP_INVALID,
} ;

static void signTx_handleInitAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE();

	CHECK_STAGE(SIGN_STAGE_NONE);
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	txHashBuilder_init(&ctx->txHashBuilder);

	ctx->currentInput = 0;
	ctx->currentOutput = 0;
	ctx->currentWitness = 0;

	ctx->sumAmountInputs = 0;
	ctx->sumAmountOutputs = 0;


	struct {
		uint8_t numInputs[4];
		uint8_t numOutputs[4];
	}* wireHeader = (void*) wireDataBuffer;

	VALIDATE(SIZEOF(*wireHeader) == wireDataSize, ERR_INVALID_DATA);

	ASSERT_TYPE(ctx->numInputs, uint16_t);
	ctx->numInputs    = (uint16_t) u4be_read(wireHeader->numInputs);
	ctx->numOutputs   = (uint16_t) u4be_read(wireHeader->numOutputs);

	VALIDATE(ctx->numInputs < SIGN_MAX_INPUTS, ERR_INVALID_DATA);
	VALIDATE(ctx->numOutputs < SIGN_MAX_OUTPUTS, ERR_INVALID_DATA);

	// Note(ppershing): current code design assumes at least one input
	// and at least one output. If this has to be relaxed, stage switching
	// logic needs to be re-visited
	VALIDATE(ctx->numInputs > 0, ERR_INVALID_DATA);
	VALIDATE(ctx->numOutputs > 0, ERR_INVALID_DATA);

	// Note(ppershing): right now we assume that we want to sign whole transaction
	ctx->numWitnesses = ctx->numInputs;

	// Note(ppershing): do not allow more witnesses than inputs. This
	// tries to lessen potential pubkey privacy leaks because
	// in WITNESS stage we do not verify whether the witness belongs
	// to a given utxo.
	VALIDATE(ctx->numWitnesses <= ctx->numInputs, ERR_INVALID_DATA);

	security_policy_t policy = policyForSignTxInit();
	switch (policy) {
#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
		CASE(POLICY_PROMPT_BEFORE_RESPONSE, HANDLE_INIT_STEP_CONFIRM);
		CASE(POLICY_ALLOW_WITHOUT_PROMPT,   HANDLE_INIT_STEP_RESPOND);
#	undef   CASE
	default:
		THROW(ERR_NOT_IMPLEMENTED);
	}
	signTx_handleInit_ui_runStep();
}

// Simple for now
static void signTx_handleInit_ui_runStep()
{
	TRACE("Step %d", ctx->ui_step);
	ui_callback_fn_t* this_fn = signTx_handleInit_ui_runStep;
	int nextStep = HANDLE_INIT_STEP_INVALID;

	switch(ctx->ui_step) {
	case HANDLE_INIT_STEP_CONFIRM: {
		ui_displayPrompt(
		        "Start new",
		        "transaction?",
		        this_fn,
		        respond_with_user_reject
		);
		nextStep = HANDLE_INIT_STEP_RESPOND;
		break;
	}
	case HANDLE_INIT_STEP_RESPOND: {
		TRACE();
		// advance state to inputs
		txHashBuilder_enterInputs(&ctx->txHashBuilder);
		ctx->stage = SIGN_STAGE_INPUTS;

		// respond
		io_send_buf(SUCCESS, NULL, 0);
		ui_displayBusy(); // needs to happen after I/O
		nextStep = HANDLE_INIT_STEP_INVALID;
		break;
	}
	default:
		ASSERT(false);
	}
	ctx->ui_step = nextStep;
}


static void amountSum_incrementBy(uint64_t* sumPtr, uint64_t amount)
{
	ASSERT(*sumPtr <= LOVELACE_MAX_SUPPLY);
	VALIDATE(*sumPtr + amount <= LOVELACE_MAX_SUPPLY, ERR_INVALID_DATA);
	*sumPtr += amount;
}

static void signTx_handleInput_ui_runStep();

enum {
	HANDLE_INPUT_STEP_RESPOND = 200,
	HANDLE_INPUT_STEP_INVALID,
};

static void signTx_handleInputAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE();
	ASSERT(ctx->currentInput < ctx->numInputs);
	CHECK_STAGE(SIGN_STAGE_INPUTS);

	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(wireDataSize >= 1, ERR_INVALID_DATA);
	uint8_t inputType = wireDataBuffer[0];

	if (inputType == SIGN_TX_INPUT_TYPE_UTXO) {

		struct {
			struct {
				uint8_t txHash[32];
				uint8_t index[4];
				uint8_t amount[8];
			} data;
			uint8_t hmac[16];
		}* wireUtxo = (void*) wireDataBuffer + 1;

		VALIDATE(wireDataSize == 1 + SIZEOF(*wireUtxo), ERR_INVALID_DATA);

		if (!attest_isCorrectHmac(
		            ATTEST_PURPOSE_BIND_UTXO_AMOUNT,
		            (uint8_t*) &wireUtxo->data, SIZEOF(wireUtxo->data),
		            wireUtxo->hmac, SIZEOF(wireUtxo->hmac)
		    )) {
			THROW(ERR_INVALID_DATA);
		}

		uint8_t* txHashBuffer = wireUtxo->data.txHash;
		size_t txHashSize = SIZEOF(wireUtxo->data.txHash);
		uint32_t parsedIndex = u4be_read(wireUtxo->data.index);
		uint64_t parsedAmount = u8be_read(wireUtxo->data.amount);

		// Note(ppershing): Ledger doesn't have uint64_t printing, this is better than nothing
		TRACE("Input amount %u.%06u", (unsigned) (parsedAmount / 1000000), (unsigned) (parsedAmount % 1000000));
		amountSum_incrementBy(&ctx->sumAmountInputs, parsedAmount);

		TRACE("Adding input to tx hash");
		txHashBuilder_addUtxoInput(&ctx->txHashBuilder, txHashBuffer, txHashSize, parsedIndex);
	} else {
		// Unknown type
		THROW(ERR_INVALID_DATA);
	}

	security_policy_t policy = policyForSignTxInput();

	switch (policy) {
#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
		CASE(POLICY_ALLOW_WITHOUT_PROMPT, HANDLE_INPUT_STEP_RESPOND);
#	undef   CASE
	default:
		THROW(ERR_NOT_IMPLEMENTED);
	}
	signTx_handleInput_ui_runStep();
}

static void signTx_handleInput_ui_runStep()
{
	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(HANDLE_INPUT_STEP_RESPOND) {
		// Advance state to next input
		ctx->currentInput++;
		if (ctx->currentInput == ctx->numInputs) {
			txHashBuilder_enterOutputs(&ctx->txHashBuilder);
			ctx->stage = SIGN_STAGE_OUTPUTS;
		}

		// respond
		io_send_buf(SUCCESS, NULL, 0);
		ui_displayBusy(); // needs to happen after I/O
	}
	UI_STEP_END(HANDLE_INPUT_STEP_INVALID);
}


static void signTx_handleOutput_ui_runStep(); // forward declaration

enum {
	HANDLE_OUTPUT_STEP_DISPLAY_AMOUNT = 300,
	HANDLE_OUTPUT_STEP_DISPLAY_ADDRESS,
	HANDLE_OUTPUT_STEP_RESPOND,
	HANDLE_OUTPUT_STEP_INVALID,
};

static void signTx_handleOutputAPDU(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	TRACE();
	ASSERT(ctx->currentOutput < ctx->numOutputs);
	CHECK_STAGE(SIGN_STAGE_OUTPUTS);
	ASSERT(wireDataSize < BUFFER_SIZE_PARANOIA);

	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	read_view_t view = make_read_view(wireDataBuffer, wireDataBuffer + wireDataSize);

	// Read data preamble
	uint64_t amount = parse_u8be(&view);
	uint8_t outputType = parse_u1be(&view);
	TRACE("Amount: %u.%06u", (unsigned) (amount / 1000000), (unsigned)(amount % 1000000));

	amountSum_incrementBy(&ctx->sumAmountOutputs, amount);
	ctx->currentAmount = amount;

	uint8_t rawAddressBuffer[200];
	size_t rawAddressSize = 0;

	security_policy_t policy;

	TRACE("Output type %d", (int) outputType);
	switch(outputType) {
	case SIGN_TX_OUTPUT_TYPE_ADDRESS: {
		// Rest of input is all address
		rawAddressSize = unboxChecksummedAddress(
		                         VIEW_REMAINING_TO_TUPLE_BUF_SIZE(&view),
		                         rawAddressBuffer, SIZEOF(rawAddressBuffer)
		                 );

		policy =  policyForSignTxOutputAddress(rawAddressBuffer, rawAddressSize);
		TRACE("Policy: %d", (int) policy);
		ENSURE_NOT_DENIED(policy);
		break;
	}
	case SIGN_TX_OUTPUT_TYPE_PATH: {
		view_skipBytes(&view,
		               bip44_parseFromWire(&ctx->currentPath,
		                                   VIEW_REMAINING_TO_TUPLE_BUF_SIZE(&view)));
		VALIDATE(view_remainingSize(&view) == 0, ERR_INVALID_DATA);

		policy = policyForSignTxOutputPath(&ctx->currentPath);
		TRACE("Policy: %d", (int) policy);
		ENSURE_NOT_DENIED(policy);
		rawAddressSize = deriveRawAddress(
		                         &ctx->currentPath,
		                         rawAddressBuffer, SIZEOF(rawAddressBuffer)
		                 );
		break;
	}
	default:
		THROW(ERR_INVALID_DATA);
	};

	ASSERT(rawAddressSize > 0);
	ASSERT(rawAddressSize < BUFFER_SIZE_PARANOIA);

	txHashBuilder_addOutput(
	        &ctx->txHashBuilder,
	        rawAddressBuffer,
	        rawAddressSize,
	        amount
	);

	ctx->currentAddress.size = cborPackRawAddressWithChecksum(
	                                   rawAddressBuffer, rawAddressSize,
	                                   ctx->currentAddress.buffer,
	                                   SIZEOF(ctx->currentAddress.buffer)
	                           );

#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
#	define  DEFAULT(ERR) default: { THROW(ERR); }
	switch (policy) {
		CASE(POLICY_SHOW_BEFORE_RESPONSE, HANDLE_OUTPUT_STEP_DISPLAY_AMOUNT);
		CASE(POLICY_ALLOW_WITHOUT_PROMPT, HANDLE_OUTPUT_STEP_RESPOND);
		DEFAULT(ERR_NOT_IMPLEMENTED)
	}
#	undef   CASE
#	undef   DEFAULT

	signTx_handleOutput_ui_runStep();
}


static void signTx_handleOutput_ui_runStep()
{
	TRACE("step %d", ctx->ui_step);
	ui_callback_fn_t* this_fn = signTx_handleOutput_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(HANDLE_OUTPUT_STEP_DISPLAY_AMOUNT) {
		char adaAmountStr[50];
		str_formatAdaAmount(adaAmountStr, SIZEOF(adaAmountStr), ctx->currentAmount);
		ui_displayPaginatedText(
		        "Send ADA",
		        adaAmountStr,
		        this_fn
		);
	}
	UI_STEP(HANDLE_OUTPUT_STEP_DISPLAY_ADDRESS) {
		char address58Str[200];
		ASSERT(ctx->currentAddress.size <= SIZEOF(ctx->currentAddress.buffer));
		encode_base58(
		        ctx->currentAddress.buffer,
		        ctx->currentAddress.size,
		        address58Str,
		        SIZEOF(address58Str)
		);

		ui_displayPaginatedText(
		        "To address",
		        address58Str,
		        this_fn
		);
	}
	UI_STEP(HANDLE_OUTPUT_STEP_RESPOND) {
		// Advance state to next output
		ctx->currentOutput++;
		// Transition to outputs
		if (ctx->currentOutput == ctx->numOutputs) {
			txHashBuilder_enterMetadata(&ctx->txHashBuilder);
			ctx->stage = SIGN_STAGE_CONFIRM;
		}

		// respond
		io_send_buf(SUCCESS, NULL, 0);
		ui_displayBusy();
	}
	UI_STEP_END(HANDLE_OUTPUT_STEP_INVALID);
}


static void signTx_handleConfirm_ui_runStep();
enum {
	HANDLE_CONFIRM_STEP_DISPLAY_FEE = 400,
	HANDLE_CONFIRM_STEP_FINAL_CONFIRM,
	HANDLE_CONFIRM_STEP_RESPOND,
	HANDLE_CONFIRM_STEP_INVALID,
};

static void signTx_handleConfirmAPDU(uint8_t p2, uint8_t* dataBuffer MARK_UNUSED, size_t dataSize)
{
	TRACE();
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(dataSize == 0, ERR_INVALID_REQUEST_PARAMETERS);

	CHECK_STAGE(SIGN_STAGE_CONFIRM);
	VALIDATE(ctx->sumAmountInputs > ctx->sumAmountOutputs, ERR_INVALID_DATA);

	txHashBuilder_finalize(
	        &ctx->txHashBuilder,
	        ctx->txHash, SIZEOF(ctx->txHash)
	);

	ctx->currentAmount = ctx->sumAmountInputs - ctx->sumAmountOutputs;

	security_policy_t policy = policyForSignTxFee(ctx->currentAmount);

#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
#	define  DEFAULT(ERR) default: { THROW(ERR); }
	switch (policy) {
		CASE(POLICY_SHOW_BEFORE_RESPONSE, HANDLE_CONFIRM_STEP_DISPLAY_FEE);
		DEFAULT(ERR_NOT_IMPLEMENTED);
	}
#	undef   CASE
#	undef   DEFAULT

	signTx_handleConfirm_ui_runStep();
}


static void signTx_handleConfirm_ui_runStep()
{
	TRACE("step %d", ctx->ui_step);
	ui_callback_fn_t* this_fn = signTx_handleConfirm_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(HANDLE_CONFIRM_STEP_DISPLAY_FEE) {
		char adaAmount[50];
		str_formatAdaAmount(adaAmount, SIZEOF(adaAmount), ctx->currentAmount);
		ui_displayPaginatedText(
		        "Transaction Fee",
		        adaAmount,
		        this_fn
		);
	}
	UI_STEP(HANDLE_CONFIRM_STEP_FINAL_CONFIRM) {
		ui_displayPrompt(
		        "Confirm",
		        "transaction?",
		        this_fn,
		        respond_with_user_reject
		);
	}
	UI_STEP(HANDLE_CONFIRM_STEP_RESPOND) {
		// switch stage
		ctx->stage = SIGN_STAGE_WITNESSES;

		// respond
		io_send_buf(SUCCESS, ctx->txHash, SIZEOF(ctx->txHash));
		ui_displayBusy();
	}
	UI_STEP_END(HANDLE_CONFIRM_STEP_INVALID);
}

static void signTx_handleWitness_ui_runStep();
enum {
	HANDLE_WITNESS_STEP_WARNING = 500,
	HANDLE_WITNESS_STEP_DISPLAY,
	HANDLE_WITNESS_STEP_CONFIRM,
	HANDLE_WITNESS_STEP_RESPOND,
	HANDLE_WITNESS_STEP_INVALID,
};

static void signTx_handleWitnessAPDU(uint8_t p2, uint8_t* dataBuffer, size_t dataSize)
{
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	ASSERT(ctx->currentWitness < ctx->numWitnesses);


	size_t parsedSize = bip44_parseFromWire(&ctx->currentPath, dataBuffer, dataSize);
	VALIDATE(parsedSize == dataSize, ERR_INVALID_DATA);

	security_policy_t policy = policyForSignTxWitness(&ctx->currentPath);
	ENSURE_NOT_DENIED(policy);

	TRACE("getTxWitness");
	getTxWitness(
	        &ctx->currentPath,
	        ctx->txHash, SIZEOF(ctx->txHash),
	        ctx->currentWitnessData, SIZEOF(ctx->currentWitnessData)
	);

	TRACE("policy %d", (int) policy);

#	define  CASE(POLICY, UI_STEP) case POLICY: {ctx->ui_step=UI_STEP; break;}
#	define  DEFAULT(ERR) default: { THROW(ERR); }
	switch (policy) {
		CASE(POLICY_PROMPT_WARN_UNUSUAL,  HANDLE_WITNESS_STEP_WARNING);
		CASE(POLICY_ALLOW_WITHOUT_PROMPT, HANDLE_WITNESS_STEP_RESPOND);
		DEFAULT(ERR_NOT_IMPLEMENTED);
	}
#	undef   CASE
#	undef   DEFAULT

	signTx_handleWitness_ui_runStep();
}

static void signTx_handleWitness_ui_runStep()
{
	TRACE("step %d", ctx->ui_step);
	ui_callback_fn_t* this_fn = signTx_handleWitness_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(HANDLE_WITNESS_STEP_WARNING) {
		ui_displayPaginatedText(
		        "Warning!",
		        "Hosts asks for unusual witness",
		        this_fn
		);
	}
	UI_STEP(HANDLE_WITNESS_STEP_DISPLAY) {
		char pathStr[100];
		bip44_printToStr(&ctx->currentPath, pathStr, SIZEOF(pathStr));
		ui_displayPaginatedText(
		        "Witness path",
		        pathStr,
		        this_fn
		);
	}
	UI_STEP(HANDLE_WITNESS_STEP_CONFIRM) {
		ui_displayPrompt(
		        "Sign using",
		        "this witness?",
		        this_fn,
		        respond_with_user_reject
		);
	}
	UI_STEP(HANDLE_WITNESS_STEP_RESPOND) {
		TRACE("io_send_buf");

		io_send_buf(SUCCESS, ctx->currentWitnessData, SIZEOF(ctx->currentWitnessData));
		ui_displayBusy(); // needs to happen after I/O

		ctx->currentWitness++;
		if (ctx->currentWitness == ctx->numWitnesses) {
			ctx->stage = SIGN_STAGE_NONE;
			// We are finished
			ui_idle();
		}
	}
	UI_STEP_END(HANDLE_INPUT_STEP_INVALID);
}



typedef void subhandler_fn_t(uint8_t p2, uint8_t* dataBuffer, size_t dataSize);

static subhandler_fn_t* lookup_subhandler(uint8_t p1)
{
	switch(p1) {
#	define  CASE(P1, HANDLER) case P1: return HANDLER;
#	define  DEFAULT(HANDLER)  default: return HANDLER;
		CASE(0x01, signTx_handleInitAPDU);
		CASE(0x02, signTx_handleInputAPDU);
		CASE(0x03, signTx_handleOutputAPDU);
		CASE(0x04, signTx_handleConfirmAPDU);
		CASE(0x05, signTx_handleWitnessAPDU);
		DEFAULT(NULL)
#	undef   CASE
#	undef   DEFAULT
	}
}

void signTx_handleAPDU(
        uint8_t p1,
        uint8_t p2,
        uint8_t* wireDataBuffer,
        size_t wireDataSize,
        bool isNewCall
)
{
	if (isNewCall) {
		os_memset(ctx, 0, SIZEOF(*ctx));
		ctx->stage = SIGN_STAGE_NONE;
	}
	subhandler_fn_t* subhandler = lookup_subhandler(p1);
	VALIDATE(subhandler != NULL, ERR_INVALID_REQUEST_PARAMETERS);
	subhandler(p2, wireDataBuffer, wireDataSize);
}
