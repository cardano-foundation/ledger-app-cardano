#include "hmac.h"
#include "assert.h"
#include <os.h>

void hmac_sha256(
        const uint8_t *key, size_t keyLen,
        const uint8_t *input, size_t inputLen,
        uint8_t *output, size_t outputLen
)
{
	ASSERT(outputLen == 32);
	// TODO(ppershing): what is the return value of this
	// cx_hmac_sha256 function? According to cx.h
	// it returns int!
	cx_hmac_sha256(key, keyLen, input, inputLen, output, outputLen);
}
