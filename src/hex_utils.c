#include "errors.h"
#include <stdint.h>
#include <os.h>
#include "test_utils.h"
#include "stream.h"
#include <string.h>

uint8_t hex_parseNibble(const char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	THROW(ERR_UNEXPECTED_TOKEN);
}

uint8_t hex_parseNibblePair(const char* str)
{
	uint8_t first = hex_parseNibble(str[0]);
	uint8_t second = hex_parseNibble(str[1]);
	return (first << 4) + second;
}

void stream_appendFromHexString(stream_t* s, const char* str)
{
	unsigned len = strlen(str);
	if (len % 2) THROW(ERR_UNEXPECTED_TOKEN);
	while (len >= 2) {
		uint8_t tmp = hex_parseNibblePair(str);
		stream_appendData(s, &tmp, 1);
		len -= 2;
		str += 2;
	}
}

void test_hex_nibble_parsing()
{
#define TESTCASE(digit, value) EXPECT_EQ(hex_parseNibble(digit), value)

	TESTCASE('0', 0);
	TESTCASE('1', 1);
	TESTCASE('2', 2);
	TESTCASE('3', 3);
	TESTCASE('4', 4);
	TESTCASE('5', 5);
	TESTCASE('6', 6);
	TESTCASE('7', 7);
	TESTCASE('8', 8);
	TESTCASE('9', 9);

	TESTCASE('a', 10);
	TESTCASE('b', 11);
	TESTCASE('c', 12);
	TESTCASE('d', 13);
	TESTCASE('e', 14);
	TESTCASE('f', 15);

	TESTCASE('A', 10);
	TESTCASE('B', 11);
	TESTCASE('C', 12);
	TESTCASE('D', 13);
	TESTCASE('E', 14);
	TESTCASE('F', 15);
#undef TESTCASE

#define TESTCASE(digit) EXPECT_THROWS(hex_parseNibble(digit), ERR_UNEXPECTED_TOKEN)
	TESTCASE('\x00');
	TESTCASE('\x01');

	TESTCASE('.');
	TESTCASE('/');

	TESTCASE(':');
	TESTCASE(';');

	TESTCASE('?');
	TESTCASE('@');

	TESTCASE('G');
	TESTCASE('H');

	TESTCASE('Z');
	TESTCASE('[');
	TESTCASE('\\');

	TESTCASE('_');
	TESTCASE('`');

	TESTCASE('g');
	TESTCASE('h');

	TESTCASE('z');
	TESTCASE('{');

	TESTCASE(127);
	TESTCASE(128u);

	TESTCASE(255u);
#undef TESTCASE
}

void test_hex_parsing()
{
	const char* hex =       "ff    00    1a    2b    3c    4d    5f    98";
	const uint8_t raw[] = {0xff, 0x00, 0x1a, 0x2b, 0x3c, 0x4d, 0x5f, 0x98};

	for (unsigned i = 0; i < sizeof(raw); i++) {
		unsigned pos = i*6;
		EXPECT_EQ(hex_parseNibblePair(hex + pos), raw[i]);
	}
}

void run_hex_test()
{
	test_hex_nibble_parsing();
	test_hex_parsing();
}
