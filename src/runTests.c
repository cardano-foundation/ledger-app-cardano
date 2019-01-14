#include "runTests.h"
#include "errors.h"
#include "io.h"
#include "stream.h"
#include "cbor.h"
#include "endian.h"
#include "assert.h"
#include "base58.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "hash.h"
#include "attestUtxo.h"
#include "keyDerivation.h"
#include "crc32.h"
#include <stdbool.h>
#include <os.h>
#include "hmac.h"
#include "ux.h"

void handleRunTests(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	BEGIN_ASSERT_NOEXCEPT {
		PRINTF("Running tests\n");
		run_endian_test();
		run_hex_test();
		run_stream_test();
		run_cbor_test();
		run_base58_test();
		run_hash_test();
		run_test_attestUtxo();
		run_key_derivation_test();
		run_crc32_test();
		run_hmac_test();
		PRINTF("All tests done\n");
	} END_ASSERT_NOEXCEPT;

	io_send_buf(SUCCESS, NULL, 0);
	ui_idle();
}
