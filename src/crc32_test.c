#include "crc32.h"
#include "test_utils.h"
#include "utils.h"
#include "stream.h"
#include "hex_utils.h"

static void testcase_crc32(const char* data, uint32_t expected)
{
	uint8_t buffer[100];
	size_t bufferLen = parseHexString(data, buffer, 100);
	uint32_t result = crc32(buffer, bufferLen);

	EXPECT_EQ(result, expected);
}

void run_crc32_test()
{
	testcase_crc32("", 0x00000000);

	testcase_crc32("33", 0x6DD28E9B);
	testcase_crc32("52", 0x5767DF55);
	testcase_crc32("B6", 0xF000F934);

	testcase_crc32("E1C2", 0x73D673CE);

	testcase_crc32("3325A4B23184AABF00", 0xED0D1B13);
	testcase_crc32("18156A211BA261A23C", 0x2C657A56);
	testcase_crc32("FFFFFFFFFFFFFFFFFF", 0xEB201890);
	testcase_crc32("00FF00AA00FF00FF00", 0x35E4DB43);

	testcase_crc32("83581CE63175C654DFD93A9290342A067158DC0F57A1108DDBD8CACE3839BDA000", 0x0A0E41CE);
}
