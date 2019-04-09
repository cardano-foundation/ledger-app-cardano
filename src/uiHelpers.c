#include <os_io_seproxyhal.h>
#include <string.h>

#include "uiHelpers.h"
#include "ux.h"
#include "assert.h"
#include "io.h"
#include "utils.h"
#include "securityPolicy.h"

static displayState_t displayState;

// WARNING(ppershing): Following two references MUST be declared `static`
// otherwise the Ledger will crash. I am really not sure why is this
// but it might be related to position-independent-code compilation.
static scrollingState_t* scrollingState = &(displayState.scrolling);
static confirmState_t* confirmState = &(displayState.confirm);

enum {
	INIT_MAGIC_SCROLLER = 2345,
	INIT_MAGIC_CONFIRM = 5432,
};

STATIC_ASSERT(SIZEOF(uint8_t) == SIZEOF(char), "bad char size");


#if defined(TARGET_NANOS)

#ifdef HEADLESS
static int HEADLESS_DELAY = 100;
#define HEADLESS_UI_ELEMENT() \
	{ \
		{ \
			BAGL_LABELINE,ID_HEADLESS, 0, 12, 128, \
			12,0,0,0,0xFFFFFF,0, \
			BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_LEFT,0 \
		}, \
		"HEADLESS ",0,0,0,NULL,NULL,NULL \
	}
#endif

#define UI_BACKGROUND() {{BAGL_RECTANGLE,0,0,0,128,32,0,0,BAGL_FILL,0,0xFFFFFF,0,0},NULL,0,0,0,NULL,NULL,NULL}
#define UI_ICON_LEFT(userid, glyph) {{BAGL_ICON,userid,3,12,7,7,0,0,0,0xFFFFFF,0,0,glyph},NULL,0,0,0,NULL,NULL,NULL}
#define UI_ICON_RIGHT(userid, glyph) {{BAGL_ICON,userid,117,13,8,6,0,0,0,0xFFFFFF,0,0,glyph},NULL,0,0,0,NULL,NULL,NULL}
#define UI_TEXT(userid, x, y, w, text) {{BAGL_LABELINE,userid,x,y,w,12,0,0,0,0xFFFFFF,0,BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER,0},(char *)text,0,0,0,NULL,NULL,NULL}


enum {
	ID_UNSPECIFIED = 0x00,
	ID_ICON_GO_LEFT = 0x01,
	ID_ICON_GO_RIGHT = 0x02,
	ID_ICON_CONFIRM = 0x03,
	ID_ICON_REJECT = 0x04,

	ID_HEADLESS = 0xff,
};

static const bagl_element_t ui_busy[] = {
	UI_BACKGROUND(),
	UI_TEXT(ID_UNSPECIFIED, 0, 20, 128, "..."),
};

static unsigned int ui_busy_button(
        unsigned int button_mask MARK_UNUSED,
        unsigned int button_mask_counter MARK_UNUSED
)
{
	return 0;
}

static const bagl_element_t ui_scrollingText[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_GO_LEFT, BAGL_GLYPH_ICON_LEFT),
	UI_ICON_RIGHT(ID_ICON_GO_RIGHT, BAGL_GLYPH_ICON_RIGHT),

	// TODO(ppershing): what are the following magical numbers?

	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, &displayState.scrolling.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, &displayState.scrolling.currentText),
	#ifdef HEADLESS
	HEADLESS_UI_ELEMENT(),
	#endif
};

// forward
static unsigned int ui_scrollingText_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
);

static const bagl_element_t* ui_prepro_scrollingText(const bagl_element_t *element)
{
	scrollingState_t* ctx = scrollingState;
	ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);

	switch (element->component.userid) {
	case ID_ICON_GO_LEFT:
		return (ctx->scrollIndex == 0)
		       ? NULL
		       : element;
	case ID_ICON_GO_RIGHT:
		return (ctx->scrollIndex + SIZEOF(ctx->currentText)
		        >= strlen(ctx->fullText) + 1)
		       ? NULL
		       : element;
	default:
		// Always display all other elements.
		return element;
	}
}

static void scroll_update_display_content()
{
	scrollingState_t* ctx = scrollingState;
	ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
	ASSERT(ctx->currentText[SIZEOF(ctx->currentText) - 1] == '\0');
	ASSERT(ctx->scrollIndex + SIZEOF(ctx->currentText) <= SIZEOF(ctx->fullText));
	os_memmove(
	        ctx->currentText,
	        ctx->fullText + ctx->scrollIndex,
	        SIZEOF(ctx->currentText) - 1
	);
	UX_REDISPLAY();
}

static void scroll_left()
{
	scrollingState_t* ctx = scrollingState;
	ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
	if (ctx->scrollIndex > 0) {
		ctx->scrollIndex--;
		scroll_update_display_content();
	}
}

