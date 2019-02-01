#include "common.h"
#include "attestKey.h"
#include "hmac.h"

static const size_t ATTEST_KEY_SIZE = 32;

typedef struct {
	uint8_t key[ATTEST_KEY_SIZE];
} attestKeyData_t;


// Global data
attestKeyData_t attestKeyData;

// Sanity check
STATIC_ASSERT(sizeof(attestKeyData.key) == ATTEST_KEY_SIZE, "bad ATTEST_KEY_SIZE");

void attest_writeHmac(
        attest_purpose_t purpose,
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
)
{
	if (purpose != ATTEST_PURPOSE_BIND_UTXO_AMOUNT) {
		// Once implemented, purpose *must* become part of the HMAC
		// as we need to avoid cross-purpose replay attacks
		THROW(ERR_NOT_IMPLEMENTED);
	}
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
        attest_purpose_t purpose,
        const uint8_t* data, uint8_t dataSize,
        uint8_t* hmac, uint8_t hmacSize
)
{
	uint8_t tmpBuffer[ATTEST_HMAC_SIZE];

	ASSERT(hmacSize == ATTEST_HMAC_SIZE);

	attest_writeHmac(purpose, data, dataSize, tmpBuffer, SIZEOF(tmpBuffer));
	return os_memcmp(hmac, tmpBuffer, SIZEOF(tmpBuffer)) == 0;
}


// Should be called at app startup
void attestKey_initialize()
{
	cx_rng(attestKeyData.key, ATTEST_KEY_SIZE);
}

#ifdef DEVEL
static const uint8_t P1_UNUSED = 0x00;
static const uint8_t P2_UNUSED = 0x00;

void handleSetAttestKey(
        uint8_t p1, uint8_t p2,
        uint8_t* wireBuffer, size_t wireSize,
        bool isNewCall MARK_UNUSED
)
{
	VALIDATE(p1 == P1_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(wireSize == ATTEST_KEY_SIZE, ERR_INVALID_DATA);

	os_memmove(attestKeyData.key, wireBuffer, ATTEST_KEY_SIZE);
	io_send_buf(SUCCESS, NULL, 0);
	ui_idle();
}

void handleGetAttestKey(
        uint8_t p1, uint8_t p2,
        uint8_t* wireBuffer MARK_UNUSED, size_t wireSize,
        bool isNewCall MARK_UNUSED
)
{

	VALIDATE(p1 == P1_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(p2 == P2_UNUSED, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(wireSize == 0, ERR_INVALID_DATA);

	io_send_buf(SUCCESS, attestKeyData.key, ATTEST_KEY_SIZE);
	ui_idle();
}
#endif
