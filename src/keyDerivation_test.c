#include "keyDerivation.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "stream.h"
#include <string.h>
#include "errors.h"
#include "utils.h"
#include "state.h"


// Note(ppershing): Used in macros to have (parenthesis) => {initializer} magic
#define UNWRAP(...) __VA_ARGS__

void PRINTF_PATH(const uint32_t* path, uint32_t pathLen)
{
	for (uint32_t i = 0; i < pathLen; i++) {
		uint32_t id = path[i];
		if (id & HARDENED_BIP32) {
			PRINTF("/%d'", id & ~HARDENED_BIP32);
		} else {
			PRINTF("/%d", id);
		}
	}
}

void testcase_derivePrivateKey(uint32_t* path, uint32_t pathLen, const char* expectedHex)
{
	PRINTF("testcase_derivePrivateKey ");
	PRINTF_PATH(path, pathLen);
	PRINTF("\n");

	path_spec_t pathSpec;
	pathSpec.length = pathLen;
	os_memmove(pathSpec.path, path, MAX_PATH_LENGTH * 4);

	uint8_t expected[64];
	parseHexString(expectedHex, expected, 64);
	chain_code_t chainCode;
	privateKey_t privateKey;
	derivePrivateKey(&pathSpec, &chainCode, &privateKey);
	EXPECT_EQ_BYTES(expected, privateKey.d, 64);
}

void testPrivateKeyDerivation()
{

#define HD HARDENED_BIP32

#define TESTCASE(path_, expectedHex_) \
	{ \
	    uint32_t path[] = { UNWRAP path_ }; \
	    testcase_derivePrivateKey(path, ARRAY_LEN(path), expectedHex_); \
	}


	// Note: Failing tests here? Did you load testing mnemonic
	// "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"?

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1),

	        "5878bdf06af259f1419324680c4d4ce05b4343895b697144d874749925e69d41"
	        "648e7e50061809207f81dac36b73cd8fab2325d49d26bc65ebbb696ea59c45b8"
	);

	TESTCASE(
	        (HD + 44, HD+ 1815, HD + 1, HD + 55 ),

	        "38996766f3a49d987f37b694d170eef5866397f71fe0c50108f4b3db27e69d41"
	        "54a57702a5bf1fde836fa243f7009e665e4e902a70699ee5e82187afc425cc6a"
	);

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1, 31),

	        "0058ea2c7e34f781d12b0cee8d8a5aff4a28436b26137e7f8bcc1fc42ae69d41"
	        "0aaccec99effc497b55fbd3c4ce53a3484a34e6e4b744975f9a415c1c82ecdc3"
	);


	TESTCASE(
	        ( HD + 44, HD + 1815, HD + 1, HD + 31, 25, HD + 44 ),

	        "8038f4cd293028cca6eadf4d282bedbb6ed0e7d2e4c10128b5cb621f30e69d41"
	        "ff4a4b14953d542c2350e0d580a224ca628577b619351df3508ed42326343a4a"
	);
#undef TESTCASE

#define TESTCASE(path_, error_) \
	{ \
	    uint32_t path[] = { UNWRAP path_ }; \
	    EXPECT_THROWS( testcase_derivePrivateKey(path, ARRAY_LEN(path), ""), error_ ); \
	}

	TESTCASE( (HD + 44, HD + 1815), ERR_INVALID_REQUEST_PARAMETERS);
	TESTCASE( (HD + 44, 1815, HD + 1), ERR_INVALID_REQUEST_PARAMETERS);
	TESTCASE( (HD + 44, HD + 33, HD + 1), ERR_INVALID_REQUEST_PARAMETERS);
#undef TESTCASE
}

void testcase_derivePublicKey(uint32_t* path, uint32_t pathLen, const char* expected)
{
	PRINTF("testcase_derivePublicKey ");
	PRINTF_PATH(path, pathLen);
	PRINTF("\n");

	path_spec_t pathSpec;
	pathSpec.length = pathLen;
	os_memmove(pathSpec.path, path, MAX_PATH_LENGTH * 4);

	chain_code_t chainCode;
	privateKey_t privateKey;
	derivePrivateKey(&pathSpec, &chainCode, &privateKey);
	cx_ecfp_public_key_t publicKey;
	deriveRawPublicKey(&privateKey, &publicKey);
	uint8_t publicKeyRaw[32];
	extractRawPublicKey(&publicKey, publicKeyRaw, 32);

	uint8_t expectedBuffer[32];
	parseHexString(expected, expectedBuffer, 32);
	EXPECT_EQ_BYTES(expectedBuffer, publicKeyRaw, 32);
}

