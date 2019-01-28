#ifdef DEVEL

#include <os.h>
#include <stdbool.h>
#include "assert.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "base58.h"
#include "stream.h"
#include <string.h>
#include "utils.h"

void testcase_base58(const char* inputHex, const char* expectedStr)
{
	PRINTF("testcase_base58: %s\n", inputHex);
	uint8_t inputBuffer[100];
	size_t inputSize;
	inputSize = parseHexString(inputHex, inputBuffer, SIZEOF(inputBuffer));
	char outputStr[100];
	size_t outputLen = encode_base58(inputBuffer, inputSize, outputStr, SIZEOF(outputStr));
	EXPECT_EQ(outputLen, strlen(expectedStr));
	EXPECT_EQ_BYTES(expectedStr, outputStr, outputLen + 1);
}

void run_base58_test()
{
	struct {
		const char* inputHex;
		const char* expectedHex;
	} testVectors[] = {
		{"", ""},

		{"ab", "3x"},
		{"16", "P"},
		{"f2", "5B"},

		{"a1b3", "DJi"},
		{"25b6", "3sT"},
		{"ffff", "LUv"},
		{"0000", "11"},

		{
			"82d818582183581ce63175c654dfd93a9290342a067158dc0f57a1108ddbd8cace3839bda0001a0a0e41ce",
			"Ae2tdPwUPEZKmwoy3AU3cXb5Chnasj6mvVNxV1H11997q3VW5ihbSfQwGpm"
		},

		{
			"82d818583983581c07d99d3987090111d70b83e21c1db61acdb659d45cc1b5769a77ae11a1015655c94dbc8f2a15f95499becfbf9f2de442bce11eacd1001abd57ca7a",
			"4swhHtxKapQbj3TZEipgtp7NQzcRWDYqCxXYoPQWjGyHmhxS1w1TjUEszCQT1sQucGwmPQMYdv1FYs3d51KgoubviPBf"
		},

		{"00000000ab", "11113x"},
		{"00000000df256631", "11116hpoSQ"},
		{"2536000000", "5CVj3Vq"},
		{"0000000000361200000000", "11111TvgAkW5V"},
	};
	ITERATE(it, testVectors) {
		testcase_base58(PTR_PIC(it->inputHex), PTR_PIC(it->expectedHex));
	}
}

#endif
