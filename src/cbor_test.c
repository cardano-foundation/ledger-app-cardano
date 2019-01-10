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
		TESTCASE("00", CBOR_TYPE_UNSIGNED, 0, 0);
		TESTCASE("01", CBOR_TYPE_UNSIGNED, 0, 1);
		TESTCASE("0a", CBOR_TYPE_UNSIGNED, 0, 10);
		TESTCASE("17", CBOR_TYPE_UNSIGNED, 0, 23);

		TESTCASE("1818", CBOR_TYPE_UNSIGNED, 1, 24);

		TESTCASE("1903e8", CBOR_TYPE_UNSIGNED, 2, 1000);

		TESTCASE("1a000f4240", CBOR_TYPE_UNSIGNED, 4, 1000000);

		TESTCASE("1b000000e8d4a51000", CBOR_TYPE_UNSIGNED, 8, 1000000000000);
		TESTCASE("1bffFFffFFffFFffFF", CBOR_TYPE_UNSIGNED, 8, 18446744073709551615u);
	} {
		TESTCASE("40", CBOR_TYPE_BYTES, 0, 0);
		TESTCASE("44", CBOR_TYPE_BYTES, 0, 4);
	} {
		TESTCASE("80", CBOR_TYPE_ARRAY, 0, 0);
		TESTCASE("83", CBOR_TYPE_ARRAY, 0, 3);
		TESTCASE("9819", CBOR_TYPE_ARRAY, 1, 25);

		TESTCASE("9f", CBOR_TYPE_ARRAY_INDEF, 0, 0);
	} {
		TESTCASE("a0", CBOR_TYPE_MAP, 0, 0);
		TESTCASE("a1", CBOR_TYPE_MAP, 0, 1);
	} {
		TESTCASE("d818", CBOR_TYPE_TAG, 1, 24);
	} {
		TESTCASE("ff", CBOR_TYPE_INDEF_END, 0, 0);
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

void test_serialization()
{
#define TESTCASE(str_, type_, value_) \
	{ \
	    stream_t s1; \
	    stream_t s2; \
	    stream_init(&s1); \
	    stream_init(&s2); \
	    stream_appendFromHexString(&s1, str_); \
	    cbor_appendToken(&s2, type_, value_); \
	    EXPECT_EQ_STREAM(&s1, &s2); \
	}

	{
		TESTCASE("00", CBOR_TYPE_UNSIGNED, 0);
		TESTCASE("01", CBOR_TYPE_UNSIGNED, 1);
		TESTCASE("0a", CBOR_TYPE_UNSIGNED, 10);
		TESTCASE("17", CBOR_TYPE_UNSIGNED, 23);

		TESTCASE("1818", CBOR_TYPE_UNSIGNED, 24);

		TESTCASE("1903e8", CBOR_TYPE_UNSIGNED, 1000);

		TESTCASE("1a000f4240", CBOR_TYPE_UNSIGNED, 1000000);

		TESTCASE("1b000000e8d4a51000", CBOR_TYPE_UNSIGNED, 1000000000000);
		TESTCASE("1bffFFffFFffFFffFF", CBOR_TYPE_UNSIGNED, 18446744073709551615u);
	} {
		TESTCASE("40", CBOR_TYPE_BYTES, 0);
		TESTCASE("44", CBOR_TYPE_BYTES, 4);
	} {
		TESTCASE("80", CBOR_TYPE_ARRAY, 0);
		TESTCASE("83", CBOR_TYPE_ARRAY, 3);
		TESTCASE("9819", CBOR_TYPE_ARRAY, 25);

		TESTCASE("9f", CBOR_TYPE_ARRAY_INDEF, 0);
	} {
		TESTCASE("a0", CBOR_TYPE_MAP, 0);
		TESTCASE("a1", CBOR_TYPE_MAP, 1);
	} {
		TESTCASE("ff", CBOR_TYPE_INDEF_END, 0);
	}
#undef TESTCASE

	{
		// Check invalid type
		stream_t s1;
		stream_init(&s1);
		EXPECT_THROWS(cbor_appendToken(&s1, 47, 0), ERR_UNEXPECTED_TOKEN);
	}
}

void run_cbor_test()
{
	test_peek_token();
	test_noncanonical();
	test_serialization();
}
