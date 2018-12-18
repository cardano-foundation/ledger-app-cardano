#include <os_io_seproxyhal.h>
#include "ux.h"
#include "getVersion.h"
#include "state.h"
#include <stdbool.h>
#include "sia.h"

#define ID_UNSPECIFIED 0x00
#define ID_BTN_LEFT 0x01
#define ID_BNT_RIGHT 0x02

static showAboutState_t *state = &instructionState.showAboutState;


static const bagl_element_t ui_showAbout[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_BTN_LEFT, BAGL_GLYPH_ICON_LEFT),
	UI_ICON_RIGHT(ID_BNT_RIGHT, BAGL_GLYPH_ICON_RIGHT),

	// TODO(ppershing): what are the following magical numbers?

	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, "About Cardano app"),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, &instructionState.showAboutState.currentText),
};

static const bagl_element_t* ui_prepro_showAbout(const bagl_element_t *element)
{
	switch (element->component.userid) {
	case ID_BTN_LEFT:
		return (state->scrollIndex == 0) ? NULL : element;
	case ID_BNT_RIGHT:
		return (state->scrollIndex == sizeof(state->fullText)-DISPLAY_TEXT_LEN) ? NULL : element;
	default:
		// Always display all other elements.
		return element;
	}
}


void update_display()
{
	state->currentText[DISPLAY_TEXT_LEN] = '\0';
	os_memmove(state->currentText, state->fullText + state->scrollIndex, DISPLAY_TEXT_LEN);
	UX_REDISPLAY();
}

void scroll_left()
{
	if (state->scrollIndex > 0) {
		state->scrollIndex--;
	}
	update_display();
}

void scroll_right()
{
	if (state->scrollIndex < sizeof(state->fullText)-DISPLAY_TEXT_LEN) {
		state->scrollIndex++;
	}
	update_display();
}

void finish()
{
	io_exchange_with_code(SW_OK, 0);
	ui_idle();
}

// This is the button handler for the comparison screen. Unlike the approval
// button handler, this handler doesn't send any data to the computer.
static unsigned int ui_showAbout_button(unsigned int button_mask, unsigned int button_mask_counter)
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
		io_exchange_with_code(SW_OK, 0);
		ui_idle();
		break;
	}
	// (The return value of a button handler is irrelevant; it is never
	// checked.)
	return 0;
}

void handleShowAbout(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength,
        volatile unsigned int *flags,
        volatile unsigned int *tx)
{
	// Read the index of the signing key. U4LE is a helper macro for
	// converting a 4-byte buffer to a uint32_t.
	//ctx->keyIndex = U4LE(dataBuffer, 0);
	// Read the hash.
	//os_memmove(ctx->hash, dataBuffer+4, sizeof(ctx->hash));

	// Prepare to display the comparison screen by converting the hash to hex
	// and moving the first 12 characters into the partialHashStr buffer.
	//bin2hex(ctx->hexHash, ctx->hash, sizeof(ctx->hash));
	os_memmove(state->fullText, "abcdefghjikl12345567777acdsacdsacadscdsa", 70);

	state->currentText[DISPLAY_TEXT_LEN] = '\0';
	state->scrollIndex = 0;

	UX_DISPLAY(ui_showAbout, ui_prepro_showAbout);

	// Set the IO_ASYNC_REPLY flag. This flag tells sia_main that we aren't
	// sending data to the computer immediately; we need to wait for a button
	// press first.
	*flags |= IO_ASYNCH_REPLY;
}
