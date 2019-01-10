#ifndef H_CARDANO_APP_HMAC
#define H_CARDANO_APP_HMAC

#include <stdint.h>
#include <stddef.h>

void hmac_sha256(
        const uint8_t *key, size_t keyLen,
        const uint8_t *input, size_t inputLen,
        uint8_t *output, size_t outputLen
);


void run_hmac_test();
#endif
