#ifdef DEVEL

#include "common.h"
#include "txHashBuilder.h"
#include "hex_utils.h"
#include "test_utils.h"

static struct {
	const char* txHashHex;
	int index;
} inputs[] = {
	{
		"34BBDF0A10E7290AD22E3EE791B6B3C35C206AB8B51BB749A2B06489CEEBF5F4",
		0
	},
};

static struct {
	const char* rawAddressHex;
	uint64_t amount;
} outputs[] = {
	{
		"83581C5F5BEE73ED41FF6C8490DFDB4732178E0216CCF7BADBE1E77D5D7FF8A1"
		"01581E581C1E9A0361BDC37DB7AB7EA2A3F187761877F3DB11211FC7436131F1"
		"5E00",
		1673668090925
	},
	{
		"83581CADA4052647C47745ABFC9E04D7DC5C5C0A85428F5B741BE6687E6005A1"
		"01581E581CD8669B0C1A9F2FCCB28D3EF58EF8EFAD73AEAD7117B6559A5F8578"
		"1300",
		372500000
	},
	{
		"83581C6532CAADC0B498BE1813D12F33BF81D68D5662255CC640B881A29315A1"
		"01581E581CCA3E553C9C63C580936DF7433AAC461E4EFB6CE966206E083AF22D"
		"0E00",
		433000500
	},
	{
		"83581C6FD85CFE0AE8C346552717424229D5AC928E72B0CBD5587A5D9BD8E5A1"
		"01581E581C2B0B011BA3683D2EB420332A084FE7ECBDEFA204C415CD7AA17E21"
		"6D00",
		3280715000
	},
	{
		"83581C431923E34D95851FBA3C88E99D9D366EB1D595E5436C68DA1B4699A5A1"
		"01581E581C3054E511BD5ACD29E7540B417600367915AFA6F95B1A40246AA4FC"
		"9F00",
		2035261700
	},
};

static const char* expectedHex = "f33b1f56240c9f4afc9dd9a9141737b2937b6cd856dd67fda81cc794d2670580";


void run_txHashBuilder_test()
{
	PRINTF("txHashBuilder test\n");
	tx_hash_builder_t builder;
	txHashBuilder_init(&builder);

	txHashBuilder_enterInputs(&builder);
	ITERATE(it, inputs) {
		uint8_t tmp[32];
		size_t tmpSize = parseHexString(PTR_PIC(it->txHashHex), tmp, SIZEOF(tmp));
		txHashBuilder_addUtxoInput(
		        &builder,
		        tmp, tmpSize,
		        it->index
		);
	}

	txHashBuilder_enterOutputs(&builder);
	ITERATE(it, outputs) {
		uint8_t tmp[70];
		size_t tmpSize = parseHexString(PTR_PIC(it->rawAddressHex), tmp, SIZEOF(tmp));
		txHashBuilder_addOutput(
		        &builder,
		        tmp, tmpSize,
		        it->amount
		);
	}

	txHashBuilder_enterMetadata(&builder);

	uint8_t expected[32];
	parseHexString(expectedHex, expected, SIZEOF(expected));

	uint8_t result[32];
	txHashBuilder_finalize(&builder, result, SIZEOF(result));

	PRINTF("result\n");
	PRINTF("%.*H", 32, result);

	EXPECT_EQ_BYTES(result, expected, 32);
}

#endif