void testPublicKeyDerivation()
{
#define TESTCASE(path_, expectedHex_) \
	{ \
	    uint32_t path[] = { UNWRAP path_ }; \
	    testcase_derivePublicKey(path, ARRAY_LEN(path), expectedHex_); \
	}

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1),

	        "eb6e933ce45516ac7b0e023de700efae5e212ccc6bf0fcb33ba9243b9d832827"
	)

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1, 1615),
	        "f1fd3cebbacec0f5ac321328e74b565b38f853839d7e98f1e5ee6cf3131ab86b"
	);


	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1, HD + 15),
	        "866276e301fb7862406fef97d92fc8e712a24b821efc11e17fdba916b8fbdd55"
	);


	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1, HD + 23, 189),
	        "3aba387352d588040e33046bfd5d856210e5324f91cb5d6b710b23e65a096400"
	);

#undef TESTCASE
}


void testcase_deriveChainCode(uint32_t* path, uint32_t pathLen, const char* expectedHex)
{
	PRINTF("testcase_deriveChainCode ");
	PRINTF_PATH(path, pathLen);
	PRINTF("\n");

	chain_code_t chainCode;
	privateKey_t privateKey;

	path_spec_t pathSpec;
	pathSpec.length = pathLen;
	os_memmove(pathSpec.path, path, MAX_PATH_LENGTH * 4);

	derivePrivateKey(&pathSpec, &chainCode, &privateKey);
	uint8_t expectedBuffer[32];
	parseHexString(expectedHex, expectedBuffer, 32);
	EXPECT_EQ_BYTES(expectedBuffer, chainCode.code, 32);
}

// not tested
void testChainCodeDerivation()
{
#define TESTCASE(path_, expectedHex_) \
	{ \
	    uint32_t path[] = { UNWRAP path_ }; \
	    testcase_deriveChainCode(path, ARRAY_LEN(path), expectedHex_); \
	}

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1),
	        "0b161cb11babe1f56c3f9f1cbbb7b6d2d13eeb3efa67205198a69b8d81885354"
	);

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 1, HD + 31, 25, HD + 44),
	        "092979eec36b62a569381ec14ba33e65f3b80f40e28333f5bbc34577ad2c305d"
	);

#undef TESTCASE
}

void testcase_deriveAddress(uint32_t* path, uint32_t pathLen, const char* expected)
{
	PRINTF("testcase_deriveAddress ");
	PRINTF_PATH(path, pathLen);
	PRINTF("\n");

	path_spec_t pathSpec;
	pathSpec.length = pathLen;
	os_memmove(pathSpec.path, path, MAX_PATH_LENGTH * 4);

	uint8_t address[128];
	os_memset(address, 0, 128);
	uint32_t length = deriveAddress(&pathSpec, address, SIZEOF(address));

	address[length] = 0;

	EXPECT_EQ(length, strlen(expected));
	EXPECT_EQ_BYTES(address, expected, length);
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
	        "Ae2tdPwUPEZEXaRTix2HTUK2MwuQbRvuPjmjBGFbhBg1VU5e7rGabXepb5Q"
	);
	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, HD + 26),
	        "Ae2tdPwUPEZ4BF2hi7DZC6V3fsvT7pKPm8Yg1yEyXGcTFhGYST4MGChhvy1"
	);

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, HD + 26, 32, 54, 61),
	        "Ae2tdPwUPEZ9Gnw58m5cssQm3YNvvZQYHntLLj999XZdp7okWztXFAyFsBw"
	);
#undef TESTCASE
}

void run_key_derivation_test()
{
	PRINTF("Running key derivation tests\n");
	PRINTF("If they fail, make sure you seeded your device with\n");
	PRINTF("12-word mnemonic: 11*abandon about\n");
	testPrivateKeyDerivation();
	testPublicKeyDerivation();
	testChainCodeDerivation();
	testAddressDerivation();
}
