#include <os_io_seproxyhal.h>
#include <stdint.h>

#include "assert.h"
#include "os.h"
#include "errors.h"

#define MAX_BIP32_PATH 10
uint32_t bip32Path[MAX_BIP32_PATH];
uint8_t pathLength;
uint8_t privateKeyData[32];
unsigned char chainCode[32];
cx_ecfp_public_key_t publicKey;
cx_ecfp_private_key_t privateKey;

void io_exchange_address();


// TODO: Add private and pub key init and remove old API flags.
void derive_bip32_node_private_key(uint8_t *privateKeyDataIn)
{
	STATIC_ASSERT(CX_APILEVEL >= 5, unsupported_api_level);
	os_memset(chainCode, 0, sizeof(chainCode));

	os_perso_derive_node_bip32(
	        CX_CURVE_Ed25519,
	        bip32Path,
	        pathLength,
	        privateKeyDataIn,
	        chainCode
	);
}

#define BIP_44 44
#define ADA_COIN_TYPE 1815
#define ADA_ACCOUNT 0
#define HARDENED_BIP32 0x80000000

#define P1_RECOVERY_PASSPHRASE 0x01
#define P1_ADDRESS_PUB_KEY 0x02

void handleGetPubKey(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength,
        volatile unsigned int *flags,
        volatile unsigned int *tx)
{
	if (p1 == P1_RECOVERY_PASSPHRASE) {
		pathLength = 2;

		bip32Path[0] = BIP_44 |
		               HARDENED_BIP32;
		bip32Path[1] = ADA_COIN_TYPE |
		               HARDENED_BIP32;
	} else if (p1 == P1_ADDRESS_PUB_KEY) {
		// TODO(ppershing): fixme
		pathLength = 4;

		bip32Path[0] = BIP_44 |
		               HARDENED_BIP32;
		bip32Path[1] = ADA_COIN_TYPE |
		               HARDENED_BIP32;
		bip32Path[2] = ADA_ACCOUNT |
		               HARDENED_BIP32;
		bip32Path[3] = (dataBuffer[0] << 24) |
		               (dataBuffer[1] << 16) |
		               (dataBuffer[2] << 8) |
		               (dataBuffer[3]);
		/*if (opCtx.bip32Path[3] < HARDENED_BIP32)
		{
		    //TODO: Set error code
		    THROW(0x5201);
		}*/
	} else {
		THROW(SW_INVALID_PARAM);
	}

	derive_bip32_node_private_key(privateKeyData);
	cx_ecfp_init_private_key(CX_CURVE_Ed25519,
	                         privateKeyData,
	                         32,
	                         &privateKey);

	cx_ecfp_generate_pair(CX_CURVE_Ed25519,
	                      &publicKey,
	                      &privateKey, 1);

	os_memset(&privateKey, 0, sizeof(privateKey));
	os_memset(privateKeyData, 0, sizeof(privateKeyData));

	//#ifdef HEADLESS
	io_exchange_address();
	//#else
	//UX_DISPLAY(ui_address_nanos, NULL);
	//flags |= IO_ASYNCH_REPLY;
	//#endif
}

void ui_idle();

void io_exchange_address()
{
	uint32_t tx = 0;

	G_io_apdu_buffer[tx++] = 32; // + sizeof(opCtx.chainCode);

	uint8_t _publicKey[32];
	// copy public key little endian to big endian
	uint8_t i;
	for (i = 0; i < 32; i++) {
		_publicKey[i] = publicKey.W[64 - i];
	}
	if ((publicKey.W[32] & 1) != 0) {
		_publicKey[31] |= 0x80;
	}

	os_memmove(G_io_apdu_buffer + tx, _publicKey, 32);

	tx += 32;

	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
	ui_idle();
}
