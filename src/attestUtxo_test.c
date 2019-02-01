#ifdef DEVEL

#include <os.h>
#include "attestUtxo.h"
#include "errors.h"
#include "hex_utils.h"
#include "io.h"
#include "test_utils.h"
#include "endian.h"
#include "utils.h"
#include "cardano.h"

void test_attest(const char** txChunksHex, uint32_t numChunks, uint32_t outputIndex, uint64_t expectedAmount)
{
	PRINTF("test_attest %d\n", outputIndex);
	// TODO(ppershing): this is too big for stack!
	attest_utxo_parser_state_t state;

	parser_init(&state, outputIndex);
	for (unsigned i = 0; i < numChunks; i++) {
		stream_appendFromHexString(& state.stream, PTR_PIC(txChunksHex[i]));
		BEGIN_TRY {
			TRY {
				parser_keepParsing(&state);
			}
			CATCH(ERR_NOT_ENOUGH_INPUT)
			{
			}
			FINALLY {
			}
		} END_TRY;
	}
	EXPECT_EQ(parser_getAttestedAmount(&state), expectedAmount);
}

void run_test_attestUtxo()
{
	// https://cardanoexplorer.com/tx/f33b1f56240c9f4afc9dd9a9141737b2937b6cd856dd67fda81cc794d2670580
	const char* tx[] = {
		"839f8200d818582482582034bbdf0a10e7290ad22e3ee791b6b3c35c206ab8b5",
		"1bb749a2b06489ceebf5f400ff9f8282d818584283581c5f5bee73ed41ff6c84",
		"90dfdb4732178e0216ccf7badbe1e77d5d7ff8a101581e581c1e9a0361bdc37d",
		"b7ab7ea2a3f187761877f3db11211fc7436131f15e001ab10129441b00000185",
		"ae645c2d8282d818584283581cada4052647c47745abfc9e04d7dc5c5c0a8542",
		"8f5b741be6687e6005a101581e581cd8669b0c1a9f2fccb28d3ef58ef8efad73",
		"aead7117b6559a5f857813001acdb5f5841a1633e6208282d818584283581c65",
		"32caadc0b498be1813d12f33bf81d68d5662255cc640b881a29315a101581e58",
		"1cca3e553c9c63c580936df7433aac461e4efb6ce966206e083af22d0e001a9c",
		"7427f71a19cf10348282d818584283581c6fd85cfe0ae8c346552717424229d5",
		"ac928e72b0cbd5587a5d9bd8e5a101581e581c2b0b011ba3683d2eb420332a08",
		"4fe7ecbdefa204c415cd7aa17e216d001a1c29005f1ac38bbcf88282d8185842",
		"83581c431923e34d95851fba3c88e99d9d366eb1d595e5436c68da1b4699a5a1",
		"01581e581c3054e511bd5acd29e7540b417600367915afa6f95b1a40246aa4fc",
		"9f001af7fec6a71a794fa104ffa0",
	};

	const struct {
		const char** txChunksHex;
		uint32_t chunksLen;
		uint32_t outputIndex;
		uint64_t expectedAmount;
	} testVectors[] = {
#define TX tx, ARRAY_LEN(tx)
		{TX, 0, 1673668090925},
		{TX, 1,     372500000},
		{TX, 2,     433000500},
		{TX, 3,    3280715000},
		{TX, 4,    2035261700},
		{TX, 5, LOVELACE_INVALID},
#undef TX
	};

	ITERATE(it, testVectors) {
		test_attest(it->txChunksHex, it->chunksLen, it->outputIndex, it->expectedAmount);
	}
}

#endif
