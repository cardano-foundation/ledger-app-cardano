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

STATIC_ASSERT(sizeof(uint8_t) == sizeof(char), "bad char size");

// {{{ scrollingText
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
	ASSERT(scrollingState->initMagic == INIT_MAGIC_SCROLLER);
	switch (element->component.userid) {
	case ID_ICON_GO_LEFT:
		return (scrollingState->scrollIndex == 0)
		       ? NULL
		       : element;
	case ID_ICON_GO_RIGHT:
		return (scrollingState->scrollIndex + UI_DISPLAY_TEXT_LEN
		        >= strlen(scrollingState->fullText))
		       ? NULL
		       : element;
	default:
		// Always display all other elements.
		return element;
	}
}


void update_display_content()
{
	ASSERT(scrollingState->initMagic == INIT_MAGIC_SCROLLER);
	ASSERT(scrollingState->currentText[UI_DISPLAY_TEXT_LEN] == '\0');
	ASSERT(scrollingState->scrollIndex + UI_DISPLAY_TEXT_LEN < SIZEOF(scrollingState->fullText));
	os_memmove(
	        scrollingState->currentText,
	        scrollingState->fullText + scrollingState->scrollIndex,
	        UI_DISPLAY_TEXT_LEN
	);
	UX_REDISPLAY();
}

void scroll_left()
{
	ASSERT(scrollingState->initMagic == INIT_MAGIC_SCROLLER);
	if (scrollingState->scrollIndex > 0) {
		scrollingState->scrollIndex--;
	}
	update_display_content();
}

void scroll_right()
{
	ASSERT(scrollingState->initMagic == INIT_MAGIC_SCROLLER);
	if (scrollingState->scrollIndex
	    < strlen(scrollingState->fullText) - UI_DISPLAY_TEXT_LEN) {
		scrollingState->scrollIndex++;
	}
	update_display_content();
}

static unsigned int ui_scrollingText_button(unsigned int button_mask, unsigned int button_mask_counter)
{
	BEGIN_TRY {
		TRY {
			ASSERT(scrollingState->initMagic == INIT_MAGIC_SCROLLER);
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
				scrollingState->callback();
				break;
			}
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
        callback_t* callback)
{
	size_t header_len = strlen(headerStr);
	size_t text_len = strlen(bodyStr);
	// sanity checks
	ASSERT(header_len < SIZEOF(scrollingState->header));
	ASSERT(text_len < SIZEOF(scrollingState->fullText));

	// clear all memory
	os_memset(&displayState, 0, SIZEOF(displayState));

	// Copy data
	os_memmove(scrollingState->header, headerStr, header_len);
	os_memmove(scrollingState->fullText, bodyStr, text_len);
	STATIC_ASSERT(UI_DISPLAY_TEXT_LEN < SIZEOF(scrollingState->currentText), "overflow check");

	// Note(ppershing): due to previous clearing of memory
	// this also works with cases where
	// strlen(fullText) < DISPLAY_TEXT_LEN


	ASSERT(scrollingState->scrollIndex + UI_DISPLAY_TEXT_LEN < SIZEOF(scrollingState->fullText));

	os_memmove(scrollingState->currentText, scrollingState->fullText + scrollingState->scrollIndex, UI_DISPLAY_TEXT_LEN);

	scrollingState->callback = callback;
	scrollingState->initMagic = INIT_MAGIC_SCROLLER;
	UX_DISPLAY(ui_scrollingText, ui_prepro_scrollingText);
}

// }}}


static const bagl_element_t ui_confirm[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_REJECT, BAGL_GLYPH_ICON_CROSS),
	UI_ICON_RIGHT(ID_ICON_CONFIRM, BAGL_GLYPH_ICON_CHECK),
	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, displayState.confirm.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, displayState.confirm.text),
};

