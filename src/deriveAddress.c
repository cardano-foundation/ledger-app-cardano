#include "deriveAddress.h"
#include "crc32.h"
#include "cbor.h"
#include "stream.h"
#include "base58.h"
#include <os_io_seproxyhal.h>
#include <stdint.h>
#include "errors.h"
#include "assert.h"
#include "hash.h"
#include "keyDerivation.h"
#include "utils.h"
#include "endian.h"
#include "io.h"
#include "ux.h"
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
        uint8_t *dataBuffer,
        size_t dataSize)
{
	VALIDATE_PARAM(p1 == 0);
	VALIDATE_PARAM(p2 == 0);
	ctx->responseReadyMagic = 0;

	size_t parsedSize = bip44_parseFromWire(&ctx->pathSpec, dataBuffer, dataSize);

	if (parsedSize != dataSize) {
		THROW(ERR_INVALID_DATA);
	}

	security_policy_t policy = policyForDeriveAddress(&ctx->pathSpec);
	if (policy == POLICY_DENY) {
		THROW(ERR_REJECTED_BY_POLICY);
	}

	ctx->addressSize = deriveAddress(
	                           & ctx->pathSpec,
	                           & ctx->addressBuffer[0],
	                           SIZEOF(ctx->addressBuffer)
	                   );

	ctx->responseReadyMagic = RESPONSE_READY_MAGIC;

	switch(policy) {
	case POLICY_ALLOW:
		respond_with_address();
		break;
	case POLICY_PROMPT_BEFORE_RESPONSE:
		displayConfirm(
		        "Export address?",
		        "",
		        respond_with_address,
		        defaultReject
		);
		break;
	default:
		ASSERT(false);
	}
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
