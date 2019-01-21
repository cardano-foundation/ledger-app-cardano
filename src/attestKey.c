#include "common.h"
#include "attestKey.h"
#include "hmac.h"

static uint8_t P1_UNUSED = 0x00;
static uint8_t P2_UNUSED = 0x00;

// Global data
attestKeyData_t attestKeyData;

// Sanity check
STATIC_ASSERT(sizeof(attestKeyData.key) == ATTEST_KEY_SIZE, "bad ATTEST_KEY_SIZE");


static size_t ATTEST_HMAC_SIZE = 16;

void attest_writeHmac(
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
)
{
	ASSERT(hmacSize = ATTEST_HMAC_SIZE);
	// attested HMAC
	uint8_t tmpBuffer[32];
	hmac_sha256(
	        attestKeyData.key, SIZEOF(attestKeyData.key),
	        data, dataSize,
	        tmpBuffer, SIZEOF(tmpBuffer)
	);

	// sanity check before copying
	ASSERT(hmacSize <= SIZEOF(tmpBuffer));
	os_memmove(hmac, tmpBuffer, hmacSize);
}

// Todo: this could be done in a better way
bool attest_isCorrectHmac(
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
)
{
	uint8_t tmpBuffer[ATTEST_HMAC_SIZE];

	ASSERT(hmacSize == ATTEST_HMAC_SIZE);

	attest_writeHmac(data, dataSize, tmpBuffer, SIZEOF(tmpBuffer));
	return os_memcmp(hmac, tmpBuffer, SIZEOF(tmpBuffer)) == 0;
}


// Should be called at app startup
void attestKey_initialize()
{
	cx_rng(attestKeyData.key, ATTEST_KEY_SIZE);
}

void handleSetAttestKey(uint8_t p1, uint8_t p2, uint8_t* dataBuffer, size_t dataSize)
{
	#ifndef DEVEL
	// This call shouldn't be available from non-devel mode
	ASSERT(false);
	#endif
	VALIDATE_REQUEST_PARAM(p1 == P1_UNUSED);
	VALIDATE_REQUEST_PARAM(p2 == P2_UNUSED);
	VALIDATE_REQUEST_PARAM(dataSize == ATTEST_KEY_SIZE);

	os_memmove(attestKeyData.key, dataBuffer, ATTEST_KEY_SIZE);
	io_send_buf(SUCCESS, NULL, 0);
	ui_idle();
}

void handleGetAttestKey(uint8_t p1, uint8_t p2, uint8_t* dataBuffer MARK_UNUSED, size_t dataSize)
{
	#ifndef DEVEL
	// This call shouldn't be available from non-devel mode
	ASSERT(false);
	#endif

	VALIDATE_REQUEST_PARAM(p1 == P1_UNUSED);
	VALIDATE_REQUEST_PARAM(p2 == P2_UNUSED);
	VALIDATE_REQUEST_PARAM(dataSize == 0);

	io_send_buf(SUCCESS, attestKeyData.key, ATTEST_KEY_SIZE);
	ui_idle();
}