static void scroll_right()
{
	scrollingState_t* ctx = scrollingState;
	ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
	if (ctx->scrollIndex + SIZEOF(ctx->currentText) < 1 + strlen(ctx->fullText)) {
		scrollingState->scrollIndex++;
		scroll_update_display_content();
	}
}


#elif defined(TARGET_NANOX)

// Helper macro for better astyle formatting of UX_FLOW definitions

#define LINES(...) { __VA_ARGS__ }

static void uiCallback_confirm(ui_callback_t* cb);

void scrolling_confirm()
{
	// TODO(ppershing): exception handling
	scrollingState_t* ctx = scrollingState;
	uiCallback_confirm(&ctx->callback);
}

UX_FLOW_DEF_VALID(
        ux_display_scrolling_flow_1_step,
        bnnn_paging,
        scrolling_confirm(),
        LINES(
                &displayState.scrolling.header,
                &displayState.scrolling.fullText
        )
);

const ux_flow_step_t *        const ux_scrolling_flow [] = {
	&ux_display_scrolling_flow_1_step,
	FLOW_END_STEP,
};

UX_FLOW_DEF_NOCB(
        ux_display_busy_flow_1_step,
        pn,
        LINES(
                &C_icon_loader,
                ""
        )
);

const ux_flow_step_t *        const ux_busy_flow [] = {
	&ux_display_busy_flow_1_step,
	FLOW_END_STEP,
};
#endif


static void uiCallback_init(ui_callback_t* cb, ui_callback_fn_t* confirm, ui_callback_fn_t* reject)
{
	cb->state = CALLBACK_NOT_RUN;
	cb->confirm = confirm;
	cb->reject = reject;
}

static void uiCallback_confirm(ui_callback_t* cb)
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

static void uiCallback_reject(ui_callback_t* cb)
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

#if defined(TARGET_NANOS)
static unsigned int ui_scrollingText_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
)
{
	scrollingState_t* ctx = scrollingState;
	BEGIN_TRY {
		TRY {
			ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
			ASSERT(io_state == IO_EXPECT_UI);
			ASSERT(device_is_unlocked() == true);
			switch (button_mask)
			{
			case BUTTON_LEFT:
			case BUTTON_EVT_FAST | BUTTON_LEFT: // SEEK LEFT
				scroll_left();
				break;

			case BUTTON_RIGHT:
			case BUTTON_EVT_FAST | BUTTON_RIGHT: // SEEK RIGHT
				scroll_right();
				break;

			case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: // PROCEED
				uiCallback_confirm(&ctx->callback);
				break;
			}
		}
		CATCH(EXCEPTION_IO_RESET)
		{
			THROW(EXCEPTION_IO_RESET);
		}
		CATCH_OTHER(e)
		{
			TRACE("Error %d\n", (int) e);
			#ifdef RESET_ON_CRASH
			io_seproxyhal_se_reset();
			#endif
		}
		FINALLY {
		}
	}
	END_TRY;
	return 0;
}


#ifdef HEADLESS
void ui_displayScrollingText_headless_cb(bool ux_allowed)
{
	TRACE("HEADLESS response");
	if (!ux_allowed) {
		TRACE("No UX allowed, ignoring headless cb!");
		return;
	}
	ui_scrollingText_button(BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT, 0);
}
#endif
#elif defined(TARGET_NANOX)
#endif

void ui_displayScrollingText(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* callback)
{
	TRACE();
	scrollingState_t* ctx = scrollingState;
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
	ctx->initMagic = INIT_MAGIC_SCROLLER;
	TRACE("setting timeout");
	#ifdef HEADLESS
	if (callback) {
		set_timer(HEADLESS_DELAY, ui_displayScrollingText_headless_cb);
	}
	#endif
	TRACE("done");
	ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_UI);
	io_state = IO_EXPECT_UI;
	#if defined(TARGET_NANOS)
	UX_DISPLAY(ui_scrollingText, ui_prepro_scrollingText);
	#elif defined(TARGET_NANOX)
	ux_flow_init(0, ux_scrolling_flow, NULL);
	#else
	STATIC_ASSERT(false);
	#endif
}


#if defined(TARGET_NANOS)
static const bagl_element_t ui_confirm[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_REJECT, BAGL_GLYPH_ICON_CROSS),
	UI_ICON_RIGHT(ID_ICON_CONFIRM, BAGL_GLYPH_ICON_CHECK),
	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, displayState.confirm.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, displayState.confirm.text),
	#ifdef HEADLESS
	HEADLESS_UI_ELEMENT(),
	#endif
};

// Forward
static unsigned int ui_confirm_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
);

