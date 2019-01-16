#ifndef H_CARDANO_APP_HMAC
#define H_CARDANO_APP_HMAC

#include <stdint.h>
#include <stddef.h>

void hmac_sha256(
        const uint8_t *keyBuffer, size_t keySize,
        const uint8_t *inBuffer, size_t inSize,
        uint8_t *outBuffer, size_t outSize
);


void run_hmac_test();
#endif
