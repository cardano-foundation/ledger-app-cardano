#ifndef H_CARDANO_APP_BASE58
#define H_CARDANO_APP_BASE58

#include <stdint.h>
#include <stddef.h>

size_t encode_base58(
        const uint8_t *inBuffer, size_t inSize,
        uint8_t *outBuffer, size_t maxOutSize
);

void run_base58_test();
#endif
