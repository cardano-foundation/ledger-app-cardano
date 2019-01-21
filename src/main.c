/*******************************************************************************
*
*  (c) 2016 Ledger
*  (c) 2018 Nebulous
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

// This code also serves as a walkthrough for writing your own Ledger Nano S
// app. Begin by reading this file top-to-bottom, and proceed to the next file
// when directed. It is recommended that you install this app on your Nano S
// so that you can see how each section of code maps to real-world behavior.
// This also allows you to experiment by modifying the code and observing the
// effect on the app.
//
// I'll begin by describing the high-level architecture of the app. The entry
// point is this file, main.c, which initializes the app and runs the APDU
// request/response loop. The loop reads APDU packets from the computer, which
// instructs it to run various commands. The Sia app supports three commands,
// each defined in a separate file: getPublicKey, signHash, and calcTxnHash.
// These each make use of Sia-specific functions, which are defined in sia.c.
// Finally, some global variables and helper functions are declared in ux.h.
//
// Each command consists of a command handler and a set of screens. Each
// screen has an associated set of elements that can be rendered, a
// preprocessor that controls which elements are rendered, and a button
// handler that processes user input. The command handler is called whenever
// sia_main receives an APDU requesting that command, and is responsible for
// displaying the first screen of the command. Control flow then moves to the
// button handler for that screen, which selects the next screen to display
// based on which button was pressed. Button handlers are also responsible for
// sending APDU replies back to the computer.
//
// The control flow can be a little confusing to understand, because the
// button handler isn't really on the "main execution path" -- it's only
// called via interrupt, typically while execution is blocked on an
// io_exchange call. (In general, it is instructive to think of io_exchange as
// the *only* call that can block.) io_exchange exchanges APDU packets with
// the computer: first it sends a response packet, then it receives a request
// packet. This ordering may seem strange, but it makes sense when you
// consider that the Nano S has to do work in between receiving a command and
// replying to it. Thus, the packet sent by io_exchange is a *response* to the
// previous request, and the packet received is the next request.
//
// But there's a problem with this flow: in most cases, we can't respond to
// the command request until we've received some user input, e.g. approving a
// signature. If io_exchange is the only call that blocks, how can we tell it
// to wait for user input? The answer is a special flag, IO_ASYNC_REPLY. When
// io_exchange is called with this flag, it blocks, but it doesn't send a
// response; instead, it just waits for a new request. Later on, we make a
// separate call to io_exchange, this time with the IO_RETURN_AFTER_TX flag.
// This call sends the response, and then returns immediately without waiting
// for the next request. Visually, it is clear that these flags have opposite
// effects on io_exchange:
//
//                                      ----Time--->
//    io_exchange:        [---Send Response---|---Wait for Request---]
//    IO_ASYNC_REPLY:                           ^Only do this part^
//    IO_RETURN_AFTER_TX:  ^Only do this part^
//
// So a typical command flow looks something like this. We start in sia_main,
// which is an infinite loop that starts by calling io_exchange. It receives
// an APDU request from the computer and calls the associated command handler.
// The handler displays a screen, e.g. "Generate address?", and sets the
// IO_ASYNC_REPLY flag before returning. Control returns to sia_main, which
// loops around and calls io_exchange again; due to the flag, it now blocks.
// Everything is frozen until the user decides which button to press. When
// they eventually press the "Approve" button, the button handler jumps into
// action. It generates the address, constructs a response APDU containing
// that address, calls io_exchange with IO_RETURN_AFTER_TX, and redisplays the
// main menu. When a new command arrives, it will be received by the blocked
// io_exchange in sia_main.
//
// More complex commands may require multiple requests and responses. There
// are two approaches to handling this. One approach is to treat each command
// handler as a self-contained unit: that is, the main loop should only handle
// the *first* request for a given command. Subsequent requests are handled by
// additional io_exchange calls within the command handler. The other approach
// is to let the main loop handle all requests, and design the handlers so
// that they can "pick up where they left off." Both designs have tradeoffs.
// In the Sia app, the only handler that requires multiple requests is
// calcTxnHash, and it takes the latter approach.
//
// On the other end of the spectrum, there are simple commands that do not
// require any user input. Many Nano S apps have a "getVersion" command that
// replies to the computer with the app's version. In this case, it is
// sufficient for the command handler to prepare the response APDU and allow
// the main loop to send it immediately, without setting IO_ASYNC_REPLY.
//
// The important things to remember are:
// - io_exchange is the only blocking call
// - the main loop invokes command handlers, which display screens and set button handlers
// - button handlers switch between screens and reply to the computer

#include <stdint.h>
#include <stdbool.h>
#include <os_io_seproxyhal.h>
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

// These are global variables declared in ux.h. They can't be defined there
// because multiple files include ux.h; they need to be defined in exactly one
// place. See ux.h for their descriptions.
// To save memory, we store all the context types in a single global union,
// taking advantage of the fact that only one command is executed at a time.

ux_state_t ux;

static const int INS_NONE = -1;

// ui_idle displays the main menu. Note that your app isn't required to use a
// menu as its idle screen; you can define your own completely custom screen.
void ui_idle(void)
{
	// The first argument is the starting index within menu_main, and the last
	// argument is a preprocessor; I've never seen an app that uses either
	// argument.
	currentInstruction = INS_NONE;
	UX_MENU_DISPLAY(0, menu_main, NULL);
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
	// Mark the transaction context as uninitialized.
	//global.calcTxnHashContext.initialized = false;

	volatile unsigned int rx = 0;
	volatile unsigned int tx = 0;
	volatile unsigned int flags = 0;

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
				rx = io_exchange(CHANNEL_APDU | flags, rx);
				flags = 0;

				// No APDU received; trigger a reset.
				if (rx == 0)
				{
					THROW(EXCEPTION_IO_RESET);
				}

				// Note(ppershing): unsafe to access before checks
				struct {
					uint8_t cla[1];
					uint8_t ins[1];
					uint8_t p1[1];
					uint8_t p2[1];
					uint8_t lc[1];
				}* wireHeader = (void*) G_io_apdu_buffer;

				if (rx < SIZEOF(*wireHeader))
				{
					THROW(ERR_MALFORMED_REQUEST_HEADER);
				}
				// check that data is safe to access

				if (u1be_read(wireHeader->lc) + SIZEOF(*wireHeader) != rx)
				{
					THROW(ERR_MALFORMED_REQUEST_HEADER);
				}
				uint8_t* data = G_io_apdu_buffer + SIZEOF(*wireHeader);

				// Reinterpret data in machine endian.
				// Note(ppershing): this is not needed unless
				// somebody changes byte widths
				struct {
					uint8_t cla;
					uint8_t ins;
					uint8_t p1;
					uint8_t p2;
					uint8_t lc;
				} parsedHeader = {
					.cla  = u1be_read(wireHeader->cla),
					.ins  = u1be_read(wireHeader->ins),
					.p1   = u1be_read(wireHeader->p1),
					.p2   = u1be_read(wireHeader->p2),
					.lc   = u1be_read(wireHeader->lc),
				};

				STATIC_ASSERT(SIZEOF(*wireHeader) == SIZEOF(parsedHeader), "header size mismatch");

				if (parsedHeader.cla != CLA)
				{
					THROW(ERR_BAD_CLA);
				}


				// Lookup and call the requested command handler.
				handler_fn_t *handlerFn = lookupHandler(parsedHeader.ins);
				if (!handlerFn)
				{
					THROW(ERR_UNKNOWN_INS);
				}

				bool isNewCall = false;
				if (currentInstruction == INS_NONE)
				{
					os_memset(&instructionState, 0, SIZEOF(instructionState));
					isNewCall = true;
					currentInstruction = parsedHeader.ins;
				} else
				{
					if (currentInstruction != parsedHeader.ins) {
						THROW(ERR_STILL_IN_CALL);
					}
				}


				// Note: handlerFn is responsible for calling io_send
				// either during its call or subsequent UI actions
				handlerFn(parsedHeader.p1,
				          parsedHeader.p2,
				          data,
				          parsedHeader.lc,
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
			}
			CATCH_OTHER(e)
			{
				if (e >= _ERR_AUTORESPOND_START && e <= _ERR_AUTORESPOND_END) {
					io_send_buf(e, NULL, 0);
					flags = IO_ASYNCH_REPLY;
					ui_idle();
				} else {
					// Note(ppershing): remaining errors should
					// stop the execution

					// TODO(ppershing): test if this really stops
					// responding to APDUs
					flags = IO_ASYNCH_REPLY;
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
				USB_power(0);
				USB_power(1);
				ui_idle();
				attestKey_initialize();
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
