#include <os_io_seproxyhal.h>
#include <string.h>

#include "uiHelpers.h"
#include "ux.h"
#include "assert.h"
#include "io.h"
#include "utils.h"

static displayState_t displayState;

// WARNING(ppershing): Following two references MUST be declared `static`
// otherwise the Ledger will crash. I am really not sure why is this
// but it might be related to position-independent-code compilation.
static scrollingState_t* scrollingState = &(displayState.scrolling);
static confirmState_t* confirmState = &(displayState.confirm);

#define ID_UNSPECIFIED 0x00
#define ID_BTN_LEFT 0x01
#define ID_BTN_RIGHT 0x02

STATIC_ASSERT(sizeof(uint8_t) == sizeof(char), "bad char size");

// {{{ scrollingText
static const bagl_element_t ui_scrollingText[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_BTN_LEFT, BAGL_GLYPH_ICON_LEFT),
	UI_ICON_RIGHT(ID_BTN_RIGHT, BAGL_GLYPH_ICON_RIGHT),

	// TODO(ppershing): what are the following magical numbers?

	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, &displayState.scrolling.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, &displayState.scrolling.currentText),
};

static const bagl_element_t* ui_prepro_scrollingText(const bagl_element_t *element)
{
	switch (element->component.userid) {
	case ID_BTN_LEFT:
		return (scrollingState->scrollIndex == 0)
		       ? NULL
		       : element;
	case ID_BTN_RIGHT:
		return (scrollingState->scrollIndex + DISPLAY_TEXT_LEN
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
	ASSERT(scrollingState->currentText[DISPLAY_TEXT_LEN] == '\0');
	ASSERT(scrollingState->scrollIndex + DISPLAY_TEXT_LEN < SIZEOF(scrollingState->fullText));
	os_memmove(
	        scrollingState->currentText,
	        scrollingState->fullText + scrollingState->scrollIndex,
	        DISPLAY_TEXT_LEN
	);
	UX_REDISPLAY();
}

void scroll_left()
{
	if (scrollingState->scrollIndex > 0) {
		scrollingState->scrollIndex--;
	}
	update_display_content();
}

void scroll_right()
{
	if (scrollingState->scrollIndex
	    < strlen(scrollingState->fullText) - DISPLAY_TEXT_LEN) {
		scrollingState->scrollIndex++;
	}
	update_display_content();
}

static unsigned int ui_scrollingText_button(unsigned int button_mask, unsigned int button_mask_counter)
{
	BEGIN_TRY {
		TRY {
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


void displayScrollingText(
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
	STATIC_ASSERT(DISPLAY_TEXT_LEN < SIZEOF(scrollingState->currentText), overflow_check);

	// Note(ppershing): due to previous clearing of memory
	// this also works with cases where
	// strlen(fullText) < DISPLAY_TEXT_LEN


	ASSERT(scrollingState->scrollIndex + DISPLAY_TEXT_LEN < SIZEOF(scrollingState->fullText));

	os_memmove(scrollingState->currentText, scrollingState->fullText + scrollingState->scrollIndex, DISPLAY_TEXT_LEN);

	scrollingState->callback = callback;
	UX_DISPLAY(ui_scrollingText, ui_prepro_scrollingText);
}

// }}}


static const bagl_element_t ui_confirm[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_BTN_LEFT, BAGL_GLYPH_ICON_CROSS),
	UI_ICON_RIGHT(ID_BTN_LEFT, BAGL_GLYPH_ICON_CHECK),
	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, displayState.confirm.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, displayState.confirm.text),
};

static const bagl_element_t* ui_prepro_confirm(const bagl_element_t *element)
{
	switch (element->component.userid) {
	case ID_BTN_LEFT:
		return (confirmState->reject == NULL) ? NULL : element;
	case ID_BTN_RIGHT:
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
			switch (button_mask)
			{
			case BUTTON_EVT_RELEASED | BUTTON_LEFT: // REJECT
				if (confirmState->reject) confirmState->reject();
				break;

			case BUTTON_EVT_RELEASED | BUTTON_RIGHT: // APPROVE
				if (confirmState->confirm) confirmState->confirm();
				break;
			}
		}
		FINALLY {
		}
	}
	END_TRY;
	return 0;
}

void displayConfirm(
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
	os_memmove(confirmState->header, headerStr, header_len);
	os_memmove(confirmState->text, bodyStr, text_len);
	confirmState->confirm = confirm;
	confirmState->reject = reject;
	UX_DISPLAY(ui_confirm, ui_prepro_confirm);
}
