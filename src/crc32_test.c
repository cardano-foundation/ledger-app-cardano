#include "crc32.h"
#include "test_utils.h"
#include "utils.h"
#include "stream.h"
#include "hex_utils.h"

void test(const char* data, uint32_t expected)
{
	stream_t s;
	stream_initFromHexString(&s, data);
	const uint8_t* buffer = stream_head(&s);

	uint32_t result = crc32(buffer, stream_availableBytes(&s));

	EXPECT_EQ(result, expected);
}

void run_crc32_test()
{
	test("", 0x00000000);

	test("33", 0x6DD28E9B);
	test("52", 0x5767DF55);
	test("B6", 0xF000F934);

	test("E1C2", 0x73D673CE);

	test("3325A4B23184AABF00", 0xED0D1B13);
	test("18156A211BA261A23C", 0x2C657A56);
	test("FFFFFFFFFFFFFFFFFF", 0xEB201890);
	test("00FF00AA00FF00FF00", 0x35E4DB43);

	test("83581CE63175C654DFD93A9290342A067158DC0F57A1108DDBD8CACE3839BDA000", 0x0A0E41CE);
}