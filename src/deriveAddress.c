#include "common.h"
#include "deriveAddress.h"
#include "keyDerivation.h"
#include "endian.h"
#include "state.h"
#include "securityPolicy.h"
#include "uiHelpers.h"
#include "addressUtils.h"
#include "base58.h"

static int16_t RESPONSE_READY_MAGIC = 11223;

static ins_derive_address_context_t* ctx = &(instructionState.deriveAddressContext);

enum {
	P1_RETURN = 1,
	P1_DISPLAY = 2,
};

// forward declarations

static void deriveAddress_return_ui_runStep();
enum {
	RETURN_UI_STEP_WARNING = 100,
	RETURN_UI_STEP_PATH,
	RETURN_UI_STEP_CONFIRM,
	RETURN_UI_STEP_RESPOND,
	RETURN_UI_STEP_INVALID,
};

void deriveAddress_handleReturn(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	// Parse wire
	size_t parsedSize = bip44_parseFromWire(&ctx->pathSpec, wireDataBuffer, wireDataSize);

	if (parsedSize != wireDataSize) {
		THROW(ERR_INVALID_DATA);
	}

	// Check security policy
	security_policy_t policy = policyForReturnDeriveAddress(&ctx->pathSpec);

	if (policy == POLICY_DENY) {
		THROW(ERR_REJECTED_BY_POLICY);
	}

	ctx->address.size = deriveAddress(
	                            &ctx->pathSpec,
	                            ctx->address.buffer,
	                            SIZEOF(ctx->address.buffer)
	                    );
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	switch (policy) {
	case POLICY_PROMPT_WARN_UNUSUAL: {
		ctx->ui_step = RETURN_UI_STEP_WARNING;
		break;
	}
	case POLICY_PROMPT_BEFORE_RESPONSE: {
		ctx->ui_step = RETURN_UI_STEP_PATH;
		break;
	}
	case POLICY_ALLOW: {
		ctx->ui_step = RETURN_UI_STEP_RESPOND;
		break;
	}
	default:
		THROW(ERR_NOT_IMPLEMENTED);
	}
	deriveAddress_return_ui_runStep();
}

static void deriveAddress_return_ui_runStep()
{
	ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);
	ui_callback_fn_t* this_fn = deriveAddress_return_ui_runStep;
	int nextStep = RETURN_UI_STEP_INVALID;

	switch (ctx->ui_step) {
	case RETURN_UI_STEP_WARNING: {
		ui_displayScrollingText(
		        "Unusual request",
		        "Proceed with care",
		        this_fn
		);
		nextStep = RETURN_UI_STEP_PATH;
		break;
	}
	case RETURN_UI_STEP_PATH: {
		// Response
		char pathStr[100];
		{
			const char* prefix = "Path: ";
			size_t len = strlen(prefix);
			os_memcpy(pathStr, prefix, len); // Note: not null-terminated yet
			bip44_printToStr(&ctx->pathSpec, pathStr + len, SIZEOF(pathStr) - len);
		}
		ui_displayScrollingText(
		        "Export address",
		        pathStr,
		        this_fn
		);
		nextStep = RETURN_UI_STEP_CONFIRM;

		break;
	}
	case RETURN_UI_STEP_CONFIRM: {
		ui_displayConfirm(
		        "Confirm",
		        "export address?",
		        this_fn,
		        respond_with_user_reject
		);
		nextStep = RETURN_UI_STEP_RESPOND;

		break;
	}
	case RETURN_UI_STEP_RESPOND: {
		ctx->responseReadyMagic = 0;
		ASSERT(ctx->address.size <= SIZEOF(ctx->address.buffer));

		io_send_buf(SUCCESS, ctx->address.buffer, ctx->address.size);
		ui_idle();
		nextStep = RETURN_UI_STEP_INVALID;
		break;
	}
	default:
		ASSERT(false);
	}
	ctx->ui_step = nextStep;
}


static void deriveAddress_display_ui_runStep();
enum {
	DISPLAY_UI_STEP_WARNING = 200,
	DISPLAY_UI_STEP_INSTRUCTIONS,
	DISPLAY_UI_STEP_PATH,
	DISPLAY_UI_STEP_ADDRESS,
	DISPLAY_UI_STEP_RESPOND,
	DISPLAY_UI_STEP_INVALID
};


