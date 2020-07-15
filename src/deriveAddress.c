#include "common.h"
#include "deriveAddress.h"
#include "keyDerivation.h"
#include "endian.h"
#include "state.h"
#include "securityPolicy.h"
#include "uiHelpers.h"
#include "addressUtilsByron.h"
#include "addressUtilsShelley.h"
#include "base58.h"
#include "bech32.h"
#include "bufView.h"
#include "hex_utils.h"

static uint16_t RESPONSE_READY_MAGIC = 11223;

static ins_derive_address_context_t* ctx = &(instructionState.deriveAddressContext);

enum {
	P1_RETURN  = 0x01,
	P1_DISPLAY = 0x02,
};

static void prepareResponse()
{
	ctx->address.size = deriveAddress(&ctx->addressParams, ctx->address.buffer, SIZEOF(ctx->address.buffer));
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;
}


static const char STAKING_HEADING_PATH[]    = "Staking key path: ";
static const char STAKING_HEADING_HASH[]    = "Staking key hash: ";
static const char STAKING_HEADING_POINTER[] = "Staking key pointer: ";
static const char STAKING_HEADING_WARNING[] = "WARNING: ";

static void ui_displayStakingInfo(addressParams_t* addressParams, ui_callback_fn_t callback)
{
	const char *heading = NULL;
	char stakingInfo[120];
	os_memset(stakingInfo, 0, SIZEOF(stakingInfo));

	switch (addressParams->stakingChoice) {

	case NO_STAKING:
		if (addressParams->type == ENTERPRISE) {
			heading = STAKING_HEADING_WARNING;
			strncpy(stakingInfo, "no staking rewards", SIZEOF(stakingInfo));

		} else if (addressParams->type == BYRON) {
			heading = STAKING_HEADING_WARNING;
			strncpy(stakingInfo, "legacy Byron address (no staking rewards)", SIZEOF(stakingInfo));

		} else {
			ASSERT(false);
		}
		break;

	case STAKING_KEY_PATH:
		heading = STAKING_HEADING_PATH;
		// TODO avoid displaying anything if staking key belongs to spending account?
		bip44_printToStr(&addressParams->stakingKeyPath, stakingInfo, SIZEOF(stakingInfo));
		break;

	case STAKING_KEY_HASH:
		heading = STAKING_HEADING_HASH;
		encode_hex(
		        addressParams->stakingKeyHash, SIZEOF(addressParams->stakingKeyHash),
		        stakingInfo, SIZEOF(stakingInfo)
		);
		break;

	case BLOCKCHAIN_POINTER:
		heading = STAKING_HEADING_POINTER;
		printBlockchainPointerToStr(addressParams->stakingKeyBlockchainPointer, stakingInfo, SIZEOF(stakingInfo));
		break;

	default:
		ASSERT(false);
	}

	ASSERT(heading != NULL);
	ASSERT(strlen(stakingInfo) > 0);

	ui_displayPaginatedText(
	        heading,
	        stakingInfo,
	        callback
	);
}


static void deriveAddress_return_ui_runStep();
enum {
	RETURN_UI_STEP_WARNING = 100,
	RETURN_UI_STEP_SPENDING_PATH,
	RETURN_UI_STEP_STAKING_INFO,
	RETURN_UI_STEP_CONFIRM,
	RETURN_UI_STEP_RESPOND,
	RETURN_UI_STEP_INVALID,
};

static void deriveAddress_handleReturn()
{
	// Check security policy
	security_policy_t policy = policyForReturnDeriveAddress(&ctx->addressParams);
	ENSURE_NOT_DENIED(policy);

	prepareResponse();

	switch (policy) {
#	define  CASE(POLICY, STEP) case POLICY: {ctx->ui_step=STEP; break;}
		CASE(POLICY_PROMPT_WARN_UNUSUAL,    RETURN_UI_STEP_WARNING);
		CASE(POLICY_PROMPT_BEFORE_RESPONSE, RETURN_UI_STEP_SPENDING_PATH);
		CASE(POLICY_ALLOW_WITHOUT_PROMPT,   RETURN_UI_STEP_RESPOND);
#	undef   CASE
	default:
		THROW(ERR_NOT_IMPLEMENTED);
	}
	deriveAddress_return_ui_runStep();
}

