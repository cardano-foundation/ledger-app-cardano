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
	ID_UNSPECIFIED = 0x00,
	ID_ICON_GO_LEFT = 0x01,
	ID_ICON_GO_RIGHT = 0x02,
	ID_ICON_CONFIRM = 0x03,
	ID_ICON_REJECT = 0x04,
};

enum {
	INIT_MAGIC_SCROLLER = 2345,
	INIT_MAGIC_CONFIRM = 5432,
};

STATIC_ASSERT(SIZEOF(uint8_t) == SIZEOF(char), "bad char size");

static const bagl_element_t ui_scrollingText[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_GO_LEFT, BAGL_GLYPH_ICON_LEFT),
	UI_ICON_RIGHT(ID_ICON_GO_RIGHT, BAGL_GLYPH_ICON_RIGHT),

	// TODO(ppershing): what are the following magical numbers?

	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, &displayState.scrolling.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, &displayState.scrolling.currentText),
};

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

void scroll_update_display_content()
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

void scroll_left()
{
	scrollingState_t* ctx = scrollingState;
	ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
	if (ctx->scrollIndex > 0) {
		ctx->scrollIndex--;
		scroll_update_display_content();
	}
}

void scroll_right()
{
	scrollingState_t* ctx = scrollingState;
	ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
	if (ctx->scrollIndex + SIZEOF(ctx->currentText) < 1 + strlen(ctx->fullText)) {
		scrollingState->scrollIndex++;
		scroll_update_display_content();
	}
}


void uiCallback_init(ui_callback_t* cb, ui_callback_fn_t* confirm, ui_callback_fn_t* reject)
{
	cb->state = CALLBACK_NOT_RUN;
	cb->confirm = confirm;
	cb->reject = reject;
}

void uiCallback_confirm(ui_callback_t* cb)
{
	if (cb->confirm == NULL) return;

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
	if (cb->reject == NULL) return;

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

static unsigned int ui_scrollingText_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
)
{
	scrollingState_t* ctx = scrollingState;
	BEGIN_TRY {
		TRY {
			ASSERT(ctx->initMagic == INIT_MAGIC_SCROLLER);
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
		CATCH_OTHER(e)
		{
			TRACE("Error %d\n", (int) e);
			#ifndef DEVEL
			io_seproxyhal_se_reset();
			#endif
		}
		FINALLY {
		}
	}
	END_TRY;
	return 0;
}


void ui_displayScrollingText(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* callback)
{
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
	UX_DISPLAY(ui_scrollingText, ui_prepro_scrollingText);
}



static const bagl_element_t ui_confirm[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_REJECT, BAGL_GLYPH_ICON_CROSS),
	UI_ICON_RIGHT(ID_ICON_CONFIRM, BAGL_GLYPH_ICON_CHECK),
	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, displayState.confirm.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, displayState.confirm.text),
};

static const bagl_element_t* ui_prepro_confirm(const bagl_element_t *element)
{
	confirmState_t* ctx = confirmState;

	ASSERT(ctx->initMagic == INIT_MAGIC_CONFIRM);
	switch (element->component.userid) {
	case ID_ICON_REJECT:
		return (ctx->callback.reject != NULL) ? element : NULL;
	case ID_ICON_CONFIRM:
		return (ctx->callback.confirm != NULL) ? element : NULL;
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
		CATCH_OTHER(e)
		{
			TRACE("Error %d\n", (int) e);
			#ifndef DEVEL
			io_seproxyhal_se_reset();
			#endif
		}
		FINALLY {
		}
	}
	END_TRY;
	return 0;
}

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
	UX_DISPLAY(ui_confirm, ui_prepro_confirm);
}

void respond_with_user_reject()
{
	io_send_buf(ERR_REJECTED_BY_USER, NULL, 0);
	ui_idle();
}
