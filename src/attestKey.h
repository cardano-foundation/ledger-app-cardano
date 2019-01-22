#ifndef H_CARDANO_APP_ATTEST_KEY
#define H_CARDANO_APP_ATTEST_KEY

#include "common.h"

#ifdef DEVEL
#include "handlers.h"
handler_fn_t handleGetAttestKey;
handler_fn_t handleSetAttestKey;
#endif

void attestKey_initialize();



static const size_t ATTEST_HMAC_SIZE = 16;

void attest_writeHmac(
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
);

bool attest_isCorrectHmac(
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
);

#endif
