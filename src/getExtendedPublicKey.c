#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "errors.h"
#include "getExtendedPublicKey.h"
#include "keyDerivation.h"
#include "utils.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

get_ext_pub_key_data_t data;

void io_exchange_address();

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

void initializePath(uint8_t *dataBuffer)
{
	STATIC_ASSERT((255-1)/4 > MAX_BIP32_PATH, __bad_length);

	data.pathLength = dataBuffer[0];

	uint8_t i = 0;
	for (i = 0; i < data.pathLength; i++) {
		uint8_t offset = 1 + 4 * i;

		data.bip32Path[i] = U4BE(dataBuffer, offset);
	}
}

void handleGetExtendedPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	ensureParametersAreCorrect(p1, p2, dataBuffer, dataLength);

	initializePath(dataBuffer);

	BEGIN_TRY {
		TRY {
			derivePrivateKey(
			        data.bip32Path,
			        data.pathLength,
			        &data.chainCode,
			        &data.privateKey
			);

			deriveRawPublicKey(&data.privateKey, &data.publicKey);
		}
		FINALLY {
			os_memset(&data.privateKey, 0, sizeof(data.privateKey));
		}
	} END_TRY;

	io_exchange_address();
}

void ui_idle();

void io_exchange_address()
{
	uint32_t tx = 0;

	G_io_apdu_buffer[tx++] = PUBLIC_KEY_SIZE;

	uint8_t rawPublicKey[PUBLIC_KEY_SIZE];
	extractRawPublicKey(&data.publicKey, rawPublicKey, SIZEOF(rawPublicKey));
	os_memmove(G_io_apdu_buffer + tx, rawPublicKey, PUBLIC_KEY_SIZE);

	tx += PUBLIC_KEY_SIZE;

	os_memmove(G_io_apdu_buffer + tx, data.chainCode.code, CHAIN_CODE_SIZE);

	tx += CHAIN_CODE_SIZE;

	G_io_apdu_buffer[tx++] = 0x90;
	G_io_apdu_buffer[tx++] = 0x00;

	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
	ui_idle();
}
