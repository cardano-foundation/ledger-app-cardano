#ifdef DEVEL

#include "common.h"

#include "runTests.h"
#include "stream.h"
#include "cbor.h"
#include "endian.h"
#include "base58.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "hash.h"
#include "attestUtxo.h"
#include "keyDerivation.h"
#include "addressUtils.h"
#include "crc32.h"
#include "hmac.h"
#include "txHashBuilder.h"
#include "textUtils.h"

void handleRunTests(
        uint8_t p1 MARK_UNUSED,
        uint8_t p2 MARK_UNUSED,
        uint8_t *wireBuffer MARK_UNUSED,
        size_t wireSize MARK_UNUSED,
        bool isNewCall MARK_UNUSED
)
{
	BEGIN_ASSERT_NOEXCEPT {
		PRINTF("Running tests\n");
		run_textUtils_test();
		run_txHashBuilder_test();
		run_endian_test();
		run_hex_test();
		run_stream_test();
		run_cbor_test();
		run_base58_test();
		run_hash_test();
		run_test_attestUtxo();
		run_key_derivation_test();
		run_address_utils_test();
		run_crc32_test();
		run_hmac_test();
		PRINTF("All tests done\n");
	} END_ASSERT_NOEXCEPT;

	io_send_buf(SUCCESS, NULL, 0);
	ui_idle();
}

#endif
