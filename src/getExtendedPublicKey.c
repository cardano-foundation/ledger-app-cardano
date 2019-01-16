#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "errors.h"
#include "getExtendedPublicKey.h"
#include "keyDerivation.h"
#include "utils.h"
#include "endian.h"

#define VALIDATE_PARAM(cond) if (!(cond)) THROW(ERR_INVALID_REQUEST_PARAMETERS)

get_ext_pub_key_data_t data;

void io_exchange_public_key(cx_ecfp_public_key_t* publicKey, chain_code_t* chainCode);

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

void initializePath(uint8_t *dataBuffer, path_spec_t* pathSpec)
{
	STATIC_ASSERT((255 - 1 /* length data */ ) / 4 /* sizeof(pathspec.path[i]) */ > MAX_PATH_LENGTH, __bad_length);

	pathSpec->length = dataBuffer[0];

	uint8_t i = 0;
	for (i = 0; i < pathSpec->length; i++) {
		uint8_t offset = 1 + 4 * i;

		pathSpec->path[i] = u4be_read(dataBuffer + offset);
	}
}

void handleGetExtendedPublicKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	ensureParametersAreCorrect(p1, p2, dataBuffer, dataLength);

	initializePath(dataBuffer, &data.pathSpec);

	BEGIN_TRY {
		TRY {
			derivePrivateKey(
			        &data.pathSpec,
			        &data.chainCode,
			        &data.privateKey
			);

			deriveRawPublicKey(&data.privateKey, &data.publicKey);
		}
		FINALLY {
			os_memset(&data.privateKey, 0, sizeof(data.privateKey));
		}
	} END_TRY;

	io_exchange_public_key(&data.publicKey, &data.chainCode);
}

void ui_idle();

void io_exchange_public_key(cx_ecfp_public_key_t* publicKey, chain_code_t* chainCode)
{
	uint32_t tx = 0;

	G_io_apdu_buffer[tx++] = PUBLIC_KEY_SIZE;

	uint8_t rawPublicKey[PUBLIC_KEY_SIZE];
	extractRawPublicKey(publicKey, rawPublicKey, SIZEOF(rawPublicKey));
	os_memmove(G_io_apdu_buffer + tx, rawPublicKey, PUBLIC_KEY_SIZE);

	tx += PUBLIC_KEY_SIZE;

	os_memmove(G_io_apdu_buffer + tx, chainCode->code, CHAIN_CODE_SIZE);

	tx += CHAIN_CODE_SIZE;

	G_io_apdu_buffer[tx++] = 0x90;
	G_io_apdu_buffer[tx++] = 0x00;

	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
	ui_idle();
}
