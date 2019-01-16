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

void ensureParametersAreCorrect(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	VALIDATE_PARAM(p1 == 0);
	VALIDATE_PARAM(p2 == 0);
	VALIDATE_PARAM(dataLength >= 1);
	VALIDATE_PARAM(dataLength == dataBuffer[0] * 4 + 1 );
}


// TODO(ppershing): this is duplicate from deriveAddress!
static void initializePath(path_spec_t* pathSpec, uint8_t *dataBuffer, size_t dataSize)
{
	// Cast length to size_t
	size_t length = dataBuffer[0];
	if (length > ARRAY_LEN(pathSpec->path)) {
		THROW(ERR_INVALID_DATA);
	}
	if (length * 4 + 1 != dataSize) {
		THROW(ERR_INVALID_DATA);
	}

	pathSpec->length = length;

	for (size_t i = 0; i < length; i++) {
		size_t offset = 1 + 4 * i;
		pathSpec->path[i] = u4be_read(dataBuffer + offset);
	}
}

// forward declaration
void respond_with_public_key(const extendedPublicKey_t*);

void handleGetExtendedPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        size_t dataLength)
{
	ensureParametersAreCorrect(p1, p2, dataBuffer, dataLength);

	initializePath(& ctx->pathSpec, dataBuffer, dataLength);

	deriveExtendedPublicKey(
	        & ctx->pathSpec,
	        & ctx->extPubKey
	);

	respond_with_public_key(& ctx->extPubKey);
}

void respond_with_public_key(const extendedPublicKey_t* extPubKey)
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
