#ifndef H_CARDANO_APP_BECH32
#define H_CARDANO_APP_BECH32

#include <stdint.h>
#include <stddef.h>

/*
 * Encode bytes, using human-readable prefix given in hrp.
 *
 * The return value is the length of the resulting bech32-encoded string,
 * i.e. strlen(hrp) + 1 [separator] + 6 [checksum] +
       + ceiling of (8/5 * bytesLen) [base32 encoding with padding].

 * The output buffer must be capable of storing one more character.
 */
size_t bech32_encode(const char *hrp, const uint8_t *bytes, size_t bytes_len, char *output, size_t maxOutputSize);

void run_bech32_test();

#endif /* H_CARDANO_APP_BECH32 */
