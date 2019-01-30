#include "common.h"

#include "getExtendedPublicKey.h"
#include "keyDerivation.h"
#include "endian.h"
#include "state.h"
#include "uiHelpers.h"
#include "securityPolicy.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

static ins_get_ext_pubkey_context_t* ctx = &(instructionState.extPubKeyContext);


static int16_t RESPONSE_READY_MAGIC = 12345;

// forward declaration
static void getExtendedPublicKey_ui_runStep();
enum {
	UI_STEP_WARNING = 100,
	UI_STEP_DISPLAY_PATH,
	UI_STEP_CONFIRM,
	UI_STEP_RESPOND,
	UI_STEP_INVALID,
};

void getExtendedPublicKey_handleAPDU(
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

	// Validate params
	VALIDATE_PARAM(p1 == 0);
	VALIDATE_PARAM(p2 == 0);

	// Parse wire
	size_t parsedSize = bip44_parseFromWire(&ctx->pathSpec, wireDataBuffer, wireDataSize);

	if (parsedSize != wireDataSize) {
		THROW(ERR_INVALID_DATA);
	}

	// Check security policy
	security_policy_t policy = policyForGetExtendedPublicKey(&ctx->pathSpec);
	THROW_IF_DENY(policy);

	// Calculation
	deriveExtendedPublicKey(
	        & ctx->pathSpec,
	        & ctx->extPubKey
	);
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	switch (policy) {
	case POLICY_PROMPT_WARN_UNUSUAL: {
		ctx->ui_step = UI_STEP_WARNING;
		break;
	}
	case POLICY_PROMPT_BEFORE_RESPONSE: {
		ctx->ui_step = UI_STEP_DISPLAY_PATH;
		break;
	}
	case POLICY_ALLOW: {
		ctx->ui_step = UI_STEP_RESPOND;
		break;
	}
	default:
		ASSERT(false);
	}
	getExtendedPublicKey_ui_runStep();
}

static void getExtendedPublicKey_ui_runStep()
{
	ui_callback_fn_t* this_fn = getExtendedPublicKey_ui_runStep;
	int nextStep = UI_STEP_INVALID;

	switch (ctx->ui_step) {
	case UI_STEP_WARNING: {
		ui_displayScrollingText(
		        "Unusual request",
		        "Proceed with care",
		        this_fn
		);
		nextStep  = UI_STEP_DISPLAY_PATH;
		break;
	}
	case UI_STEP_DISPLAY_PATH: {
		// Response
		char pathStr[100];
		bip44_printToStr(&ctx->pathSpec, pathStr, SIZEOF(pathStr) );

		ui_displayScrollingText(
		        "Export public key",
		        pathStr,
		        this_fn
		);
		nextStep = UI_STEP_CONFIRM;
		break;
	}
	case UI_STEP_CONFIRM: {
		ui_displayConfirm(
		        "Confirm export",
		        "public key?",
		        this_fn,
		        respond_with_user_reject
		);
		nextStep = UI_STEP_RESPOND;
		break;
	}
	case UI_STEP_RESPOND: {
		ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);

		io_send_buf(SUCCESS, (uint8_t*) &ctx->extPubKey, SIZEOF(ctx->extPubKey));
		ui_idle();
		nextStep = UI_STEP_INVALID;
		break;
	}
	default:
		ASSERT(false);
	}
	ctx->ui_step = nextStep;
}
