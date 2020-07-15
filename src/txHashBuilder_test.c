#ifdef DEVEL

#include "common.h"
#include "cardano.h"
#include "txHashBuilder.h"
#include "hex_utils.h"
#include "test_utils.h"

/* original data from trezor

{
  "inputs": [
    {
        "path": "m/1852'/1815'/0'/0/0",
        "prev_hash": "0b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7",
        "prev_index": 0
    },
    {
        "path": "m/1852'/1815'/0'/0/0",
        "prev_hash": "1b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7",
        "prev_index": 1
    },
    {
        "path": "m/1852'/1815'/0'/0/0",
        "prev_hash": "2b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7",
        "prev_index": 2
    },
    {
        "path": "m/1852'/1815'/0'/0/0",
        "prev_hash": "3b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7",
        "prev_index": 3
    }
  ],
  "outputs": [
    {
        "addressType": 3,
        "path": "m/44'/1815'/0'/0/0",
        "amount": "100"
    },
    {
        "addressType": 0,
        "path": "m/1852'/1815'/0'/0/0",
        "stakingKeyPath": "m/1852'/1815'/0'/2/0",
        "amount": "200"
    },
    {
        "addressType": 1,
        "path": "m/1852'/1815'/0'/0/0",
        "pointer":{
          "block_index": 1000,
          "tx_index": 2000,
          "certificate_index": 3000
        },
        "amount": "300"
    },
    {
        "addressType": 2,
        "path": "m/1852'/1815'/0'/0/0",
        "amount": "400"
    },
    {
        "addressType": 2,
        "path": "m/1852'/1815'/0'/0/0",
        "amount": "500"
    }
  ],
  "certificates": [
    {
      "type": 0,
      "path": "m/1852'/1815'/0'/2/0"
    },
    {
      "type": 1,
      "path": "m/1852'/1815'/0'/2/0"
    },
    {
      "type": 1,
      "path": "m/1852'/1815'/1'/2/0"
    },
    {
      "type": 2,
      "path": "m/1852'/1815'/0'/2/0",
      "pool": "0d13015cdbcdbd0889ce276192a1601f2d4d20b8392d4ef4f9a754e2"
    },
    {
      "type": 2,
      "path": "m/1852'/1815'/0'/2/0",
      "pool": "1d13015cdbcdbd0889ce276192a1601f2d4d20b8392d4ef4f9a754e2"
    },
    {
      "type": 2,
      "path": "m/1852'/1815'/0'/2/0",
      "pool": "2d13015cdbcdbd0889ce276192a1601f2d4d20b8392d4ef4f9a754e2"
    }
  ],
  "withdrawals": [
    {
      "path": "m/1852'/1815'/0'/2/0",
      "amount": 111
    },
    {
      "path": "m/1852'/1815'/0'/2/0",
      "amount": 222
    },
    {
      "path": "m/1852'/1815'/0'/2/0",
      "amount": 333
    },
    {
      "path": "m/1852'/1815'/0'/2/0",
      "amount": 444
    },
    {
      "path": "m/1852'/1815'/0'/2/0",
      "amount": 555
    },
    {
      "path": "m/1852'/1815'/0'/2/0",
      "amount": 666
    }
  ],
  "fee": 42,
  "ttl": 235000
}

tx_body:
83a600848258200b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7008258201b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7018258202b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b7028258203b40265111d8bb3c3c608d95b3a0bf83461ace32d79336579a1939b3aad1c0b703018582582b82d818582183581c6ee5bb111c8771ce03278e624056a12c9cfb353eb112e8abf21fa4fea0001a74eee4081864825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc18c8825823409493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e87688f50973819012c82581d609493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e19019082581d609493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e1901f402182a031a000395f8048682008200581c32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc82018200581c32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc82018200581c337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c4725183028200581c32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc581c0d13015cdbcdbd0889ce276192a1601f2d4d20b8392d4ef4f9a754e283028200581c32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc581c1d13015cdbcdbd0889ce276192a1601f2d4d20b8392d4ef4f9a754e283028200581c32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc581c2d13015cdbcdbd0889ce276192a1601f2d4d20b8392d4ef4f9a754e205a1581de032c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc19029aa1008382582073fea80d424276ad0978d4fe5310e8bc2d485f5f6bb3bf87612989f112ad5a7d58404466e5f2dae2e79498e68f617db7c9309255e3d8d8c2a2934a010cd055b07cdefb843527f180467adac25568c3c2f8b26067c0695ddf425cbefa94c89df6f00e8258202c041c9c6a676ac54d25e2fdce44c56581e316ae43adc4c7bf17f23214d8d8925840840c52ed5f330f806825d42a5e7d9a13c65df65a33834768f45116771a8753daf793da32c3e812fa9dfb30d32c3d9784c2307b2eadc3af2400879dab175bf90482582009ab278d49b7b86a055185c474c4942281ddfa05a54684c7e8a6f230625aee57584031d44911d9f848bfd0bd92530ca15804a33d958d91a052948ba2da0695aa061d5ac2d98a7af00aa395f659497150ba579f26401794fa2e830c57a09dfc097702f6

tx_hash:
f38bb9a2b8dde8065135028181af57fd337df900b468603ff90f1f1d17a3192b
*/


