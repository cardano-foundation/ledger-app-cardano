#include "cbor.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "stream.h"
#include <os.h>
#include <string.h>
#include "hash.h"
#include "utils.h"

void run_blake2b_test()
{
#define TESTCASE(input_, inputChunks_, chunksLength_, expected_) \
	{ \
		stream_t si; \
		stream_initFromHexString(&si, input_); \
		const uint8_t* inputBuffer = stream_head(&si); \
		stream_t se; \
		stream_initFromHexString(&se, expected_); \
		const uint8_t* expectedBuffer = stream_head(&se); \
		blake2b512_context_t ctx; \
		blake2b512_init(&ctx); \
		uint8_t output[64]; \
		uint8_t pos = 0; \
		for (unsigned i = 0; i < chunksLength_; i++) { \
			unsigned len = inputChunks_[i]; \
			blake2b512_append(&ctx, inputBuffer+pos, len); \
			pos += len; \
		} \
		blake2b512_finalize(&ctx, output, 64); \
		EXPECT_EQ_BYTES(expectedBuffer, output, 64); \
	}

	{
		int chunks0[0];

		TESTCASE(
		        "",
		        chunks0,
		        ARRAY_LEN(chunks0),
		        "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"
		        "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce"
		);

		int chunks1[] = {1};
		TESTCASE(
		        "52",
		        chunks1,
		        ARRAY_LEN(chunks1),
		        "329a583d5c11e03e3e1df966dbb28beea395e013bbcc752e9fdec40ab661dd1e"
		        "5e5e4ed0cff7b4125d64b3bc7da4762415296d3b5e964d780fe842a4cfca71b0"
		);

		int chunks2[] = {0, 1, 0};
		TESTCASE(
		        "52",
		        chunks2,
		        ARRAY_LEN(chunks2),
		        "329a583d5c11e03e3e1df966dbb28beea395e013bbcc752e9fdec40ab661dd1e"
		        "5e5e4ed0cff7b4125d64b3bc7da4762415296d3b5e964d780fe842a4cfca71b0"
		);
	} {
		int chunks0[] = {12};

		TESTCASE(
		        "5231ab4652eaeaea25369217",
		        chunks0,
		        ARRAY_LEN(chunks0),
		        "f9c3aa38dd69d2692f24584da906014a08d8c75cc764477e46476074ea96bf18"
		        "1bb1d705ce2c4cd901c8f0f38ef03c47971a275c1a9a58495060d6aea5c72481"
		);

		int chunks1[] = {1, 0, 1, 0, 3, 4, 1, 2};

		TESTCASE(
		        "5231ab4652eaeaea25369217",
		        chunks1,
		        ARRAY_LEN(chunks1),
		        "f9c3aa38dd69d2692f24584da906014a08d8c75cc764477e46476074ea96bf18"
		        "1bb1d705ce2c4cd901c8f0f38ef03c47971a275c1a9a58495060d6aea5c72481"
		);
	}
#undef TESTCASE

#define TESTCASE(input_, expected_) \
	{ \
		stream_t si, se; \
		stream_initFromHexString(&si, input_); \
		const uint8_t* inputBuffer = stream_head(&si); \
		stream_initFromHexString(&se, expected_); \
		const uint8_t* expectedBuffer = stream_head(&se); \
		uint8_t output[28]; \
		blake2b224_hash(inputBuffer, stream_availableBytes(&si), output, 28); \
		EXPECT_EQ_BYTES(expectedBuffer, output, 28); \
	}

	{
		TESTCASE(
		        // long input
		        "5e4b43b19363f6e314819542d6e4ee1c383dd62dbc94c86a8634391e3b120b38"
		        "8c49810e638fcc359444637e9679137b463bfd1126bcdb6f877f3c90a57c353a"
		        "9ecb26b26e5bf15e58c37b83b63b01fc8ec1082f6624f998241e2dd11f3cc2b7"
		        "e7e4af2ceb822f11f02ad7fb2aa2822f880da89ea825e0557708def47ea3d88a",

		        "8f9153cd38d46d90d4e88a7701af5f9fddb672d70c2ce6dc5face6e3"
		)
	}

#undef TESTCASE
}