void deriveAddress_handleDisplay(uint8_t p2, uint8_t* wireDataBuffer, size_t wireDataSize)
{
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	// Parse wire
	size_t parsedSize = bip44_parseFromWire(&ctx->pathSpec, wireDataBuffer, wireDataSize);

	if (parsedSize != wireDataSize) {
		THROW(ERR_INVALID_DATA);
	}

	// Check security policy
	security_policy_t policy = policyForShowDeriveAddress(&ctx->pathSpec);

	if (policy == POLICY_DENY) {
		THROW(ERR_REJECTED_BY_POLICY);
	}

	ctx->address.size = deriveAddress(
	                            &ctx->pathSpec,
	                            ctx->address.buffer,
	                            SIZEOF(ctx->address.buffer)
	                    );
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	switch (policy) {
	case POLICY_PROMPT_WARN_UNUSUAL: {
		ctx->ui_step = DISPLAY_UI_STEP_WARNING;
		break;
	}
	case POLICY_SHOW_BEFORE_RESPONSE: {
		ctx->ui_step = DISPLAY_UI_STEP_INSTRUCTIONS;
		break;
	}
	default:
		THROW(ERR_NOT_IMPLEMENTED);
	}
	deriveAddress_display_ui_runStep();
}

static void deriveAddress_display_ui_runStep()
{
	ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);
	ui_callback_fn_t* this_fn = deriveAddress_display_ui_runStep;
	int nextStep = DISPLAY_UI_STEP_INVALID;

	switch (ctx->ui_step) {
	case DISPLAY_UI_STEP_WARNING: {
		ui_displayScrollingText(
		        "Unusual request",
		        "Proceed with care",
		        this_fn
		);
		nextStep = DISPLAY_UI_STEP_INSTRUCTIONS;
		break;
	}
	case DISPLAY_UI_STEP_INSTRUCTIONS: {
		ui_displayScrollingText(
		        "Verify address",
		        "Make sure it agrees with your computer",
		        this_fn
		);
		nextStep = DISPLAY_UI_STEP_PATH;
		break;
	}
	case DISPLAY_UI_STEP_PATH: {
		// Response
		char pathStr[100];
		bip44_printToStr(&ctx->pathSpec, pathStr, SIZEOF(pathStr));
		ui_displayScrollingText(
		        "Address path",
		        pathStr,
		        this_fn
		);
		nextStep = DISPLAY_UI_STEP_ADDRESS;
		break;
	}
	case DISPLAY_UI_STEP_ADDRESS: {
		char address58Str[100];
		ASSERT(ctx->address.size <= SIZEOF(ctx->address.buffer));

		encode_base58(
		        ctx->address.buffer,
		        ctx->address.size,
		        address58Str,
		        SIZEOF(address58Str)
		);
		ui_displayScrollingText(
		        "Address",
		        address58Str,
		        this_fn
		);
		nextStep = DISPLAY_UI_STEP_RESPOND;
		break;
	}
	case DISPLAY_UI_STEP_RESPOND: {
		io_send_buf(SUCCESS, NULL, 0);
		ui_idle();
		nextStep = RETURN_UI_STEP_INVALID;
		break;
	}
	default:
		ASSERT(false);
	}
	ctx->ui_step = nextStep;
}

void deriveAddress_handleAPDU(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireDataBuffer,
        size_t wireDataSize,
        bool isNewCall
)
{
	// Initialize state
	if (isNewCall) {
		os_memset(ctx, 0, SIZEOF(*ctx));
	}
	ctx->responseReadyMagic = 0;
	switch (p1) {
	case P1_RETURN: {
		return deriveAddress_handleReturn(p2, wireDataBuffer, wireDataSize);
	}
	case P1_DISPLAY: {
		return deriveAddress_handleDisplay(p2, wireDataBuffer, wireDataSize);
	}
	default:
		THROW(ERR_INVALID_REQUEST_PARAMETERS);
	}
}
