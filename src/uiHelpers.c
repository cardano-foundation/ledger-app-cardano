#include <os_io_seproxyhal.h>
#include <string.h>

#include "uiHelpers.h"
#include "ux.h"
#include "assert.h"
#include "io.h"

displayState_t displayState;

#define ID_UNSPECIFIED 0x00
#define ID_BTN_LEFT 0x01
#define ID_BNT_RIGHT 0x02

static const bagl_element_t ui_scrollingText[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_BTN_LEFT, BAGL_GLYPH_ICON_LEFT),
	UI_ICON_RIGHT(ID_BNT_RIGHT, BAGL_GLYPH_ICON_RIGHT),

	// TODO(ppershing): what are the following magical numbers?

	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, &displayState.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, &displayState.currentText),
};

static const bagl_element_t* ui_prepro_scrollingText(const bagl_element_t *element)
{
	switch (element->component.userid) {
	case ID_BTN_LEFT:
		return (displayState.scrollIndex == 0) ? NULL : element;
	case ID_BNT_RIGHT:
		return (displayState.scrollIndex + DISPLAY_TEXT_LEN >= strlen(displayState.fullText)) ? NULL : element;
	default:
		// Always display all other elements.
		return element;
	}
}


void update_display_content()
{
	checkOrFail(
	        displayState.currentText[DISPLAY_TEXT_LEN] == '\0',
	        "memory corruption"
	);
	checkOrFail(
	        displayState.scrollIndex >= 0,
	        "bad scroll index");
	checkOrFail(
	        displayState.scrollIndex + DISPLAY_TEXT_LEN < sizeof(displayState.fullText),
	        "bad scroll index");
	os_memmove(displayState.currentText, displayState.fullText + displayState.scrollIndex, DISPLAY_TEXT_LEN);
	UX_REDISPLAY();
}

void scroll_left()
{
	if (displayState.scrollIndex > 0) {
		displayState.scrollIndex--;
	}
	update_display_content();
}

void scroll_right()
{
	if (displayState.scrollIndex < strlen(displayState.fullText) - DISPLAY_TEXT_LEN) {
		displayState.scrollIndex++;
	}
	update_display_content();
}

static unsigned int ui_scrollingText_button(unsigned int button_mask, unsigned int button_mask_counter)
{
	switch (button_mask) {
	case BUTTON_LEFT:
	case BUTTON_EVT_FAST | BUTTON_LEFT: // SEEK LEFT
		scroll_left();
		break;

	case BUTTON_RIGHT:
	case BUTTON_EVT_FAST | BUTTON_RIGHT: // SEEK RIGHT
		scroll_right();
		break;

	case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: // PROCEED
		displayState.callback();
		break;
	}
	// (The return value of a button handler is irrelevant; it is never
	// checked.)
	return 0;
}


void displayScrollingText(
        char* header, uint16_t header_len,
        char* text, uint16_t text_len,
        confirm_cb_t* callback)
{
	// sanity checks
	checkOrFail(header_len < sizeof(displayState.header), "too long header");
	checkOrFail(text_len < sizeof(displayState.fullText), "too long text");

	// clear all memory
	os_memset(&displayState, 0, sizeof(displayState));

	// Copy data
	os_memmove(displayState.header, header, header_len);
	os_memmove(displayState.fullText, text, text_len);
	STATIC_ASSERT(DISPLAY_TEXT_LEN < sizeof(displayState.currentText), overflow_check);

	// Note(ppershing): due to previous clearing of memory
	// this also works with cases where
	// strlen(fullText) < DISPLAY_TEXT_LEN

	checkOrFail(displayState.scrollIndex + DISPLAY_TEXT_LEN < sizeof(displayState.fullText), "buffer overflow");
	os_memmove(displayState.currentText, displayState.fullText + displayState.scrollIndex, DISPLAY_TEXT_LEN);

	displayState.callback = callback;
	UX_DISPLAY(ui_scrollingText, ui_prepro_scrollingText);
}
