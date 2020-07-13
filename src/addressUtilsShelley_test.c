#ifdef DEVEL

#include "addressUtilsShelley.h"
#include "test_utils.h"
#include "hex_utils.h"
#include "bip44.h"

// Note(ppershing): Used in macros to have (parenthesis) => {initializer} magic
#define UNWRAP(...) __VA_ARGS__

#define HD HARDENED_BIP32
#define MAX_ADDRESS_LENGTH 128

static void pathSpec_init(bip44_path_t* pathSpec, const uint32_t* pathArray, uint32_t pathLength)
{
	pathSpec->length = pathLength;
	os_memmove(pathSpec->path, pathArray, pathLength * 4);
}

static void testcase_deriveAddressShelley(
        uint8_t header, const uint32_t* spendingPathArray, size_t spendingPathLen,
        uint8_t stakingChoice, const uint32_t* stakingPathArray, size_t stakingPathLen,
        const char* stakingKeyHashHex, const blockchainPointer_t* stakingKeyBlockchainPointer,
        const char* expectedHex)
{
	// avoid inconsistent tests
	switch (stakingChoice) {
	case NO_STAKING:
		ASSERT(stakingPathLen == 0 && stakingKeyHashHex == NULL && stakingKeyBlockchainPointer == NULL);
		break;
	case STAKING_KEY_PATH:
		ASSERT(stakingPathLen != 0 && stakingKeyHashHex == NULL && stakingKeyBlockchainPointer == NULL);
		break;
	case STAKING_KEY_HASH:
		ASSERT(stakingPathLen == 0 && stakingKeyHashHex != NULL && stakingKeyBlockchainPointer == NULL);
		break;
	case BLOCKCHAIN_POINTER:
		ASSERT(stakingPathLen == 0 && stakingKeyHashHex == NULL && stakingKeyBlockchainPointer != NULL);
		break;
	default:
		ASSERT(false);
	}

	shelleyAddressParams_t params = {
		.header = header,
		.stakingChoice = stakingChoice
	}; // the rest is initialized to zero
	pathSpec_init(&params.spendingKeyPath, spendingPathArray, spendingPathLen);
	if (stakingPathLen > 0)
		pathSpec_init(&params.stakingKeyPath, stakingPathArray, stakingPathLen);
	if (stakingKeyHashHex != NULL) {
		ASSERT(strlen(stakingKeyHashHex) == 2 * PUBLIC_KEY_HASH_LENGTH);
		parseHexString(stakingKeyHashHex, params.stakingKeyHash, SIZEOF(params.stakingKeyHash));
	}
	if (stakingKeyBlockchainPointer != NULL) {
		params.stakingKeyBlockchainPointer = *stakingKeyBlockchainPointer;
	}

	PRINTF("testcase_deriveAddressShelley 0x%02x ", header);
	bip44_PRINTF(&params.spendingKeyPath);
	if (params.stakingKeyPath.length > 0)
		bip44_PRINTF(&params.stakingKeyPath);
	if (stakingKeyHashHex != NULL)
		PRINTF(" %s", stakingKeyHashHex);
	if (stakingKeyBlockchainPointer != NULL)
		PRINTF(" (%d, %d, %d)", stakingKeyBlockchainPointer->blockIndex, stakingKeyBlockchainPointer->txIndex, stakingKeyBlockchainPointer->certificateIndex);
	PRINTF("\n");

	uint8_t address[MAX_ADDRESS_LENGTH];
	size_t addressSize = deriveAddress_shelley(&params, address, SIZEOF(address));;

	uint8_t expected[MAX_ADDRESS_LENGTH];
	size_t expectedSize = parseHexString(expectedHex, expected, SIZEOF(expected));

	EXPECT_EQ(addressSize, expectedSize);
	EXPECT_EQ_BYTES(address, expected, expectedSize);
}

