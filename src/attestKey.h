#ifndef H_CARDANO_APP_ATTEST_KEY
#define H_CARDANO_APP_ATTEST_KEY

#include "common.h"
#include <stdbool.h>

static const size_t ATTEST_KEY_SIZE = 32;

typedef struct {
	uint8_t key[ATTEST_KEY_SIZE];
} attestKeyData_t;

void attestKey_initialize();

#ifdef DEVEL
void handleGetAttestKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, size_t dataSize);
void handleSetAttestKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, size_t dataSize);
#endif

void attest_writeHmac(
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
);

bool attest_isCorrectHmac(
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
);

#endif
