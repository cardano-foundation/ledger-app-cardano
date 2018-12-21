#include "cbor.h"
#include "cbor_test.h"
#include "test_utils.h"
#include "stream.h"
#include <os.h>


#define INIT(str_) \
  stream_t s; \
  os_memset(&s, 0, sizeof(s)); \
  uint8_t data[] = str_; \
  /* str_ is null-termanated so -1 */ \
  stream_appendData(&s, data, sizeof(data) - 1);

// Test unsigned parsing
void test_peek_token()
{
#define TESTCASE(data_, type_, width_, value_) \
    BEGIN_ASSERT_NOEXCEPT { \
        INIT(data_); \
        token_t res = cbor_peekToken(&s); \
        EXPECT_EQ(res.type, type_); \
        EXPECT_EQ(res.width, width_); \
        EXPECT_EQ(res.value, value_); \
        cbor_advanceToken(&s); \
        EXPECT_EQ(stream_availableBytes(&s), 0); \
    } END_ASSERT_NOEXCEPT; \

	{
		TESTCASE("\x00", TYPE_UNSIGNED, 0, 0);
		TESTCASE("\x01", TYPE_UNSIGNED, 0, 1);
		TESTCASE("\x0a", TYPE_UNSIGNED, 0, 10);
		TESTCASE("\x17", TYPE_UNSIGNED, 0, 23);

		TESTCASE("\x18\x18", TYPE_UNSIGNED, 1, 24);

		TESTCASE("\x19\x03\xe8", TYPE_UNSIGNED, 2, 1000);

		TESTCASE("\x1a\x00\x0f\x42\x40", TYPE_UNSIGNED, 4, 1000000);

		TESTCASE("\x1b\x00\x00\x00\xe8\xd4\xa5\x10\x00", TYPE_UNSIGNED, 8, 1000000000000);
		TESTCASE("\x1b\xff\xff\xff\xff\xff\xff\xff\xff", TYPE_UNSIGNED, 8, 18446744073709551615u);
	} {
		TESTCASE("\x40", TYPE_BYTES, 0, 0);
		TESTCASE("\x44", TYPE_BYTES, 0, 4);
	} {
		TESTCASE("\x80", TYPE_ARRAY, 0, 0);
		TESTCASE("\x83", TYPE_ARRAY, 0, 3);
		TESTCASE("\x98\x19", TYPE_ARRAY, 1, 25);

		TESTCASE("\x9f", TYPE_ARRAY_INDEF, 0, 0);
	} {
		TESTCASE("\xa0", TYPE_MAP, 0, 0);
		TESTCASE("\xa1", TYPE_MAP, 0, 1);
	} {
		TESTCASE("\xff", TYPE_INDEF_END, 0, 0);
	}
#undef TESTCASE
}

// test whether we reject non-canonical serialization
void test_noncanonical()
{
#define TESTCASE(data) \
    BEGIN_ASSERT_NOEXCEPT { \
        INIT(data); \
        EXPECT_THROWS(cbor_peekToken(&s), ERR_UNEXPECTED_TOKEN); \
    } END_ASSERT_NOEXCEPT;

	TESTCASE("\x18\x00");
	TESTCASE("\x18\x17");

	TESTCASE("\x19\x00\x00");
	TESTCASE("\x19\x00\xff");

	TESTCASE("\x1a\x00\x00\x00\x00");
	TESTCASE("\x1a\x00\x00\xff\xff");

	TESTCASE("\x1b\x00\x00\x00\x00\x00\x00\x00\x00");
	TESTCASE("\x1b\x00\x00\x00\x00\xff\xff\xff\xff");
#undef TESTCASE
}

void run_cbor_test()
{
	test_peek_token();
	test_noncanonical();
}
