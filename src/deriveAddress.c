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

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

#define MAX_BIP32_PATH 32

static void io_exchange_address();
void ui_idle();

derive_address_data_t daData;

static void ensureParametersAreCorrect(
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

static void initializePath(uint8_t *dataBuffer)
{
	STATIC_ASSERT((255 - 1) / 4 > MAX_BIP32_PATH, __bad_length);

	daData.pathSpec.length = dataBuffer[0];

	uint8_t i = 0;
	for (i = 0; i < daData.pathSpec.length; i++) {
		uint8_t offset = 1 + 4 * i;

		daData.pathSpec.path[i] = U4BE(dataBuffer, offset);
	}
}

void handleDeriveAddress(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	ensureParametersAreCorrect(p1, p2, dataBuffer, dataLength);

	initializePath(dataBuffer);

	daData.addressLength = deriveAddress(
	                               &daData.pathSpec,
	                               daData.address, SIZEOF(daData.address)
	                       );

	io_exchange_address();
}

static void io_exchange_address()
{
	uint32_t tx = 0;

	G_io_apdu_buffer[tx++] = daData.addressLength;

	os_memmove(G_io_apdu_buffer + tx, daData.address, daData.addressLength);

	tx += daData.addressLength;

	G_io_apdu_buffer[tx++] = 0x90;
	G_io_apdu_buffer[tx++] = 0x00;

	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
	ui_idle();
}