static void deriveAddress_return_ui_runStep()
{
	TRACE("step %d\n", ctx->ui_step);
	ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);
	ui_callback_fn_t* this_fn = deriveAddress_return_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(RETURN_UI_STEP_WARNING) {
		ui_displayPaginatedText(
		        "Unusual request",
		        "Proceed with care",
		        this_fn
		);
	}
	UI_STEP(RETURN_UI_STEP_SPENDING_PATH) {
		char pathStr[100];
		{
			const char* prefix = "Path: ";
			size_t len = strlen(prefix);
			os_memcpy(pathStr, prefix, len); // Note: not null-terminated yet
			bip44_printToStr(&ctx->addressParams.spendingKeyPath, pathStr + len, SIZEOF(pathStr) - len);
		}
		ui_displayPaginatedText(
		        "Export address",
		        pathStr,
		        this_fn
		);
	}
	UI_STEP(RETURN_UI_STEP_STAKING_INFO) {
		ui_displayStakingInfo(&ctx->addressParams, this_fn);
	}
	UI_STEP(RETURN_UI_STEP_CONFIRM) {
		ui_displayPrompt(
		        "Confirm",
		        "export address?",
		        this_fn,
		        respond_with_user_reject
		);
	}
	UI_STEP(RETURN_UI_STEP_RESPOND) {
		ctx->responseReadyMagic = 0;
		ASSERT(ctx->address.size <= SIZEOF(ctx->address.buffer));

		io_send_buf(SUCCESS, ctx->address.buffer, ctx->address.size);
		ui_idle();
	}
	UI_STEP_END(RETURN_UI_STEP_INVALID);

}


static void deriveAddress_display_ui_runStep();
enum {
	DISPLAY_UI_STEP_WARNING = 200,
	DISPLAY_UI_STEP_INSTRUCTIONS,
	DISPLAY_UI_STEP_PATH,
	DISPLAY_UI_STEP_STAKING_INFO,
	DISPLAY_UI_STEP_ADDRESS,
	DISPLAY_UI_STEP_RESPOND,
	DISPLAY_UI_STEP_INVALID
};


static void deriveAddress_handleDisplay()
{
	// Check security policy
	security_policy_t policy = policyForShowDeriveAddress(&ctx->addressParams);
	ENSURE_NOT_DENIED(policy);

	prepareResponse();

	switch (policy) {
#	define  CASE(policy, step) case policy: {ctx->ui_step=step; break;}
		CASE(POLICY_PROMPT_WARN_UNUSUAL,  DISPLAY_UI_STEP_WARNING);
		CASE(POLICY_SHOW_BEFORE_RESPONSE, DISPLAY_UI_STEP_INSTRUCTIONS);
#	undef   CASE
	default:
		THROW(ERR_NOT_IMPLEMENTED);
	}
	deriveAddress_display_ui_runStep();
}

static void deriveAddress_display_ui_runStep()
{
	ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);
	ui_callback_fn_t* this_fn = deriveAddress_display_ui_runStep;

	UI_STEP_BEGIN(ctx->ui_step);

	UI_STEP(DISPLAY_UI_STEP_WARNING) {
		ui_displayPaginatedText(
		        "Unusual request",
		        "Proceed with care",
		        this_fn
		);
	}
	UI_STEP(DISPLAY_UI_STEP_INSTRUCTIONS) {
		ui_displayPaginatedText(
		        "Verify address",
		        "Make sure it agrees with your computer",
		        this_fn
		);
	}
	UI_STEP(DISPLAY_UI_STEP_PATH) {
		// Response
		char pathStr[100];
		bip44_printToStr(&ctx->addressParams.spendingKeyPath, pathStr, SIZEOF(pathStr));
		ui_displayPaginatedText(
		        "Address path",
		        pathStr,
		        this_fn
		);
	}
	UI_STEP(RETURN_UI_STEP_STAKING_INFO) {
		ui_displayStakingInfo(&ctx->addressParams, this_fn);
	}
	UI_STEP(DISPLAY_UI_STEP_ADDRESS) {
		char humanAddress[MAX_HUMAN_ADDRESS_LENGTH];
		os_memset(humanAddress, 0, SIZEOF(humanAddress));

		ASSERT(ctx->address.size <= SIZEOF(ctx->address.buffer));
		size_t length = humanReadableAddress(
		                        ctx->address.buffer, ctx->address.size,
		                        humanAddress, SIZEOF(humanAddress)
		                );
		ASSERT(length > 0);
		ASSERT(strlen(humanAddress) == length);

		ui_displayPaginatedText(
		        "Address",
		        humanAddress,
		        this_fn
		);
	}
	UI_STEP(DISPLAY_UI_STEP_RESPOND) {
		io_send_buf(SUCCESS, NULL, 0);
		ui_idle();
	}
	UI_STEP_END(DISPLAY_UI_STEP_INVALID);
}

void deriveAddress_handleAPDU(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireDataBuffer,
        size_t wireDataSize,
        bool isNewCall
)
{
	VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
	TRACE_BUFFER(wireDataBuffer, wireDataSize);

	// Initialize state
	if (isNewCall) {
		os_memset(ctx, 0, SIZEOF(*ctx));
	}
	ctx->responseReadyMagic = 0;

	parseAddressParams(wireDataBuffer, wireDataSize, &ctx->addressParams);

	switch (p1) {
#	define  CASE(P1, HANDLER_FN) case P1: {HANDLER_FN(); break;}
		CASE(P1_RETURN,  deriveAddress_handleReturn);
		CASE(P1_DISPLAY, deriveAddress_handleDisplay);
#	undef  CASE
	default:
		THROW(ERR_INVALID_REQUEST_PARAMETERS);
	}
}
