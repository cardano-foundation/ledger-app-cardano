#ifndef H_CARDANO_APP_STATE
#define H_CARDANO_APP_STATE

#include "getVersion.h"
#include "stream.h"
#include "getExtendedPublicKey.h"
#include "deriveAddress.h"
#include "signTx.h"
#include "attestUtxo.h"

typedef struct {
	stream_t s;
} ins_tests_context_t;

typedef union {
	// Here should go states of all instructions
	ins_tests_context_t testsContext;
	ins_get_ext_pubkey_context_t extPubKeyContext;
	ins_derive_address_context_t deriveAddressContext;
	ins_sign_tx_context_t signTxContext;
	ins_attest_utxo_context_t attestUtxoContext;
} instructionState_t;

// Note(instructions are uint8_t but we have a special INS_NONE value
extern int currentInstruction;

extern instructionState_t instructionState;

#endif
