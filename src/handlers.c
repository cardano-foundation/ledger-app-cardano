#ifndef H_HANDLERS

#include <os_io_seproxyhal.h>
#include <stdlib.h>

#include "handlers.h"
#include "getVersion.h"
#include "getExtendedPublicKey.h"
#include "runTests.h"
#include "attestUtxo.h"
#include "attestKey.h"
#include "state.h"
#include "errors.h"

// The APDU protocol uses a single-byte instruction code (INS) to specify
// which command should be executed. We'll use this code to dispatch on a
// table of function pointers.

enum {
	// 0x0* - app status calls
	INS_GET_VERSION   = 0x00,
	INS_SHOW_ABOUT    = 0x02,

	// 0x1* - public-key/address related
	INS_GET_PUB_KEY   = 0x10,

	// 0x2* - signing-transaction related
	INS_ATTEST_UTXO = 0x20,

	#ifdef DEVEL
	// 0xF* - debug_mode related
	INS_RUN_TESTS     = 0xF0,
	// 0xF1 reserved for INS_SET_HEADLESS_INTERACTION
	// session key used for attestation
	INS_GET_ATTEST_KEY = 0xF2,
	INS_SET_ATTEST_KEY = 0xF3,

	#endif
};


handler_fn_t* lookupHandler(uint8_t ins)
{
	if (global.currentInstruction && global.currentInstruction != ins) {
		THROW(ERR_STILL_IN_CALL);
	}
	if (!global.currentInstruction) {
		global.currentInstruction = ins;
		os_memset(&instructionState, 0, sizeof(instructionState));
	}

	switch (ins) {

	case INS_GET_VERSION:
		return handleGetVersion;
	case INS_SHOW_ABOUT:
		return handleShowAbout;

	case INS_GET_PUB_KEY:
		return handleGetExtendedPublicKey;


	case INS_ATTEST_UTXO:
		return handle_attestUtxo;

// *INDENT-OFF* astyle has problems with #define inside switch
#ifdef DEVEL
	case INS_RUN_TESTS:
		return handleRunTests;

	case INS_GET_SESSION_KEY:
		return handleGetAttestKey;

	case INS_SET_SESSION_KEY:
		return handleSetAttestKey;
#endif
// *INDENT-ON*

	default:
		return NULL;
	}
}

#endif