static struct {
	const char* txHashHex;
	int index;
} inputs[] = {
	{
		"0B40265111D8BB3C3C608D95B3A0BF83461ACE32D79336579A1939B3AAD1C0B7",
		0
	},
	{
		"1B40265111D8BB3C3C608D95B3A0BF83461ACE32D79336579A1939B3AAD1C0B7",
		1
	},
	{
		"2B40265111D8BB3C3C608D95B3A0BF83461ACE32D79336579A1939B3AAD1C0B7",
		2
	},
	{
		"3B40265111D8BB3C3C608D95B3A0BF83461ACE32D79336579A1939B3AAD1C0B7",
		3
	},
};

static struct {
	const char* rawAddressHex;
	uint64_t amount;
} outputs[] = {
	{
		"82D818582183581C6EE5BB111C8771CE03278E624056A12C9CFB353EB112E8ABF21FA4FEA0001A74EEE408",
		100
	},
	{
		"009493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E32C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC",
		200
	},
	{
		"409493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E87688F509738",
		300
	},
	{
		"609493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E",
		400
	},
	{
		"609493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E",
		500
	},
};

// TODO use meaningful data of correct length for certificates
static struct {
	const char* stakingKeyHash;
} registrationCertificates[] = {
	{
		"32C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC"
	},
};

static struct {
	const char* stakingKeyHash;
} deregistrationCertificates[] = {
	{
		"32C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC"
	},
	{
		"337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251"
	},
};

static struct {
	const char* stakingKeyHash;
	const char* poolKeyHash;
} delegationCertificates[] = {
	{
		"32C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC",
		"0D13015CDBCDBD0889CE276192A1601F2D4D20B8392D4EF4F9A754E2"
	},
	{
		"32C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC",
		"1D13015CDBCDBD0889CE276192A1601F2D4D20B8392D4EF4F9A754E2"
	},
	{
		"32C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC",
		"2D13015CDBCDBD0889CE276192A1601F2D4D20B8392D4EF4F9A754E2"
	},
};

static struct {
	const char* rewardAccount;
	uint64_t amount;
} withdrawals[] = {
	{
		"E032C728D3861E164CAB28CB8F006448139C8F1740FFB8E7AA9E5232DC",
		666
	}
};

static const char* expectedHex = "f38bb9a2b8dde8065135028181af57fd337df900b468603ff90f1f1d17a3192b";


