#ifdef DEVEL

#include "crc32.h"
#include "test_utils.h"
#include "utils.h"
#include "stream.h"
#include "hex_utils.h"


void run_crc32_test()
{
	const struct {
		const char* inputHex;
		const uint32_t expected;
	} testVectors[] = {
		{"", 0x00000000},

		{"33", 0x6DD28E9B},
		{"52", 0x5767DF55},
		{"B6", 0xF000F934},

		{"E1C2", 0x73D673CE},

		{"3325A4B23184AABF00", 0xED0D1B13},
		{"18156A211BA261A23C", 0x2C657A56},
		{"FFFFFFFFFFFFFFFFFF", 0xEB201890},
		{"00FF00AA00FF00FF00", 0x35E4DB43},

		{"83581CE63175C654DFD93A9290342A067158DC0F57A1108DDBD8CACE3839BDA000", 0x0A0E41CE},
	};
	ITERATE(it, testVectors) {
		PRINTF("testcase_crc32 %s\n", PTR_PIC(it->inputHex));
		uint8_t buffer[100];
		size_t bufferSize = parseHexString(PTR_PIC(it->inputHex), buffer, 100);
		uint32_t result = crc32(buffer, bufferSize);

		EXPECT_EQ(result, it->expected);
	}
}

#endif