static const bagl_element_t* ui_prepro_confirm(const bagl_element_t *element)
{
	confirmState_t* ctx = confirmState;

	ASSERT(ctx->initMagic == INIT_MAGIC_CONFIRM);
	switch (element->component.userid) {
	case ID_ICON_REJECT:
		return ctx->callback.reject ? element : NULL;
	case ID_ICON_CONFIRM:
		return ctx->callback.confirm ? element : NULL;
	default:
		// Always display all other elements.
		return element;
	}
}

static unsigned int ui_confirm_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
)
{
	BEGIN_TRY {
		TRY {
			ASSERT(confirmState->initMagic == INIT_MAGIC_CONFIRM);
			ASSERT(io_state == IO_EXPECT_UI);
			ASSERT(device_is_unlocked() == true);
			switch (button_mask)
			{
			case BUTTON_EVT_RELEASED | BUTTON_LEFT: // REJECT
				uiCallback_reject(&confirmState->callback);
				break;

			case BUTTON_EVT_RELEASED | BUTTON_RIGHT: // APPROVE
				uiCallback_confirm(&confirmState->callback);
				break;
			}
		}
		CATCH(EXCEPTION_IO_RESET)
		{
			THROW(EXCEPTION_IO_RESET);
		}
		CATCH_OTHER(e)
		{
			TRACE("Error %d\n", (int) e);
			#ifdef RESET_ON_CRASH
			io_seproxyhal_se_reset();
			#endif
		}
		FINALLY {
		}
	}
	END_TRY;
	return 0;
}
#elif defined(TARGET_NANOX)

void confirm_confirm()
{
	// TODO(ppershing): exception handling
	confirmState_t* ctx = confirmState;
	uiCallback_confirm(&ctx->callback);
}

void confirm_reject()
{
	// TODO(ppershing): exception handling
	uiCallback_reject(&confirmState->callback);
}

UX_FLOW_DEF_VALID(
        ux_display_confirm_flow_1_step,
        pbb,
        confirm_confirm(),
        LINES(
                &C_icon_validate_14,
                &displayState.scrolling.header,
                &displayState.scrolling.currentText,
        )
);

UX_FLOW_DEF_VALID(
        ux_display_confirm_flow_2_step,
        pbb,
        confirm_reject(),
        LINES(
                &C_icon_crossmark,
                "Reject?",
                ""
        )
);

const ux_flow_step_t *        const ux_confirm_flow [] = {
	&ux_display_confirm_flow_1_step,
	&ux_display_confirm_flow_2_step,
	FLOW_END_STEP,
};
#endif

void ui_displayBusy()
{
	#if defined(TARGET_NANOS)
	UX_DISPLAY(ui_busy, NULL);
	#elif defined(TARGET_NANOX)
	ux_flow_init(0, ux_busy_flow, NULL);
	#else
	STATIC_ASSERT(false)
	#endif
}

#ifdef HEADLESS
void ui_displayConfirm_headless_cb(bool ux_allowed)
{
	TRACE("HEADLESS response");
	if (!ux_allowed) {
		TRACE("No UX allowed, ignoring headless cb!");
		return;
	}
	ui_confirm_button(BUTTON_EVT_RELEASED | BUTTON_RIGHT, 0);
}
#endif


void ui_displayConfirm(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* confirm,
        ui_callback_fn_t* reject)
{
	size_t header_len = strlen(headerStr);
	size_t text_len = strlen(bodyStr);
	// sanity checks, keep 1 byte for null terminator
	ASSERT(header_len < SIZEOF(confirmState->header));
	ASSERT(text_len < SIZEOF(confirmState->text));

	// clear all memory
	os_memset(&displayState, 0, SIZEOF(displayState));
	confirmState_t* ctx = confirmState;

	// Copy data
	os_memmove(ctx->header, headerStr, header_len + 1);
	os_memmove(ctx->text, bodyStr, text_len + 1);

	uiCallback_init(&ctx->callback, confirm, reject);
	ctx->initMagic = INIT_MAGIC_CONFIRM;
	#ifdef HEADLESS
	if (confirm) {
		set_timer(HEADLESS_DELAY, ui_displayConfirm_headless_cb);
	}
	#endif
	ASSERT(io_state == IO_EXPECT_NONE || io_state == IO_EXPECT_UI);
	io_state = IO_EXPECT_UI;
	#if defined(TARGET_NANOS)
	UX_DISPLAY(ui_confirm, ui_prepro_confirm);
	#elif defined(TARGET_NANOX)
	ux_flow_init(0, ux_confirm_flow, NULL);
	#else
	STATIC_ASSERT(false);
	#endif
}

void respond_with_user_reject()
{
	io_send_buf(ERR_REJECTED_BY_USER, NULL, 0);
	ui_idle();
}
