#include "common.h"
#include "deriveAddress.h"
#include "crc32.h"
#include "cbor.h"
#include "keyDerivation.h"
#include "endian.h"
#include "state.h"
#include "securityPolicy.h"
#include "uiHelpers.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)


static int16_t RESPONSE_READY_MAGIC = 11223;

static void respond_with_address();

static ins_derive_address_context_t* ctx = &(instructionState.deriveAddressContext);


void handleDeriveAddress(
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
	security_policy_t policy = policyForReturnDeriveAddress(&ctx->pathSpec);
	if (policy == POLICY_DENY) {
		THROW(ERR_REJECTED_BY_POLICY);
	}

	ctx->addressSize = deriveAddress(
	                           & ctx->pathSpec,
	                           & ctx->addressBuffer[0],
	                           SIZEOF(ctx->addressBuffer)
	                   );

	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	// Response
	char pathStr[100];
	bip44_printToStr(&ctx->pathSpec, pathStr, SIZEOF(pathStr));
	ui_checkUserConsent(
	        policy,
	        "Export address?",
	        pathStr,
	        respond_with_address,
	        respond_with_user_reject
	);
}


static void respond_with_address()
{
	ASSERT(ctx->responseReadyMagic == RESPONSE_READY_MAGIC);
	ctx->responseReadyMagic = 0;

	uint8_t* addressBuffer = ctx->addressBuffer;
	size_t addressSize = ctx->addressSize;

	ASSERT(addressSize < BUFFER_SIZE_PARANOIA);

	uint8_t* responseBuffer = G_io_apdu_buffer;
	size_t responseMaxSize = SIZEOF(G_io_apdu_buffer);

	size_t outSize = addressSize + 1;
	ASSERT(outSize <= responseMaxSize);

	u1be_write(responseBuffer, addressSize);
	os_memmove(responseBuffer + 1, addressBuffer, addressSize);

	io_send_buf(SUCCESS, responseBuffer, outSize);
	ui_idle();
}
