#include "attestKey.h"
#include <os.h>
#include "assert.h"
#include <stdbool.h>
#include "io.h"
#include "ux.h"
#include "utils.h"


static uint8_t P1_UNUSED = 0x00;
static uint8_t P2_UNUSED = 0x00;

// Global data
attestKeyData_t attestKeyData;

// Sanity check
STATIC_ASSERT(sizeof(attestKeyData.key) == ATTEST_KEY_SIZE, "bad ATTEST_KEY_SIZE");

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

void handleGetAttestKey(uint8_t p1, uint8_t p2, uint8_t* dataBuffer, size_t dataSize)
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
