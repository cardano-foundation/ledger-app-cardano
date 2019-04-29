#include <os_io_seproxyhal.h>
#include "uiHelpers.h"
#include "ux.h"
#include "assert.h"
#include "io.h"
#include "utils.h"
#include "securityPolicy.h"

displayState_t displayState;

// These are global variables declared in ux.h. They can't be defined there
// because multiple files include ux.h; they need to be defined in exactly one
// place. See ux.h for their descriptions.
// To save memory, we store all the context types in a single global union,
// taking advantage of the fact that only one command is executed at a time.
#if defined(TARGET_NANOS)
// ux is a magic global variable implicitly referenced by the UX_ macros. Apps
// should never need to reference it directly
ux_state_t ux;
#elif defined(TARGET_NANOX)
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#endif

STATIC_ASSERT(SIZEOF(uint8_t) == SIZEOF(char), "bad char size");

void assert_uiPaginatedText_magic()
{
	ASSERT(paginatedTextState->initMagic == INIT_MAGIC_PAGINATED_TEXT);
}

void assert_uiPrompt_magic()
{
	ASSERT(promptState->initMagic == INIT_MAGIC_PROMPT);
}

void uiCallback_confirm(ui_callback_t* cb)
{
	if (!cb->confirm) return;

	switch(cb->state) {
	case CALLBACK_NOT_RUN:
		// Note: needs to be done before resolving in case it throws
		cb->state = CALLBACK_RUN;
		cb->confirm();
		break;
	case CALLBACK_RUN:
		// Ignore
		break;
	default:
		ASSERT(false);
	}
}

void uiCallback_reject(ui_callback_t* cb)
{
	if (!cb->reject) return;

	switch(cb->state) {
	case CALLBACK_NOT_RUN:
		// Note: needs to be done before resolving in case it throws
		cb->state = CALLBACK_RUN;
		cb->reject();
		break;
	case CALLBACK_RUN:
		// Ignore
		break;
	default:
		ASSERT(false);
	}
}

#ifdef HEADLESS
static int HEADLESS_DELAY = 100;

void ui_displayPrompt_headless_cb(bool ux_allowed)
{
	TRACE("HEADLESS response");
	if (!ux_allowed) {
		TRACE("No UX allowed, ignoring headless cb!");
		return;
	}
	TRY_CATCH_UI({
		assert_uiPrompt_magic();
		ASSERT(io_state == IO_EXPECT_UI);
		ASSERT(device_is_unlocked() == true);
		uiCallback_confirm(&promptState->callback);
	})
}

void autoconfirmPrompt()
{
	#if defined(TARGET_NANOS)
	nanos_set_timer(HEADLESS_DELAY, ui_displayPrompt_headless_cb);
	#elif defined(TARGET_NANOX)
	UX_CALLBACK_SET_INTERVAL(HEADLESS_DELAY);
	#endif
}

void ui_displayPaginatedText_headless_cb(bool ux_allowed)
{
	TRACE("HEADLESS response");
	if (!ux_allowed) {
		TRACE("No UX allowed, ignoring headless cb!");
		return;
	}
	TRY_CATCH_UI({
		assert_uiPaginatedText_magic();
		ASSERT(io_state == IO_EXPECT_UI);
		ASSERT(device_is_unlocked() == true);
		uiCallback_confirm(&paginatedTextState->callback);
	});
}

void autoconfirmPaginatedText()
{
	#if defined(TARGET_NANOS)
	nanos_set_timer(HEADLESS_DELAY, ui_displayPaginatedText_headless_cb);
	#elif defined(TARGET_NANOX)
	UX_CALLBACK_SET_INTERVAL(HEADLESS_DELAY);
	#endif
}

#endif

static void uiCallback_init(ui_callback_t* cb, ui_callback_fn_t* confirm, ui_callback_fn_t* reject)
{
	cb->state = CALLBACK_NOT_RUN;
	cb->confirm = confirm;
	cb->reject = reject;
}

void ui_displayPrompt(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* confirm,
        ui_callback_fn_t* reject)
{
	size_t header_len = strlen(headerStr);
	size_t text_len = strlen(bodyStr);
	// sanity checks, keep 1 byte for null terminator
	ASSERT(header_len < SIZEOF(promptState->header));
	ASSERT(text_len < SIZEOF(promptState->text));

	// clear all memory
	os_memset(&displayState, 0, SIZEOF(displayState));
	promptState_t* ctx = promptState;

	// Copy data
	os_memmove(ctx->header, headerStr, header_len + 1);
	os_memmove(ctx->text, bodyStr, text_len + 1);

	uiCallback_init(&ctx->callback, confirm, reject);
	ctx->initMagic = INIT_MAGIC_PROMPT;
	ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_UI);
	io_state = IO_EXPECT_UI;

	ui_displayPrompt_run();

	#ifdef HEADLESS
	if (confirm) {
		autoconfirmPrompt();
	}
	#endif
}

void ui_displayPaginatedText(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* callback)
{
	TRACE();
	paginatedTextState_t* ctx = paginatedTextState;
	size_t header_len = strlen(headerStr);
	size_t body_len = strlen(bodyStr);
	// sanity checks
	ASSERT(header_len < SIZEOF(ctx->header));
	ASSERT(body_len < SIZEOF(ctx->fullText));

	// clear all memory
	os_memset(ctx, 0, SIZEOF(*ctx));

	// Copy data
	os_memmove(ctx->header, headerStr, header_len);
	os_memmove(ctx->fullText, bodyStr, body_len);

	ctx->scrollIndex = 0;

	os_memmove(
	        ctx->currentText,
	        ctx->fullText,
	        SIZEOF(ctx->currentText) - 1
	);

	uiCallback_init(&ctx->callback, callback, NULL);
	ctx->initMagic = INIT_MAGIC_PAGINATED_TEXT;
	TRACE("setting timeout");
	TRACE("done");
	ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_UI);
	io_state = IO_EXPECT_UI;

	ui_displayPaginatedText_run();

	#ifdef HEADLESS
	if (callback) {
		autoconfirmPaginatedText();
	}
	#endif
}

void respond_with_user_reject()
{
	io_send_buf(ERR_REJECTED_BY_USER, NULL, 0);
	ui_idle();
}
