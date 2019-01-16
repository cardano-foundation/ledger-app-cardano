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

// TODO: move this into global state
static deriveAddressGlobal_t* ctx = &(instructionState.deriveAddressGlobal);

static void validatePath(const path_spec_t* pathSpec)
{
	VALIDATE_PARAM(isValidCardanoBIP44Path(pathSpec));
	// other checks are when deriving private key
	VALIDATE_PARAM(pathSpec->path[2] == (0 | HARDENED_BIP32)); // account 0
	VALIDATE_PARAM(pathSpec->length >= 5);
	VALIDATE_PARAM(pathSpec->path[3] == 0 || pathSpec->path[3] == 1);
}

void handleDeriveAddress(
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