static void testAddressDerivation()
{
#define NO_STAKING_KEY_PATH ()
#define NO_STAKING_KEY_HASH NULL
#define TESTCASE(header_, spendingPath_, stakingChoice_, stakingPath_, stakingKeyHashHex_, expected_) \
	{ \
		uint32_t spendingPath[] = { UNWRAP spendingPath_ }; \
		uint32_t stakingPath[] = { UNWRAP stakingPath_ }; \
		testcase_deriveAddressShelley(header_, spendingPath, ARRAY_LEN(spendingPath), stakingChoice_, stakingPath, ARRAY_LEN(stakingPath), stakingKeyHashHex_, NULL, expected_); \
	}

	TESTCASE(
	        BYRON | 0x00, (HD + 44, HD + 1815, HD + 0, 1, 55),
	        NO_STAKING, NO_STAKING_KEY_PATH, NO_STAKING_KEY_HASH,
	        "82d818582183581cb1999ee43d0c3a9fe4a1a5d959ae87069781fbb7f60ff7e8e0136881a0001ad7ed912f"
	);

	TESTCASE(
	        BASE | 0x03, (HD + 1852, HD + 1815, HD + 0, 0, 1),
	        STAKING_KEY_PATH, (HD + 1852, HD + 1815, HD + 0, 2, 0), NO_STAKING_KEY_HASH,
	        "035a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b31d227aefa4b773149170885aadba30aab3127cc611ddbc4999def61c"
	        // bech32: addr1qdd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vcayfawlf9hwv2fzuygt2km5v92kvf8e3s3mk7ynxw77cwqdquehe
	);
	TESTCASE(
	        BASE | 0x00, (HD + 1852, HD + 1815, HD + 0, 0, 1),
	        STAKING_KEY_PATH, (HD + 1852, HD + 1815, HD + 0, 2, 0), NO_STAKING_KEY_HASH,
	        "005a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b31d227aefa4b773149170885aadba30aab3127cc611ddbc4999def61c"
	        // bech32: addr1qpd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vcayfawlf9hwv2fzuygt2km5v92kvf8e3s3mk7ynxw77cwqhn8sgh
	);
	TESTCASE(
	        BASE | 0x00, (HD + 1852, HD + 1815, HD + 0, 0, 1),
	        STAKING_KEY_HASH, NO_STAKING_KEY_PATH, "1d227aefa4b773149170885aadba30aab3127cc611ddbc4999def61c",
	        "005a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b31d227aefa4b773149170885aadba30aab3127cc611ddbc4999def61c"
	        // bech32: addr1qpd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vcayfawlf9hwv2fzuygt2km5v92kvf8e3s3mk7ynxw77cwqhn8sgh
	);
	TESTCASE(
	        BASE | 0x03, (HD + 1852, HD + 1815, HD + 0, 0, 1),
	        STAKING_KEY_HASH, NO_STAKING_KEY_PATH, "122a946b9ad3d2ddf029d3a828f0468aece76895f15c9efbd69b4277",
	        "035a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b3122a946b9ad3d2ddf029d3a828f0468aece76895f15c9efbd69b4277"
	        // bech32: addr1qdd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vcj922xhxkn6twlq2wn4q50q352annk3903tj00h45mgfmswz93l5
	);

	TESTCASE(
	        ENTERPRISE | 0x00, (HD + 1852, HD + 1815, HD + 0, 0, 1), NO_STAKING, NO_STAKING_KEY_PATH, NO_STAKING_KEY_HASH,
	        "605a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b3"
	        // bech32: addr1vpd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vc93wyej
	);
	TESTCASE(
	        ENTERPRISE | 0x03, (HD + 1852, HD + 1815, HD + 0, 0, 1), NO_STAKING, NO_STAKING_KEY_PATH, NO_STAKING_KEY_HASH,
	        "635a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b3"
	        // bech32: addr1vdd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vc9wh7em
	);

	// TODO add more for REWARD etc.

#undef TESTCASE
#undef NO_STAKING_KEY_PATH
#undef NO_STAKING_KEY_HASH

#define TESTCASE_POINTER(header_, spendingPath_, stakingKeyBlockchainPointer_, expected_) \
	{ \
		uint32_t spendingPath[] = { UNWRAP spendingPath_ }; \
		blockchainPointer_t stakingKeyBlockchainPointer = { UNWRAP stakingKeyBlockchainPointer_ }; \
		testcase_deriveAddressShelley(header_, spendingPath, ARRAY_LEN(spendingPath), BLOCKCHAIN_POINTER, NULL, 0, NULL, &stakingKeyBlockchainPointer, expected_); \
	}

	TESTCASE_POINTER(
	        POINTER | 0x00, (HD + 1852, HD + 1815, HD + 0, 0, 1), (1, 2, 3),
	        "405a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b3010203"
	        // bech32: addr1gpd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vcpqgpsh506pr
	);
	TESTCASE_POINTER(
	        POINTER | 0x03, (HD + 1852, HD + 1815, HD + 0, 0, 1), (24157, 177, 42),
	        "435a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b381bc5d81312a"
	        // bech32: addr1gdd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vuph3wczvf288aeyu
	);
	TESTCASE_POINTER(
	        POINTER | 0x03, (HD + 1852, HD + 1815, HD + 0, 0, 1), (0, 0, 0),
	        "435a53103829a7382c2ab76111fb69f13e69d616824c62058e44f1a8b3000000"
	        // bech32: addr1gdd9xypc9xnnstp2kas3r7mf7ylxn4sksfxxypvwgnc63vcqqqqqnnd32q
	);

#undef TESTCASE_POINTER
}

void run_addressUtilsShelley_test()
{
	testAddressDerivation();
}

#endif
