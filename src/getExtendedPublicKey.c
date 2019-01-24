#include "common.h"

#include "getExtendedPublicKey.h"
#include "keyDerivation.h"
#include "endian.h"
#include "state.h"
#include "uiHelpers.h"
#include "securityPolicy.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

static ins_get_ext_pubkey_context_t* ctx = &(instructionState.extPubKeyContext);


// forward declaration
static void respond_with_extended_public_key();

static int16_t RESPONSE_READY_MAGIC = 12345;

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

	if (policy == POLICY_DENY) {
		THROW(ERR_REJECTED_BY_POLICY);
	}

	// Calculation
	deriveExtendedPublicKey(
	        & ctx->pathSpec,
	        & ctx->extPubKey
	);
	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	// Response
	char pathStr[100];
	bip44_printToStr(&ctx->pathSpec, pathStr, SIZEOF(pathStr));
	ui_checkUserConsent(
	        policy,
	        "Export public key?",
	        pathStr,
	        respond_with_extended_public_key,
	        respond_with_user_reject
	);

}

static void respond_with_extended_public_key()
{
	ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);
	const extendedPublicKey_t* extPubKey = & ctx->extPubKey;

	// Note: we reuse G_io_apdu_buffer!
	uint8_t* responseBuffer = G_io_apdu_buffer;
	size_t responseMaxSize = SIZEOF(G_io_apdu_buffer);

	STATIC_ASSERT(SIZEOF(*extPubKey) == EXTENDED_PUBKEY_SIZE, "bad extended public key size");

	size_t outSize = 1 + EXTENDED_PUBKEY_SIZE;
	ASSERT(outSize <= responseMaxSize);

	u1be_write(responseBuffer, EXTENDED_PUBKEY_SIZE);
	os_memmove(responseBuffer + 1, extPubKey, SIZEOF(*extPubKey));

	io_send_buf(SUCCESS, responseBuffer, outSize);
	ui_idle();
}
