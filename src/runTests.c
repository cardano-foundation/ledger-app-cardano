#ifdef DEVEL

#include "common.h"

#include "runTests.h"
#include "stream.h"
#include "cbor.h"
#include "endian.h"
#include "base58.h"
#include "bech32.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "hash.h"
#include "keyDerivation.h"
#include "addressUtilsByron.h"
#include "addressUtilsShelley.h"
#include "crc32.h"
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
		run_bech32_test();
		run_textUtils_test();
		run_txHashBuilder_test();
		run_endian_test();
		run_hex_test();
		run_stream_test();
		run_cbor_test();
		run_base58_test();
		run_hash_test();
		run_key_derivation_test();
		run_addressUtilsByron_test();
		run_addressUtilsShelley_test();
		run_crc32_test();
		PRINTF("All tests done\n");
	} END_ASSERT_NOEXCEPT;

	io_send_buf(SUCCESS, NULL, 0);
	ui_idle();
}

#endif
