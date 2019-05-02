#include "errors.h"
#include <stdint.h>
#include <os.h>
#include "test_utils.h"
#include "stream.h"
#include <string.h>
#include "utils.h"

uint8_t hex_parseNibble(const char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	THROW(ERR_UNEXPECTED_TOKEN);
}

uint8_t hex_parseNibblePair(const char* buffer)
{
	uint8_t first = hex_parseNibble(buffer[0]);
	uint8_t second = hex_parseNibble(buffer[1]);
	return (uint8_t) ((first << 4) + second);
}

void stream_appendFromHexString(stream_t* s, const char* inStr)
{
	unsigned len = strlen(inStr);
	if (len % 2) THROW(ERR_UNEXPECTED_TOKEN);
	while (len >= 2) {
		uint8_t tmp = hex_parseNibblePair(inStr);
		stream_appendData(s, &tmp, 1);
		len -= 2;
		inStr += 2;
	}
}

size_t parseHexString(const char* inStr, uint8_t* outBuffer, size_t outMaxSize)
{

	size_t len = strlen(inStr);
	if (len % 2) THROW(ERR_UNEXPECTED_TOKEN);

	size_t outLen = len / 2;
	if (outLen > outMaxSize) {
		THROW(ERR_DATA_TOO_LARGE);
	}

	while (len >= 2) {
		*outBuffer = hex_parseNibblePair(inStr);
		len -= 2;
		inStr += 2;
		outBuffer += 1;
	}
	return outLen;
}


void stream_initFromHexString(stream_t* s, const char* inStr)
{
	stream_init(s);
	stream_appendFromHexString(s, inStr);
}

void test_hex_nibble_parsing()
{
	struct {
		char nibble;
		int value;
	} testVectors[] = {
		{'0', 0},
		{'1', 1},
		{'2', 2},
		{'3', 3},
		{'4', 4},
		{'5', 5},
		{'6', 6},
		{'7', 7},
		{'8', 8},
		{'9', 9},

		{'a', 10},
		{'b', 11},
		{'c', 12},
		{'d', 13},
		{'e', 14},
		{'f', 15},

		{'A', 10},
		{'B', 11},
		{'C', 12},
		{'D', 13},
		{'E', 14},
		{'F', 15},
	};
	PRINTF("test_hex_nibble\n");

	ITERATE(it, testVectors) {
		EXPECT_EQ(hex_parseNibble(it->nibble), it->value);
	}

	struct {
		char nibble;
	} invalidVectors[] = {
		{'\x00'}, {'\x01'},
		{'.'}, {'/'},
		{':'}, {';'},
		{'?'}, {'@'},
		{'G'}, {'H'},
		{'Z'}, {'['}, {'\\'},
		{'_'}, {'`'},
		{'g'}, {'h'},
		{'z'}, {'{'},
		{127}, {128u},
		{255u},
	};
	PRINTF("test_hex_nibble invalid\n");
	ITERATE(it, invalidVectors) {
		EXPECT_THROWS(hex_parseNibble(it->nibble), ERR_UNEXPECTED_TOKEN);
	}
}

void test_hex_parsing()
{
	struct {
		char* hex;
		uint8_t raw;
	} testVectors[] = {
		{"ff", 0xff},
		{"00", 0x00},
		{"1a", 0x1a},
		{"2b", 0x2b},
		{"3c", 0x3c},
		{"4d", 0x4d},
		{"5f", 0x5f},
		{"98", 0x98},
	};

	PRINTF("test_hex_parsing\n");
	ITERATE(it, testVectors) {
		EXPECT_EQ(hex_parseNibblePair(PTR_PIC(it->hex)), it->raw);
	}
}

void run_hex_test()
{
	test_hex_nibble_parsing();
	test_hex_parsing();
}
