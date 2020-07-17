#ifdef DEVEL

#include "addressUtilsByron.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "cardano.h"
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

void testcase_deriveAddress_byron(uint32_t* path, uint32_t pathLen, uint32_t protocolMagic, const char* expectedHex)
{
	PRINTF("testcase_deriveAddressByron ");

	bip44_path_t pathSpec;
	pathSpec_init(&pathSpec, path, pathLen);

	PRINTF_bip44(&pathSpec);
	PRINTF("\n");

	uint8_t address[128];
	size_t addressSize = deriveAddress_byron(&pathSpec, protocolMagic, address, SIZEOF(address));

	uint8_t expected[100];
	size_t expectedSize = decode_hex(expectedHex, expected, SIZEOF(expected));

	EXPECT_EQ(addressSize, expectedSize);
	EXPECT_EQ_BYTES(address, expected, expectedSize);
}

void testcase_extractProtocolMagicSucceeds(const char* addressHex, uint32_t expectedProtocolMagic)
{
	PRINTF("testcase_extractProtocolMagicSucceeds\n");

	uint8_t address[100];
	size_t addressSize = decode_hex(addressHex, address, SIZEOF(address));

	uint32_t protocolMagic = extractProtocolMagic(address, addressSize);

	EXPECT_EQ(protocolMagic, expectedProtocolMagic);
}

void testcase_extractProtocolMagicThrows(const char* addressHex, uint32_t expectedErrorCode)
{
	PRINTF("testcase_extractProtocolMagicThrows\n");

	uint8_t address[100];
	size_t addressSize = decode_hex(addressHex, address, SIZEOF(address));

	EXPECT_THROWS(extractProtocolMagic(address, addressSize), expectedErrorCode);
}

void testAddressDerivation()
{
#define TESTCASE(path_, protocolMagic_, expected_) \
	{ \
		uint32_t path[] = { UNWRAP path_ }; \
		testcase_deriveAddress_byron(path, ARRAY_LEN(path), protocolMagic_, expected_); \
	}

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, 55), MAINNET_PROTOCOL_MAGIC,
	        "82d818582183581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a0001ad7ed912f"
	);
	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, HD + 26), MAINNET_PROTOCOL_MAGIC,
	        "82d818582183581c49dda88b3cdb4c9b60ad35699b2795a446120a2460f1a789c6152ce2a0001a00fb5684"
	);

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, HD + 26, 32, 54, 61), MAINNET_PROTOCOL_MAGIC,
	        "82d818582183581c7cef664fa8b2c01a9d31cfdc2b0ed662b3b16be4f5bcaf783c24c729a0001aab55fb52"
	);

	TESTCASE(
	        (HD + 44, HD + 1815, HD + 0, 1, 55), 42,
	        "82d818582583581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a10242182a001a2b7c56f6"
	);
#undef TESTCASE
}

void testProtocolMagicExtractionSucceeds()
{
#define TESTCASE(addressHex_, expected_) \
	{ \
		char addressHex[] = addressHex_; \
		testcase_extractProtocolMagicSucceeds(addressHex, expected_); \
	}

	TESTCASE(
	        "82d818582183581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a0001ad7ed912f",
			MAINNET_PROTOCOL_MAGIC
	);

	TESTCASE(
			"82d818584283581cd2348b8ef7b8a6d1c922efa499c669b151eeef99e4ce3521e88223f8a101581e581cf281e648a89015a9861bd9e992414d1145ddaf80690be53235b0e2e5001a19983465",
			MAINNET_PROTOCOL_MAGIC
	);

	TESTCASE(
	        "82d818584983581c9c708538a763ff27169987a489e35057ef3cd3778c05e96f7ba9450ea201581e581c9c1722f7e446689256e1a30260f3510d558d99d0c391f2ba89cb697702451a4170cb17001a6979126c",
			1097911063
	);

	TESTCASE(
	        "82d818582583581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a10242182a001a2b7c56f6",
			42
	);
#undef TESTCASE
}

void testProtocolMagicExtractionThrows()
{
#define TESTCASE(addressHex_, expectedErrorCode_) \
	{ \
		char addressHex[] = addressHex_; \
		testcase_extractProtocolMagicThrows(addressHex, expectedErrorCode_); \
	}

	// invalid CBOR
	TESTCASE(
	        "deadbeef",
			ERR_UNEXPECTED_TOKEN
	);

	// shelley address
	TESTCASE(
	        "035a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b31d227aefa4b773149170885aadba30aab3127cc611ddbc4999def61c",
			ERR_INVALID_DATA
	);

	// mainnet protocol magic explicitly encoded
	TESTCASE(
	        "82d818582883581ca1eda96a9952a56c983d9f49117f935af325e8a6c9d38496e945faa8a102451a2d964a09001a099ade84",
			ERR_INVALID_DATA
	);

	// too many keys in address attributes
	TESTCASE(
	        "82d818583183581ca1eda96a9952a56c983d9f49117f935af325e8a6c9d38496e945faa8a40142182f0242182f0342182f0442182f001a965d526c",
			ERR_INVALID_DATA
	);

	// address attributes value not bytes
	TESTCASE(
	        "82d818582483581ca1eda96a9952a56c983d9f49117f935af325e8a6c9d38496e945faa8a101182f001a1abe13ed",
			ERR_INVALID_DATA
	);

	// invalid crc32 checksum
	TESTCASE(
	        "82d818582183581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a0001ad7ed912e",
			ERR_INVALID_DATA
	);
#undef TESTCASE
}


void run_addressUtilsByron_test()
{
	testAddressDerivation();
	testProtocolMagicExtractionSucceeds();
	testProtocolMagicExtractionThrows();
}

#endif
