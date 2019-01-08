#include "cbor.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "stream.h"
#include <os.h>
#include <string.h>


// Test vectors are taken from
// https://tools.ietf.org/html/rfc7049#appendix-A
void test_peek_token()
{
#define TESTCASE(str_, type_, width_, value_) \
	{ \
	    stream_t s; \
	    stream_init(&s); \
	    stream_appendFromHexString(&s, str_); \
	    token_t res = cbor_peekToken(&s); \
	    EXPECT_EQ(res.type, type_); \
	    EXPECT_EQ(res.width, width_); \
	    EXPECT_EQ(res.value, value_); \
	    cbor_advanceToken(&s); \
	    EXPECT_EQ(stream_availableBytes(&s), 0); \
	}

	{
		TESTCASE("00", TYPE_UNSIGNED, 0, 0);
		TESTCASE("01", TYPE_UNSIGNED, 0, 1);
		TESTCASE("0a", TYPE_UNSIGNED, 0, 10);
		TESTCASE("17", TYPE_UNSIGNED, 0, 23);

		TESTCASE("1818", TYPE_UNSIGNED, 1, 24);

		TESTCASE("1903e8", TYPE_UNSIGNED, 2, 1000);

		TESTCASE("1a000f4240", TYPE_UNSIGNED, 4, 1000000);

		TESTCASE("1b000000e8d4a51000", TYPE_UNSIGNED, 8, 1000000000000);
		TESTCASE("1bffFFffFFffFFffFF", TYPE_UNSIGNED, 8, 18446744073709551615u);
	} {
		TESTCASE("40", TYPE_BYTES, 0, 0);
		TESTCASE("44", TYPE_BYTES, 0, 4);
	} {
		TESTCASE("80", TYPE_ARRAY, 0, 0);
		TESTCASE("83", TYPE_ARRAY, 0, 3);
		TESTCASE("9819", TYPE_ARRAY, 1, 25);

		TESTCASE("9f", TYPE_ARRAY_INDEF, 0, 0);
	} {
		TESTCASE("a0", TYPE_MAP, 0, 0);
		TESTCASE("a1", TYPE_MAP, 0, 1);
	} {
		TESTCASE("d818", TYPE_TAG, 1, 24);
	} {
		TESTCASE("ff", TYPE_INDEF_END, 0, 0);
	}
#undef TESTCASE
}

// test whether we reject non-canonical serialization
void test_noncanonical()
{
#define TESTCASE(str_) \
	{ \
	    stream_t s; \
	    stream_init(&s); \
	    stream_appendFromHexString(&s, str_); \
	    EXPECT_THROWS(cbor_peekToken(&s), ERR_UNEXPECTED_TOKEN); \
	}

	TESTCASE("1800");
	TESTCASE("1817");

	TESTCASE("190000");
	TESTCASE("1900ff");

	TESTCASE("1a00000000");
	TESTCASE("1a0000ffff");

	TESTCASE("1b0000000000000000");
	TESTCASE("1b00000000ffffffff");
#undef TESTCASE
}

void run_cbor_test()
{
	test_peek_token();
	test_noncanonical();
}
