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

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

static getExtendedPublicKeyGlobal_t* ctx = &(instructionState.extPubKeyGlobal);


// forward declaration
static void respond_with_public_key(const extendedPublicKey_t*);


static void validatePath(const path_spec_t* pathSpec)
{
	VALIDATE_PARAM(isValidCardanoBIP44Path(&ctx->pathSpec));
	VALIDATE_PARAM(pathSpec->length >= 3);
	VALIDATE_PARAM(pathSpec->path[2] == (0 | HARDENED_BIP32)); // account 0
};


void handleGetExtendedPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        size_t dataSize)
{
	VALIDATE_PARAM(p1 == 0);
	VALIDATE_PARAM(p2 == 0);

	size_t parsedSize = pathSpec_parseFromWire(&ctx->pathSpec, dataBuffer, dataSize);

	if (parsedSize != dataSize) {
		THROW(ERR_INVALID_DATA);
	}

	validatePath(&ctx->pathSpec);

	deriveExtendedPublicKey(
	        & ctx->pathSpec,
	        & ctx->extPubKey
	);

	respond_with_public_key(& ctx->extPubKey);
}

static void respond_with_public_key(const extendedPublicKey_t* extPubKey)
{
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