static const bagl_element_t* ui_prepro_confirm(const bagl_element_t *element)
{
	ASSERT(confirmState->initMagic == INIT_MAGIC_CONFIRM);
	switch (element->component.userid) {
	case ID_ICON_REJECT:
		return (confirmState->reject == NULL) ? NULL : element;
	case ID_ICON_CONFIRM:
		return (confirmState->confirm == NULL) ? NULL : element;
	default:
		// Always display all other elements.
		return element;
	}
}

static unsigned int ui_confirm_button(unsigned int button_mask, unsigned int button_mask_counter)
{
	BEGIN_TRY {
		TRY {
			ASSERT(confirmState->initMagic == INIT_MAGIC_CONFIRM);
			switch (button_mask)
			{
			case BUTTON_EVT_RELEASED | BUTTON_LEFT: // REJECT
				if (confirmState->reject) {
					confirmState->reject();
				}
				break;

			case BUTTON_EVT_RELEASED | BUTTON_RIGHT: // APPROVE
				if (confirmState->confirm) {
					confirmState->confirm();
				}
				break;
			}
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
        callback_t* confirm,
        callback_t* reject)
{
	size_t header_len = strlen(headerStr);
	size_t text_len = strlen(bodyStr);
	// sanity checks, keep 1 byte for null terminator
	ASSERT(header_len < SIZEOF(confirmState->header));
	ASSERT(text_len < SIZEOF(confirmState->text));

	// clear all memory
	os_memset(&displayState, 0, SIZEOF(displayState));

	// Copy data
	os_memmove(confirmState->header, headerStr, header_len + 1);
	os_memmove(confirmState->text, bodyStr, text_len + 1);
	confirmState->confirm = confirm;
	confirmState->reject = reject;
	confirmState->initMagic = INIT_MAGIC_CONFIRM;
	UX_DISPLAY(ui_confirm, ui_prepro_confirm);
}

static const bagl_element_t ui_warning_unusual[] = {
	UI_BACKGROUND(),
	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, "Unusual request."),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, "Reject if unsure!"),
};


static unsigned int ui_warning_unusual_button(unsigned int button_mask, unsigned int button_mask_counter)
{

	switch (button_mask) {
	case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: // PROCEED
		UX_DISPLAY(ui_confirm, ui_prepro_confirm);
		break;
	}
	return 0;
}

void ui_displayWarningAndConfirm(
        const char* headerStr,
        const char* bodyStr,
        callback_t* confirm,
        callback_t* reject)
{
	size_t header_len = strlen(headerStr);
	size_t text_len = strlen(bodyStr);
	// sanity checks, keep 1 byte for null terminator
	ASSERT(header_len < SIZEOF(confirmState->header));
	ASSERT(text_len < SIZEOF(confirmState->text));

	// clear all memory
	os_memset(&displayState, 0, SIZEOF(displayState));

	// Copy data
	os_memmove(confirmState->header, headerStr, header_len + 1);
	os_memmove(confirmState->text, bodyStr, text_len + 1);
	confirmState->confirm = confirm;
	confirmState->reject = reject;
	confirmState->initMagic = INIT_MAGIC_CONFIRM;
	UX_DISPLAY(ui_warning_unusual, NULL);
}

void respond_with_user_reject()
{
	io_send_buf(ERR_REJECTED_BY_USER, NULL, 0);
	ui_idle();
}



void ui_checkUserConsent(
        security_policy_t policy,
        const char* headerStr,
        const char* textStr,
        callback_t* onConfirm,
        callback_t* onReject
)
{
	switch (policy) {
	case POLICY_DENY:
		onReject();
		break;
	case POLICY_ALLOW:
		onConfirm();
		break;
	case POLICY_PROMPT_BEFORE_RESPONSE:
		ui_displayConfirm(
		        headerStr,
		        textStr,
		        onConfirm,
		        onReject
		);
		break;
	case POLICY_PROMPT_WARN_UNUSUAL:
		ui_displayWarningAndConfirm(
		        headerStr,
		        textStr,
		        onConfirm,
		        onReject
		);
		break;
	default:
		ASSERT(false);
	}
}
