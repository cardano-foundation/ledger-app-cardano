#ifdef DEVEL

#include "common.h"
#include "textUtils.h"
#include "test_utils.h"

void testcase_formatAda(
        uint64_t amount,
        const char* expected
)
{
	PRINTF("testcase_formatAda %s\n", expected);
	char tmp[30];
	size_t len = str_formatAdaAmount(amount, tmp, SIZEOF(tmp));
	EXPECT_EQ(len, strlen(expected));
	EXPECT_EQ(strcmp(tmp, expected), 0);
}

void test_formatAda()
{
	testcase_formatAda(0,       "0.000000");
	testcase_formatAda(1,       "0.000001");
	testcase_formatAda(10,      "0.000010");
	testcase_formatAda(123456,  "0.123456");
	testcase_formatAda(1000000, "1.000000");
	testcase_formatAda(
	        12345678901234567890u,
	        "12,345,678,901,234.567890"
	);

	{
		PRINTF("test_formatAda edge cases");
		char tmp[12];
		os_memset(tmp, 'X', SIZEOF(tmp));
		str_formatAdaAmount(0, tmp, 9);
		EXPECT_EQ(tmp[8], 0);
		EXPECT_EQ(tmp[9], 'X');

		EXPECT_THROWS(str_formatAdaAmount(10000000, tmp, 9),
		              ERR_DATA_TOO_LARGE);
		EXPECT_EQ(tmp[9], 'X');
	}
}

void testcase_formatTtl(
        uint64_t ttl,
        const char* expected
)
{
	PRINTF("testcase_formatTtl %s\n", expected);
	char tmp[30];
	size_t len = str_formatTtl(ttl, tmp, SIZEOF(tmp));
	EXPECT_EQ(len, strlen(expected));
	EXPECT_EQ(strcmp(tmp, expected), 0);
}

void test_formatTtl()
{
	testcase_formatTtl(                     123, "epoch 0 / slot 123");
	testcase_formatTtl(         5 * 21600 + 124, "epoch 5 / slot 124");
	testcase_formatTtl(1000001llu * 21600 + 124, "epoch more than 1000000");
}

void run_textUtils_test()
{
	test_formatAda();
	test_formatTtl();
}

#endif
