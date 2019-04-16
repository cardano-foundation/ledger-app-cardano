#include "common.h"

#include "getExtendedPublicKey.h"
#include "keyDerivation.h"
#include "endian.h"
#include "state.h"
#include "uiHelpers.h"
#include "securityPolicy.h"

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
	VALIDATE(p1 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	// Parse wire
	size_t parsedSize = bip44_parseFromWire(&ctx->pathSpec, wireDataBuffer, wireDataSize);

	if (parsedSize != wireDataSize) {
		THROW(ERR_INVALID_DATA);
	}

	// Check security policy
	security_policy_t policy = policyForGetExtendedPublicKey(&ctx->pathSpec);
	ENSURE_NOT_DENIED(policy);

	// Calculation
	deriveExtendedPublicKey(
	        & ctx->pathSpec,
	        & ctx->extPubKey
	);
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	switch (policy) {
#	define  CASE(policy, step) case policy: {ctx->ui_step = step; break;}
		CASE(POLICY_PROMPT_WARN_UNUSUAL,    UI_STEP_WARNING);
		CASE(POLICY_PROMPT_BEFORE_RESPONSE, UI_STEP_DISPLAY_PATH);
		CASE(POLICY_ALLOW_WITHOUT_PROMPT,   UI_STEP_RESPOND);
#	undef   CASE
	default:
		ASSERT(false);
	}
	getExtendedPublicKey_ui_runStep();
}

static void getExtendedPublicKey_ui_runStep()
{
	ui_callback_fn_t* this_fn = getExtendedPublicKey_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(UI_STEP_WARNING) {
		ui_displayPaginatedText(
		        "Unusual request",
		        "Proceed with care",
		        this_fn
		);
	}
	UI_STEP(UI_STEP_DISPLAY_PATH) {
		// Response
		char pathStr[100];
		bip44_printToStr(&ctx->pathSpec, pathStr, SIZEOF(pathStr) );

		ui_displayPaginatedText(
		        "Export public key",
		        pathStr,
		        this_fn
		);
	}
	UI_STEP(UI_STEP_CONFIRM) {
		ui_displayPrompt(
		        "Confirm export",
		        "public key?",
		        this_fn,
		        respond_with_user_reject
		);
	}
	UI_STEP(UI_STEP_RESPOND) {
		ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);

		io_send_buf(SUCCESS, (uint8_t*) &ctx->extPubKey, SIZEOF(ctx->extPubKey));
		ui_idle();

	}
	UI_STEP_END(UI_STEP_INVALID);
}
