#include "hex_utils.h"
#include "stream.h"
#include "utils.h"
#include "assert.h"
#include "messageSigning.h"
#include "stream.h"
#include "endian.h"
#include "bip44.h"
#include "keyDerivation.h"

void signRawMessage(privateKey_t* privateKey,
                    const uint8_t* messageBuffer, size_t messageSize,
                    uint8_t* outBuffer, size_t outSize)
{
	uint8_t signature[64];
	ASSERT(messageSize < BUFFER_SIZE_PARANOIA);
	ASSERT(outSize == SIZEOF(signature));

	// Note(ppershing): this could be done without
	// temporary copy
	STATIC_ASSERT(sizeof(int) == sizeof(size_t), "bad sizing");
	size_t signatureSize =
	        (size_t) cx_eddsa_sign(
	                (const struct cx_ecfp_256_private_key_s*) privateKey,
	                0 /* mode */,
	                CX_SHA512,
	                messageBuffer, messageSize,
	                NULL /* ctx */, 0 /* ctx len */,
	                signature, SIZEOF(signature),
	                0 /* info */
	        );

	ASSERT(signatureSize == 64);
	os_memmove(outBuffer, signature, signatureSize);
}

void getTxWitness(bip44_path_t* pathSpec,
                  const uint8_t* txHashBuffer, size_t txHashSize,
                  uint8_t* outBuffer, size_t outSize)
{

	chain_code_t chainCode;
	privateKey_t privateKey;

	TRACE("derive private key");
	derivePrivateKey(pathSpec, &chainCode, &privateKey);

	ASSERT(txHashSize == 32);
	uint8_t messageBuffer[8 + txHashSize];
	// Warning(ppershing): following magic contains some CBOR parts so
	// be careful if txHashSize changes
	u8be_write(messageBuffer, 0x011a2d964a095820);
	os_memmove(messageBuffer + 8, txHashBuffer, txHashSize);

	signRawMessage(
	        &privateKey,
	        messageBuffer, SIZEOF(messageBuffer),
	        outBuffer, outSize
	);
}