void run_txHashBuilder_test()
{
	PRINTF("txHashBuilder test\n");
	tx_hash_builder_t builder;

	const size_t numCertificates = ARRAY_LEN(registrationCertificates) +
	                               ARRAY_LEN(deregistrationCertificates) + ARRAY_LEN(delegationCertificates);

	txHashBuilder_init(&builder, numCertificates, ARRAY_LEN(withdrawals), false);

	txHashBuilder_enterInputs(&builder, ARRAY_LEN(inputs));
	ITERATE(it, inputs) {
		uint8_t tmp[TX_HASH_LENGTH];
		size_t tmpSize = decode_hex(PTR_PIC(it->txHashHex), tmp, SIZEOF(tmp));
		txHashBuilder_addInput(
		        &builder,
		        tmp, tmpSize,
		        it->index
		);
	}

	txHashBuilder_enterOutputs(&builder, ARRAY_LEN(outputs));
	ITERATE(it, outputs) {
		uint8_t tmp[70];
		size_t tmpSize = decode_hex(PTR_PIC(it->rawAddressHex), tmp, SIZEOF(tmp));
		txHashBuilder_addOutput(
		        &builder,
		        tmp, tmpSize,
		        it->amount
		);
	}

	txHashBuilder_addFee(&builder, 42);

	txHashBuilder_addTtl(&builder, 235000);

	txHashBuilder_enterCertificates(&builder);

	ITERATE(it, registrationCertificates) {
		uint8_t tmp[70];
		size_t tmpSize = decode_hex(PTR_PIC(it->stakingKeyHash), tmp, SIZEOF(tmp));
		txHashBuilder_addCertificate_stakingKey(
		        &builder,
		        CERTIFICATE_TYPE_STAKE_REGISTRATION,
		        tmp, tmpSize
		);
	}

	ITERATE(it, deregistrationCertificates) {
		uint8_t tmp[70];
		size_t tmpSize = decode_hex(PTR_PIC(it->stakingKeyHash), tmp, SIZEOF(tmp));
		txHashBuilder_addCertificate_stakingKey(
		        &builder,
		        CERTIFICATE_TYPE_STAKE_DEREGISTRATION,
		        tmp, tmpSize
		);
	}

	ITERATE(it, delegationCertificates) {
		uint8_t tmp_credential[70];
		size_t tmpSize_credential = decode_hex(
		                                    PTR_PIC(it->stakingKeyHash),
		                                    tmp_credential, SIZEOF(tmp_credential)
		                            );
		uint8_t tmp_pool[70];
		size_t tmpSize_pool = decode_hex(PTR_PIC(it->poolKeyHash), tmp_pool, SIZEOF(tmp_pool));
		txHashBuilder_addCertificate_delegation(
		        &builder,
		        tmp_credential, tmpSize_credential,
		        tmp_pool, tmpSize_pool
		);
	}

	txHashBuilder_enterWithdrawals(&builder);

	ITERATE(it, withdrawals) {
		uint8_t tmp[70];
		size_t tmpSize = decode_hex(PTR_PIC(it->rewardAccount), tmp, SIZEOF(tmp));
		txHashBuilder_addWithdrawal(
		        &builder,
		        tmp, tmpSize,
		        it->amount
		);
	}

	{
		/* TODO add some metadata, and modify the argument to txHashBuilder_init() and the expected result

		const char metadataHashHex[] = "AA"; // TODO replace
		uint8_t tmp[70]; // TODO fix size?
		size_t tmpSize = decode_hex(metadataHashHex, tmp, SIZEOF(tmp));
		txHashBuilder_addMetadata(&builder, tmp, tmpSize);
		*/
	}

	uint8_t result[TX_HASH_LENGTH];
	txHashBuilder_finalize(&builder, result, SIZEOF(result));

	uint8_t expected[TX_HASH_LENGTH];
	decode_hex(expectedHex, expected, SIZEOF(expected));


	PRINTF("result\n");
	PRINTF("%.*H\n", 32, result);

	EXPECT_EQ_BYTES(result, expected, 32);
}

#endif
