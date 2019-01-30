#ifndef H_CARDANO_APP_ATTEST_KEY
#define H_CARDANO_APP_ATTEST_KEY

#include "common.h"

#ifdef DEVEL
#include "handlers.h"
handler_fn_t handleGetAttestKey;
handler_fn_t handleSetAttestKey;
#endif

void attestKey_initialize();


// Note(ppershing): For now we have only one
// attestation purpose. This code is merely
// to make sure new potential uses *cannot*
// be mixed and attestation used in some
// replay attack
typedef enum {
	ATTEST_PURPOSE_BIND_UTXO_AMOUNT = 1,
} attest_purpose_t;

static const size_t ATTEST_HMAC_SIZE = 16;

void attest_writeHmac(
        attest_purpose_t purpose,
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
);

bool attest_isCorrectHmac(
        attest_purpose_t purpose,
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
);

#endif
