#ifndef H_CARDANO_APP_ATTEST_KEY
#define H_CARDANO_APP_ATTEST_KEY

#include <stdint.h>
#include <stddef.h>

static const unsigned ATTEST_KEY_SIZE = 32;

typedef struct {
	uint8_t key[ATTEST_KEY_SIZE];
} attestKeyData_t;

extern attestKeyData_t attestKeyData;

void attestKey_initialize();
void handleGetAttestKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, size_t dataSize);
void handleSetAttestKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, size_t dataSize);

#endif
