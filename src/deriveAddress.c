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

/*
 * The serialization format:
 *
 * address header 1B
 * spending public key derivation path (1B for length + [0-10] x 4B)
 * staking choice 1B
 *     if NO_STAKING:
 *         nothing more
 *     if STAKING_KEY_PATH:
 *         staking public key derivation path (1B for length + [0-10] x 4B)
 *     if STAKING_KEY_HASH:
 *         staking key hash 28B
 *     if BLOCKCHAIN_POINTER:
 *         certificate blockchain pointer 3 x 4B
 *
 * (see also enums in addressUtilsShelley.h)
 */
static void parseAddressParams(uint8_t *wireDataBuffer, size_t wireDataSize)
{
	TRACE();
	shelleyAddressParams_t* params = &ctx->addressParams;

	read_view_t view = make_read_view(wireDataBuffer, wireDataBuffer + wireDataSize);

	// address header
	VALIDATE(view_remainingSize(&view) >= 1, ERR_INVALID_DATA);
	params->header = parse_u1be(&view);
	TRACE("Address header: 0x%x\n", params->header);
	VALIDATE(isSupportedAddressType(params->header), ERR_UNSUPPORTED_ADDRESS_TYPE);

	// spending public key derivation path
	view_skipBytes(&view, bip44_parseFromWire(&params->spendingKeyPath, VIEW_REMAINING_TO_TUPLE_BUF_SIZE(&view)));
	TRACE();
	bip44_PRINTF(&params->stakingKeyPath);

	// staking choice
	VALIDATE(view_remainingSize(&view) >= 1, ERR_INVALID_DATA);
	params->stakingChoice = parse_u1be(&view);
	TRACE("Staking choice: 0x%x\n", params->stakingChoice);
	VALIDATE(isValidStakingChoice(params->stakingChoice), ERR_INVALID_DATA);

	// staking choice determines what to parse next
	switch (params->stakingChoice) {

	case NO_STAKING:
		break;

	case STAKING_KEY_PATH:
		view_skipBytes(&view, bip44_parseFromWire(&params->stakingKeyPath, VIEW_REMAINING_TO_TUPLE_BUF_SIZE(&view)));
		TRACE();
		bip44_PRINTF(&params->stakingKeyPath);
		break;

	case STAKING_KEY_HASH:
		VALIDATE(view_remainingSize(&view) == PUBLIC_KEY_HASH_LENGTH, ERR_INVALID_DATA);
		ASSERT(SIZEOF(params->stakingKeyHash) == PUBLIC_KEY_HASH_LENGTH);
		os_memcpy(params->stakingKeyHash, view.ptr, PUBLIC_KEY_HASH_LENGTH);
		view_skipBytes(&view, PUBLIC_KEY_HASH_LENGTH);
		TRACE();
		break;

	case BLOCKCHAIN_POINTER:
		VALIDATE(view_remainingSize(&view) == 12, ERR_INVALID_DATA);
		params->stakingKeyBlockchainPointer.blockIndex = parse_u4be(&view);
		params->stakingKeyBlockchainPointer.txIndex = parse_u4be(&view);
		params->stakingKeyBlockchainPointer.certificateIndex = parse_u4be(&view);
		TRACE("Staking pointer: [%d, %d, %d]\n", params->stakingKeyBlockchainPointer.blockIndex,
		      params->stakingKeyBlockchainPointer.txIndex, params->stakingKeyBlockchainPointer.certificateIndex);
		break;

	default:
		ASSERT(false);
	}

	VALIDATE(view_remainingSize(&view) == 0, ERR_INVALID_DATA);
}

static void prepareResponse()
{
	ctx->address.size = deriveAddress_shelley(&ctx->addressParams, ctx->address.buffer, SIZEOF(ctx->address.buffer));
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;
}

static void displayStakingInfo(ui_callback_fn_t callback)
{
	const uint8_t addressType = getAddressType(ctx->addressParams.header);
	char stakingInfo[120];

	switch (ctx->addressParams.stakingChoice) {

	case NO_STAKING:
		if (addressType == ENTERPRISE) {
			ui_displayPaginatedText(
			        "WARNING: ",
			        "no staking rewards",
			        callback
			);
		} else if (addressType == BYRON) {
			ui_displayPaginatedText(
			        "WARNING: ",
			        "legacy Byron address (no staking rewards)",
			        callback
			);
		} else {
			ASSERT(false);
		}
		break;

	case STAKING_KEY_PATH:
		// TODO avoid displaying anything if staking key belongs to spending account?
		bip44_printToStr(&ctx->addressParams.stakingKeyPath, stakingInfo, SIZEOF(stakingInfo));
		ui_displayPaginatedText(
		        "Staking key path: ",
		        stakingInfo,
		        callback
		);
		break;

	case STAKING_KEY_HASH:
		encode_hex(
		        ctx->addressParams.stakingKeyHash, SIZEOF(ctx->addressParams.stakingKeyHash),
		        stakingInfo, SIZEOF(stakingInfo)
		);
		ui_displayPaginatedText(
		        "Staking key hash: ",
		        stakingInfo,
		        callback
		);
		break;

	case BLOCKCHAIN_POINTER:
		printBlockchainPointerToStr(ctx->addressParams.stakingKeyBlockchainPointer, stakingInfo, SIZEOF(stakingInfo));
		ui_displayPaginatedText(
		        "Staking key pointer: ",
		        stakingInfo,
		        callback
		);
		break;

	default:
		ASSERT(false);
	}
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
		displayStakingInfo(this_fn);
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
		displayStakingInfo(this_fn);
	}
	UI_STEP(DISPLAY_UI_STEP_ADDRESS) {
		char humanAddress[100]; // TODO is this enough?
		ASSERT(ctx->address.size <= SIZEOF(ctx->address.buffer));
		if (getAddressType(ctx->addressParams.header) == BYRON) {
			base58_encode(
			        ctx->address.buffer, ctx->address.size,
			        humanAddress, SIZEOF(humanAddress)
			);
		} else { // all shelley addresses
			bech32_encode("addr",
			              ctx->address.buffer, ctx->address.size,
			              humanAddress, SIZEOF(humanAddress)
			             );
		}
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
	// Initialize state
	if (isNewCall) {
		os_memset(ctx, 0, SIZEOF(*ctx));
	}
	ctx->responseReadyMagic = 0;

	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);

	parseAddressParams(wireDataBuffer, wireDataSize);

	switch (p1) {
#	define  CASE(P1, HANDLER_FN) case P1: {HANDLER_FN(); break;}
		CASE(P1_RETURN,  deriveAddress_handleReturn);
		CASE(P1_DISPLAY, deriveAddress_handleDisplay);
#	undef  CASE
	default:
		THROW(ERR_INVALID_REQUEST_PARAMETERS);
	}
}
