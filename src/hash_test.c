#ifdef DEVEL

#include "cbor.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "stream.h"
#include <os.h>
#include <string.h>
#include "hash.h"
#include "utils.h"


void testcase_chunks_blake2b_512(
        const char* chunksHex[],
        uint32_t chunksCount,
        const char* expectedHex
)
{
	PRINTF("testcase_chunks_blake2b_512\n");
	blake2b_512_context_t ctx;
	blake2b_512_init(&ctx);
	uint8_t outputBuffer[64];
	for (unsigned i = 0; i < chunksCount; i++) {
		uint8_t chunkBuffer[20];
		size_t chunkSize = parseHexString(PTR_PIC(chunksHex[i]), chunkBuffer, SIZEOF(chunkBuffer));

		blake2b_512_append(&ctx, chunkBuffer, chunkSize);
	}
	blake2b_512_finalize(&ctx, outputBuffer, SIZEOF(outputBuffer));

	uint8_t expectedBuffer[64];
	size_t expectedSize = parseHexString(expectedHex, expectedBuffer, SIZEOF(expectedBuffer));
	ASSERT(expectedSize == 64);

	EXPECT_EQ_BYTES(expectedBuffer, outputBuffer, SIZEOF(expectedBuffer));
}

void testcase_blake2b_224(const char* inputHex, const char* expectedHex)
{
	PRINTF("testcase_blake2b_224 %s\n", inputHex);
	uint8_t inputBuffer[200];
	size_t inputSize = parseHexString(inputHex, inputBuffer, SIZEOF(inputBuffer));

	uint8_t expectedBuffer[28];
	size_t expectedSize = parseHexString(expectedHex, expectedBuffer, SIZEOF(expectedBuffer));

	ASSERT(expectedSize == 28);
	uint8_t outputBuffer[28];
	blake2b_224_hash(inputBuffer, inputSize, outputBuffer, SIZEOF(outputBuffer));
	EXPECT_EQ_BYTES(expectedBuffer, outputBuffer, expectedSize);
}

void run_blake2b_test()
{
#define UNWRAP(...) __VA_ARGS__
#define TESTCASE_CHUNKS_BLAKE2B_512(chunks_, expected_) \
	{ \
		const char* chunks[] = { UNWRAP chunks_ }; \
		testcase_chunks_blake2b_512(chunks, ARRAY_LEN(chunks), expected_); \
	}

	{
		TESTCASE_CHUNKS_BLAKE2B_512(
		        (""),
		        "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"
		        "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce"
		);

		TESTCASE_CHUNKS_BLAKE2B_512(
		        ("52"),
		        "329a583d5c11e03e3e1df966dbb28beea395e013bbcc752e9fdec40ab661dd1e"
		        "5e5e4ed0cff7b4125d64b3bc7da4762415296d3b5e964d780fe842a4cfca71b0"
		);

		TESTCASE_CHUNKS_BLAKE2B_512(
		        ("", "52", ""),
		        "329a583d5c11e03e3e1df966dbb28beea395e013bbcc752e9fdec40ab661dd1e"
		        "5e5e4ed0cff7b4125d64b3bc7da4762415296d3b5e964d780fe842a4cfca71b0"
		);
	} {
		TESTCASE_CHUNKS_BLAKE2B_512(
		        ("5231ab4652eaeaea25369217"),
		        "f9c3aa38dd69d2692f24584da906014a08d8c75cc764477e46476074ea96bf18"
		        "1bb1d705ce2c4cd901c8f0f38ef03c47971a275c1a9a58495060d6aea5c72481"
		);

		TESTCASE_CHUNKS_BLAKE2B_512(
		        ("52", "", "31", "", "ab4652", "eaeaea25", "36", "9217"),
		        "f9c3aa38dd69d2692f24584da906014a08d8c75cc764477e46476074ea96bf18"
		        "1bb1d705ce2c4cd901c8f0f38ef03c47971a275c1a9a58495060d6aea5c72481"
		);
	}
#undef TESTCASE_CHUNKS_BLAKE2B_512
	{
		testcase_blake2b_224(
		        // long input
		        "5e4b43b19363f6e314819542d6e4ee1c383dd62dbc94c86a8634391e3b120b38"
		        "8c49810e638fcc359444637e9679137b463bfd1126bcdb6f877f3c90a57c353a"
		        "9ecb26b26e5bf15e58c37b83b63b01fc8ec1082f6624f998241e2dd11f3cc2b7"
		        "e7e4af2ceb822f11f02ad7fb2aa2822f880da89ea825e0557708def47ea3d88a",

		        "8f9153cd38d46d90d4e88a7701af5f9fddb672d70c2ce6dc5face6e3"
		);
	}
}

void run_hash_test()
{
	run_blake2b_test();
}

#endif
