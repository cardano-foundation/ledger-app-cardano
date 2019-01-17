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

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)



static void io_respond_with_address(uint8_t* addressBuffer, size_t addressSize);

static ins_derive_address_context_t* ctx = &(instructionState.deriveAddressContext);


static void validatePath(const bip44_path_t* pathSpec)
{
#define VALIDATE_PATH(cond) if (!(cond)) THROW(ERR_INVALID_BIP44_PATH)
	VALIDATE_PATH(bip44_hasValidPrefix(pathSpec));
	VALIDATE_PATH(bip44_hasValidAccount(pathSpec));
	VALIDATE_PATH(bip44_hasValidChainType(pathSpec));
	VALIDATE_PATH(bip44_containsAddress(pathSpec));
#undef VALIDATE_PATH
}

void handleDeriveAddress(
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

	ctx->addressSize = deriveAddress(
	                           & ctx->pathSpec,
	                           & ctx->addressBuffer[0],
	                           SIZEOF(ctx->addressBuffer)
	                   );

	io_respond_with_address(& ctx->addressBuffer[0], ctx->addressSize);
}

static void io_respond_with_address(uint8_t* addressBuffer, size_t addressSize)
{
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
