#include <os.h>
#include <stdbool.h>
#include "assert.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "base58.h"
#include "stream.h"
#include <string.h>

void run_base58_test()
{
#define TESTCASE(input_, expected_) \
	{ \
	    stream_t si; \
	    stream_init(&si); \
	    stream_appendFromHexString(&si, input_); \
		const uint8_t* inputBuffer = stream_head(&si); \
		uint8_t inputBufferSize = stream_availableBytes(&si); \
		uint8_t output[248]; \
		uint8_t outputLength = encode_base58(inputBuffer, inputBufferSize, output, sizeof(output)); \
		EXPECT_EQ(outputLength, strlen(expected_)); \
		EXPECT_EQ_BYTES(expected_, output, outputLength); \
	}

	{
		TESTCASE("", "");
	} {
		TESTCASE("ab", "3x");
		TESTCASE("16", "P");
		TESTCASE("f2", "5B");
	} {
		TESTCASE("a1b3", "DJi");
		TESTCASE("25b6", "3sT");
		TESTCASE("ffff", "LUv");
		TESTCASE("0000", "11");
	} {
		TESTCASE("82d818582183581ce63175c654dfd93a9290342a067158dc0f57a1108ddbd8cace3839bda0001a0a0e41ce",
		         "Ae2tdPwUPEZKmwoy3AU3cXb5Chnasj6mvVNxV1H11997q3VW5ihbSfQwGpm");
		TESTCASE("82d818583983581c07d99d3987090111d70b83e21c1db61acdb659d45cc1b5769a77ae11a1015655c94dbc8f2a15f95499becfbf9f2de442bce11eacd1001abd57ca7a",
		         "4swhHtxKapQbj3TZEipgtp7NQzcRWDYqCxXYoPQWjGyHmhxS1w1TjUEszCQT1sQucGwmPQMYdv1FYs3d51KgoubviPBf");
	} {
		TESTCASE("00000000ab", "11113x");
		TESTCASE("00000000df256631", "11116hpoSQ");
		TESTCASE("2536000000", "5CVj3Vq");
		TESTCASE("0000000000361200000000", "11111TvgAkW5V");
	}

#undef TESTCASE
}
