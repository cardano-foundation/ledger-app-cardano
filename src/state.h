#ifndef H_CARDANO_APP_STATE
#define H_CARDANO_APP_STATE

#include "getVersion.h"
#include "stream.h"
#include "getExtendedPublicKey.h"
#include "deriveAddress.h"
#include "attestUtxo.h"

typedef struct {
	stream_t s;
} ins_tests_context_t;

typedef union {
	// Here should go states of all instructions
	ins_tests_context_t testsContext;
	ins_get_ext_pubkey_context_t extPubKeyContext;
	ins_derive_address_context_t deriveAddressContext;
	ins_attest_utxo_context_t attestUtxoContext;
} instructionState_t;

typedef struct {
	uint8_t currentInstruction;
} globalState_t;

extern instructionState_t instructionState;
extern globalState_t global;

#endif
