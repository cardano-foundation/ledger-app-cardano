#include "keyDerivation.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "stream.h"
#include <string.h>
#include "errors.h"

#define ARRAYLEN(array) (sizeof(array) / sizeof(array[0]))

void testPrivateKeyDerivation()
{
#define TESTCASE(path_, pathLength_, expected_) \
	{ \
		uint8_t chainCode[32]; \
		cx_ecfp_256_extended_private_key_t privateKey; \
		derivePrivateKey(path_, pathLength_, chainCode, &privateKey); \
		stream_t s; \
		stream_init(&s); \
		stream_appendFromHexString(&s, expected_); \
		const uint8_t* expectedBuffer = stream_head(&s); \
		EXPECT_EQ_BYTES(expectedBuffer, privateKey.d, 64); \
	}

	{
		uint32_t path1[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1};
		char* expected1 = "5878bdf06af259f1419324680c4d4ce05b4343895b697144d874749925e69d41"
		                  "648e7e50061809207f81dac36b73cd8fab2325d49d26bc65ebbb696ea59c45b8";

		TESTCASE(path1, ARRAYLEN(path1), expected1);

		uint32_t path2[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, HARDENED_BIP32 + 55 };
		char* expected2 = "38996766f3a49d987f37b694d170eef5866397f71fe0c50108f4b3db27e69d41"
		                  "54a57702a5bf1fde836fa243f7009e665e4e902a70699ee5e82187afc425cc6a";

		TESTCASE(path2, ARRAYLEN(path2), expected2);

		uint32_t path3[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, 31 };
		char* expected3 = "0058ea2c7e34f781d12b0cee8d8a5aff4a28436b26137e7f8bcc1fc42ae69d41"
		                  "0aaccec99effc497b55fbd3c4ce53a3484a34e6e4b744975f9a415c1c82ecdc3";

		TESTCASE(path3, ARRAYLEN(path3), expected3);

		uint32_t path4[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, HARDENED_BIP32 + 31, 25, HARDENED_BIP32 + 44 };
		char* expected4 = "8038f4cd293028cca6eadf4d282bedbb6ed0e7d2e4c10128b5cb621f30e69d41"
		                  "ff4a4b14953d542c2350e0d580a224ca628577b619351df3508ed42326343a4a";

		TESTCASE(path4, ARRAYLEN(path4), expected4);
	} {
		uint8_t chainCode[32];
		cx_ecfp_256_extended_private_key_t privateKey;

		uint32_t tooShortPath[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815 };
		EXPECT_THROWS(derivePrivateKey(tooShortPath, ARRAYLEN(tooShortPath), chainCode, &privateKey), ERR_INVALID_REQUEST_PARAMETERS);

		uint32_t invalidPath1[] = { HARDENED_BIP32 + 44, 1815, HARDENED_BIP32 + 1 };
		EXPECT_THROWS(derivePrivateKey(invalidPath1, ARRAYLEN(invalidPath1), chainCode, &privateKey), ERR_INVALID_REQUEST_PARAMETERS);

		uint32_t invalidPath2[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 33, HARDENED_BIP32 + 1 };
		EXPECT_THROWS(derivePrivateKey(invalidPath2, ARRAYLEN(invalidPath2), chainCode, &privateKey), ERR_INVALID_REQUEST_PARAMETERS);
	}
#undef TESTCASE
}

void testPublicKeyDerivation()
{
#define TESTCASE(path_, pathLength_, expected_) \
	{ \
		uint8_t chainCode[32]; \
		cx_ecfp_256_extended_private_key_t privateKey; \
		derivePrivateKey(path_, pathLength_, chainCode, &privateKey); \
		cx_ecfp_public_key_t publicKey; \
		derivePublicKey(&privateKey, &publicKey); \
		uint8_t publicKeyRaw[32]; \
		getRawPublicKey(&publicKey, publicKeyRaw); \
		stream_t s; \
		stream_init(&s); \
		stream_appendFromHexString(&s, expected_); \
		const uint8_t* expectedBuffer = stream_head(&s); \
		EXPECT_EQ_BYTES(expectedBuffer, publicKeyRaw, 32); \
	}

	{
		uint32_t path1[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1};
		char* expected1 = "eb6e933ce45516ac7b0e023de700efae5e212ccc6bf0fcb33ba9243b9d832827";

		TESTCASE(path1, ARRAYLEN(path1), expected1);

		uint32_t path2[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, 1615};
		char* expected2 = "f1fd3cebbacec0f5ac321328e74b565b38f853839d7e98f1e5ee6cf3131ab86b";

		TESTCASE(path2, ARRAYLEN(path2), expected2);

		uint32_t path3[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, HARDENED_BIP32 + 15};
		char* expected3 = "866276e301fb7862406fef97d92fc8e712a24b821efc11e17fdba916b8fbdd55";

		TESTCASE(path3, ARRAYLEN(path3), expected3);

		uint32_t path4[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, HARDENED_BIP32 + 23, 189};
		char* expected4 = "3aba387352d588040e33046bfd5d856210e5324f91cb5d6b710b23e65a096400";

		TESTCASE(path4, ARRAYLEN(path4), expected4);
	}
#undef TESTCASE
}

// not tested
void testChainCodeDerivation()
{
#define TESTCASE(path_, pathLength_, expected_) \
	{ \
		uint8_t chainCode[32]; \
		cx_ecfp_256_extended_private_key_t privateKey; \
		derivePrivateKey(path_, pathLength_, chainCode, &privateKey); \
		stream_t s; \
		stream_init(&s); \
		stream_appendFromHexString(&s, expected_); \
		const uint8_t* expectedBuffer = stream_head(&s); \
		EXPECT_EQ_BYTES(expectedBuffer, chainCode, 32); \
	}

	{
		uint32_t path1[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1};
		char* expected1 = "0b161cb11babe1f56c3f9f1cbbb7b6d2d13eeb3efa67205198a69b8d81885354";

		TESTCASE(path1, ARRAYLEN(path1), expected1);

		uint32_t path2[] = { HARDENED_BIP32 + 44, HARDENED_BIP32 + 1815, HARDENED_BIP32 + 1, HARDENED_BIP32 + 31, 25, HARDENED_BIP32 + 44 };
		char* expected2 = "092979eec36b62a569381ec14ba33e65f3b80f40e28333f5bbc34577ad2c305d";

		TESTCASE(path2, ARRAYLEN(path2), expected2);
	}
#undef TESTCASE
}

void key_derivation_test()
{
	testPrivateKeyDerivation();
	testPublicKeyDerivation();
	testChainCodeDerivation();
}
#undef ARRAYLEN