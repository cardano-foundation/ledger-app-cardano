#include "hmac.h"
#include "assert.h"
#include "utils.h"
#include <os.h>

void hmac_sha256(
        const uint8_t *keyBuffer, size_t keySize,
        const uint8_t *inBuffer, size_t inSize,
        uint8_t *outBuffer, size_t outSize
)
{
	ASSERT(outSize == 32);
	ASSERT(inSize < BUFFER_SIZE_PARANOIA);
	ASSERT(keySize < BUFFER_SIZE_PARANOIA);

	// TODO(ppershing): what is the return value of this
	// cx_hmac_sha256 function? According to cx.h
	// it returns int!
	cx_hmac_sha256(keyBuffer, keySize, inBuffer, inSize, outBuffer, outSize);
}
