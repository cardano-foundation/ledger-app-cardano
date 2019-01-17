#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "errors.h"
#include "getExtendedPublicKey.h"
#include "keyDerivation.h"
#include "utils.h"
#include "endian.h"
#include "state.h"
#include "utils.h"
#include "ux.h"
#include "io.h"
#include "uiHelpers.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

static ins_get_ext_pubkey_context_t* ctx = &(instructionState.extPubKeyContext);


// forward declaration
static void respond_with_extended_public_key();
static void default_reject();

static void validatePath(const bip44_path_t* pathSpec)
{
#define VALIDATE_PATH(cond) if (!(cond)) THROW(ERR_INVALID_BIP44_PATH)

	VALIDATE_PATH(bip44_hasValidPrefix(pathSpec));
	VALIDATE_PATH(bip44_hasValidAccount(pathSpec));
#undef VALIDATE_PATH
};


void handleGetExtendedPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        size_t dataSize)
{
	VALIDATE_PARAM(p1 == 0);
	VALIDATE_PARAM(p2 == 0);

	size_t parsedSize = bip44_parseFromWire(&ctx->pathSpec, dataBuffer, dataSize);

	if (parsedSize != dataSize) {
		THROW(ERR_INVALID_DATA);
	}

	validatePath(&ctx->pathSpec);

	deriveExtendedPublicKey(
	        & ctx->pathSpec,
	        & ctx->extPubKey
	);

	const bool REQUIRE_CONFIRM = false;
	if (REQUIRE_CONFIRM) {
		displayConfirm(
		        "Export public key?",
		        "",
		        respond_with_extended_public_key,
		        default_reject
		);
	} else {
		respond_with_extended_public_key();
	}
}

// TODO(ppershing): move to uiHelpers?
static void default_reject()
{
	io_send_buf(ERR_USER_REJECTED, NULL, 0);
	ui_idle();
}

static void respond_with_extended_public_key()
{
	const extendedPublicKey_t* extPubKey = & ctx->extPubKey;

	// Note: we reuse G_io_apdu_buffer!
	uint8_t* responseBuffer = G_io_apdu_buffer;
	size_t responseMaxSize = SIZEOF(G_io_apdu_buffer);

	STATIC_ASSERT(SIZEOF(*extPubKey) == EXTENDED_PUBKEY_SIZE, __bad_ext_pub_key_size);

	size_t outSize = 1 + EXTENDED_PUBKEY_SIZE;
	ASSERT(outSize <= responseMaxSize);

	u1be_write(responseBuffer, EXTENDED_PUBKEY_SIZE);
	os_memmove(responseBuffer + 1, extPubKey, SIZEOF(*extPubKey));

	io_send_buf(SUCCESS, responseBuffer, outSize);
	ui_idle();
}
