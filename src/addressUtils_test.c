#ifdef DEVEL

#include "addressUtils.h"
#include "test_utils.h"
#include "hex_utils.h"
// Note(ppershing): Used in macros to have (parenthesis) => {initializer} magic
#define UNWRAP(...) __VA_ARGS__

#define HD HARDENED_BIP32

static void pathSpec_init(bip44_path_t* pathSpec, uint32_t* pathArray, uint32_t pathLength)
{
	pathSpec->length = pathLength;
	os_memmove(pathSpec->path, pathArray, pathLength * 4);
}

static void PRINTF_bip44(const bip44_path_t* pathSpec)
{
	char tmp[100];
	SIZEOF(*pathSpec);
	bip44_printToStr(pathSpec, tmp, SIZEOF(tmp));
	PRINTF("%s", tmp);
};

void testcase_deriveAddress(uint32_t* path, uint32_t pathLen, const char* expectedHex)
{
	PRINTF("testcase_deriveAddress ");

	bip44_path_t pathSpec;
	pathSpec_init(&pathSpec, path, pathLen);

	PRINTF_bip44(&pathSpec);
	PRINTF("\n");

	uint8_t address[128];
	size_t addressSize = deriveAddress(&pathSpec, address, SIZEOF(address));

	uint8_t expected[100];
	size_t expectedSize = parseHexString(expectedHex, expected, SIZEOF(expected));

	EXPECT_EQ(addressSize, expectedSize);
	EXPECT_EQ_BYTES(address, expected, expectedSize);
}

void testAddressDerivation()
{
#define TESTCASE(path_, expected_) \
	{ \
		uint32_t path[] = { UNWRAP path_ }; \
		testcase_deriveAddress(path, ARRAY_LEN(path), expected_); \
	}


	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, 55),
	        "82d818582183581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a0001ad7ed912f"
	);
	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, HD + 26),
	        "82d818582183581c49dda88b3cdb4c9b60ad35699b2795a446120a2460f1a789c6152ce2a0001a00fb5684"
	);

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, HD + 26, 32, 54, 61),
	        "82d818582183581c7cef664fa8b2c01a9d31cfdc2b0ed662b3b16be4f5bcaf783c24c729a0001aab55fb52"
	);
#undef TESTCASE
}


void run_address_utils_test()
{
	testAddressDerivation();
}

#endif
