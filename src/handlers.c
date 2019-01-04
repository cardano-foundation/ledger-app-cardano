#ifndef H_HANDLERS

#include <os_io_seproxyhal.h>
#include <stdlib.h>

#include "handlers.h"
#include "getVersion.h"
#include "getPubKey.h"
#include "runTests.h"

#include "state.h"
#include "errors.h"

// The APDU protocol uses a single-byte instruction code (INS) to specify
// which command should be executed. We'll use this code to dispatch on a
// table of function pointers.

enum {
	INS_GET_VERSION   = 0x01,
	INS_SHOW_ABOUT    = 0x02,
	INS_RUN_TESTS     = 0x03,
	INS_GET_PUB_KEY   = 0x10,
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
		return handleGetPubKey;
	case INS_RUN_TESTS:
		return handleRunTests;
	default:
		return NULL;
	}
}

#endif
