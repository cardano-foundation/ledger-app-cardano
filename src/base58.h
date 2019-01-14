#ifndef H_CARDANO_APP_BASE58
#define H_CARDANO_APP_BASE58

#include <stdint.h>
#include <stddef.h>

size_t encode_base58(
        const uint8_t *in, size_t length,
        uint8_t *out, size_t maxoutlen
);

void run_base58_test();
#endif
