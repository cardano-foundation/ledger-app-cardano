#include "cbor.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "stream.h"
#include <os.h>
#include <string.h>

void run_blake2b_test()
{
#define ARRAYLEN(array) (sizeof(array) / sizeof(array[0]))
#define TESTCASE(input_, inputChunks_, chunksLength_, expected_) \
	{ \
		stream_t si; \
		stream_init(&si); \
		stream_appendFromHexString(&si, input_); \
		const uint8_t* inputBuffer = stream_head(&si); \
		stream_t se; \
		stream_init(&se); \
		stream_appendFromHexString(&se, expected_); \
		const uint8_t* expectedBuffer = stream_head(&se); \
		cx_blake2b_t hash; \
		cx_blake2b_init( \
			&hash, \
			64 * 8 /* output size in BITS */\
		); \
		uint8_t output[64]; \
		uint8_t pos = 0; \
		for (unsigned i = 0; i < chunksLength_; i++) { \
			cx_hash( \
				&hash.header, \
				0, /* Do not output the hash, yet */ \
				inputBuffer + pos, \
				inputChunks_[i], /* Input length */ \
				NULL, 0 \
			); \
			pos += inputChunks_[i]; \
		} \
		cx_hash( \
			&hash.header, \
			CX_LAST, /* Output the hash */ \
			NULL, \
			0, \
			output, \
			64 /* output length */ \
		); \
		EXPECT_EQ_BYTES(expectedBuffer, output, 64); \
	}

	{
		int chunks0[0];

		TESTCASE(
		        "",
		        chunks0,
		        ARRAYLEN(chunks0),
		        "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"
		        "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce"
		);

		int chunks1[] = {1};
		TESTCASE(
		        "52",
		        chunks1,
		        ARRAYLEN(chunks1),
		        "329a583d5c11e03e3e1df966dbb28beea395e013bbcc752e9fdec40ab661dd1e"
		        "5e5e4ed0cff7b4125d64b3bc7da4762415296d3b5e964d780fe842a4cfca71b0"
		);

		int chunks2[] = {0, 1, 0};
		TESTCASE(
		        "52",
		        chunks2,
		        ARRAYLEN(chunks2),
		        "329a583d5c11e03e3e1df966dbb28beea395e013bbcc752e9fdec40ab661dd1e"
		        "5e5e4ed0cff7b4125d64b3bc7da4762415296d3b5e964d780fe842a4cfca71b0"
		);
	} {
		int chunks0[] = {12};

		TESTCASE(
		        "5231ab4652eaeaea25369217",
		        chunks0,
		        ARRAYLEN(chunks0),
		        "f9c3aa38dd69d2692f24584da906014a08d8c75cc764477e46476074ea96bf18"
		        "1bb1d705ce2c4cd901c8f0f38ef03c47971a275c1a9a58495060d6aea5c72481"
		);

		int chunks1[] = {1, 0, 1, 0, 3, 4, 1, 2};

		TESTCASE(
		        "5231ab4652eaeaea25369217",
		        chunks1,
		        ARRAYLEN(chunks1),
		        "f9c3aa38dd69d2692f24584da906014a08d8c75cc764477e46476074ea96bf18"
		        "1bb1d705ce2c4cd901c8f0f38ef03c47971a275c1a9a58495060d6aea5c72481"
		);
	}
#undef TESTCASE
#undef ARRAYLEN
}