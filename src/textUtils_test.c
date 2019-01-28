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
	size_t len = str_formatAdaAmount(tmp, SIZEOF(tmp), amount);
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
		str_formatAdaAmount(tmp, 9, 0);
		EXPECT_EQ(tmp[8], 0);
		EXPECT_EQ(tmp[9], 'X');

		EXPECT_THROWS(str_formatAdaAmount(tmp, 9,
		                                  10000000), ERR_DATA_TOO_LARGE);
		EXPECT_EQ(tmp[9], 'X');
	}
}

void run_textUtils_test()
{
	test_formatAda();
}

#endif
