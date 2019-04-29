/*******************************************************************************
*
*  (c) 2016 Ledger
*  (c) 2018 Nebulous
*  (c) 2019 VacuumLabs
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

// For a nice primer on writing/understanding Ledger NANO S apps
// see https://github.com/LedgerHQ/ledger-app-sia/

#include <stdint.h>
#include <stdbool.h>
#include <os_io_seproxyhal.h>
#include <os.h>
#include "ux.h"

#include "getVersion.h"
#include "attestKey.h"
#include "handlers.h"
#include "state.h"
#include "errors.h"
#include "menu.h"
#include "assert.h"
#include "io.h"
#include "endian.h"

// The whole app is designed for a specific api level.
// In case there is an api change, first *verify* changes
// (especially potential security implications) before bumping
// the API level!
STATIC_ASSERT(CX_APILEVEL == 9 || CX_APILEVEL == 10, "bad api level");

static const int INS_NONE = -1;

// ui_idle displays the main menu. Note that your app isn't required to use a
// menu as its idle screen; you can define your own completely custom screen.
void ui_idle(void)
{
	currentInstruction = INS_NONE;
	// The first argument is the starting index within menu_main, and the last
	// argument is a preprocessor; I've never seen an app that uses either
	// argument.
	#if defined(TARGET_NANOS)
	nanos_clear_timer();
	UX_MENU_DISPLAY(0, menu_main, NULL);
	#elif defined(TARGET_NANOX)
	// reserve a display stack slot if none yet
	if(G_ux.stack_count == 0) {
		ux_stack_push();
	}
	ux_flow_init(0, ux_idle_flow, NULL);
	#else
	STATIC_ASSERT(false);
	#endif
}

static const uint8_t CLA = 0xD7;

// This is the main loop that reads and writes APDUs. It receives request
// APDUs from the computer, looks up the corresponding command handler, and
// calls it on the APDU payload. Then it loops around and calls io_exchange
// again. The handler may set the 'flags' and 'tx' variables, which affect the
// subsequent io_exchange call. The handler may also throw an exception, which
// will be caught, converted to an error code, appended to the response APDU,
// and sent in the next io_exchange call.
static void cardano_main(void)
{
	volatile size_t rx = 0;
	volatile size_t tx = 0;
	volatile uint8_t flags = 0;

	// Exchange APDUs until EXCEPTION_IO_RESET is thrown.
	for (;;) {
		// The Ledger SDK implements a form of exception handling. In addition
		// to explicit THROWs in user code, syscalls (prefixed with os_ or
		// cx_) may also throw exceptions.
		//
		// In sia_main, this TRY block serves to catch any thrown exceptions
		// and convert them to response codes, which are then sent in APDUs.
		// However, EXCEPTION_IO_RESET will be re-thrown and caught by the
		// "true" main function defined at the bottom of this file.
		BEGIN_TRY {
			TRY {
				rx = tx;
				tx = 0; // ensure no race in CATCH_OTHER if io_exchange throws an error
				ASSERT((unsigned int) rx < sizeof(G_io_apdu_buffer));
				rx = (unsigned int) io_exchange((uint8_t) (CHANNEL_APDU | flags), (uint16_t) rx);
				flags = 0;

				// We should be awaiting APDU
				ASSERT(io_state == IO_EXPECT_IO);
				io_state = IO_EXPECT_NONE;

				// No APDU received; trigger a reset.
				if (rx == 0)
				{
					THROW(EXCEPTION_IO_RESET);
				}

				VALIDATE(device_is_unlocked(), ERR_DEVICE_LOCKED);

				// Note(ppershing): unsafe to access before checks
				// Warning(ppershing): in case of unlikely change of APDU format
				// make sure you read wider values as big endian
				struct {
					uint8_t cla;
					uint8_t ins;
					uint8_t p1;
					uint8_t p2;
					uint8_t lc;
				}* header = (void*) G_io_apdu_buffer;

				VALIDATE(rx >= SIZEOF(*header), ERR_MALFORMED_REQUEST_HEADER);

				// check that data is safe to access
				VALIDATE(rx == header->lc + SIZEOF(*header), ERR_MALFORMED_REQUEST_HEADER);

				uint8_t* data = G_io_apdu_buffer + SIZEOF(*header);

				VALIDATE(header->cla == CLA, ERR_BAD_CLA);


				// Lookup and call the requested command handler.
				handler_fn_t *handlerFn = lookupHandler(header->ins);

				VALIDATE(handlerFn != NULL, ERR_UNKNOWN_INS);

				bool isNewCall = false;
				if (currentInstruction == INS_NONE)
				{
					os_memset(&instructionState, 0, SIZEOF(instructionState));
					isNewCall = true;
					currentInstruction = header->ins;
				} else
				{
					VALIDATE(header->ins == currentInstruction, ERR_STILL_IN_CALL);
				}


				// Note: handlerFn is responsible for calling io_send
				// either during its call or subsequent UI actions
				handlerFn(header->p1,
				          header->p2,
				          data,
				          header->lc,
				          isNewCall);
				flags = IO_ASYNCH_REPLY;
			}
			CATCH(EXCEPTION_IO_RESET)
			{
				THROW(EXCEPTION_IO_RESET);
			}
			CATCH(ERR_ASSERT)
			{
				// Note(ppershing): assertions should not auto-respond
				#ifdef RESET_ON_CRASH
				// Reset device
				io_seproxyhal_se_reset();
				#endif
			}
			CATCH_OTHER(e)
			{
				if (e >= _ERR_AUTORESPOND_START && e < _ERR_AUTORESPOND_END) {
					io_send_buf(e, NULL, 0);
					flags = IO_ASYNCH_REPLY;
					ui_idle();
				} else {
					PRINTF("Uncaught error %x", (unsigned) e);
					#ifdef RESET_ON_CRASH
					// Reset device
					io_seproxyhal_se_reset();
					#endif
				}
			}
			FINALLY {
			}
		}
		END_TRY;
	}
}


// Everything below this point is Ledger magic. And the magic isn't well-
// documented, so if you want to understand it, you'll need to read the
// source, which you can find in the nanos-secure-sdk repo. Fortunately, you
// don't need to understand any of this in order to write an app.
//


static void app_exit(void)
{
	BEGIN_TRY_L(exit) {
		TRY_L(exit) {
			os_sched_exit(-1);
		}
		FINALLY_L(exit) {
		}
	}
	END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(void)
{
	// exit critical section
	__asm volatile("cpsie i");

	for (;;) {
		UX_INIT();
		os_boot();
		BEGIN_TRY {
			TRY {
				io_seproxyhal_init();

				#if defined(TARGET_NANOX)
				// grab the current plane mode setting
				G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
				#endif

				USB_power(0);
				USB_power(1);
				ui_idle();

				#if defined(HAVE_BLE)
				BLE_power(0, NULL);
				BLE_power(1, "Nano X ADA");
				#endif

				attestKey_initialize();
				io_state = IO_EXPECT_IO;
				cardano_main();
			}
			CATCH(EXCEPTION_IO_RESET)
			{
				// reset IO and UX before continuing
				continue;
			}
			CATCH_ALL {
				break;
			}
			FINALLY {
			}
		}
		END_TRY;
	}
	app_exit();
	return 0;
}
