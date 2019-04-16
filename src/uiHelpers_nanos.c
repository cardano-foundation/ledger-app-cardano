#include <bolos_target.h> // we need target definitions
#if defined(TARGET_NANOS)

#include <os_io_seproxyhal.h>
#include "uiHelpers.h"

#ifdef HEADLESS
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

static void scroll_update_display_content()
{
	paginatedTextState_t* ctx = paginatedTextState;
	assert_uiPaginatedText_magic();
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
	paginatedTextState_t* ctx = paginatedTextState;
	assert_uiPaginatedText_magic();
	if (ctx->scrollIndex > 0) {
		ctx->scrollIndex--;
		scroll_update_display_content();
	}
}

static void scroll_right()
{
	paginatedTextState_t* ctx = paginatedTextState;
	assert_uiPaginatedText_magic();
	if (ctx->scrollIndex + SIZEOF(ctx->currentText) < 1 + strlen(ctx->fullText)) {
		paginatedTextState->scrollIndex++;
		scroll_update_display_content();
	}
}

unsigned int ui_paginatedText_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
)
{
	paginatedTextState_t* ctx = paginatedTextState;
	TRY_CATCH_UI({
		assert_uiPaginatedText_magic();
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
	});
	return 0;
}

unsigned int ui_prompt_button(
        unsigned int button_mask,
        unsigned int button_mask_counter MARK_UNUSED
)
{
	TRY_CATCH_UI({
		assert_uiPrompt_magic();
		ASSERT(io_state == IO_EXPECT_UI);
		ASSERT(device_is_unlocked() == true);
		switch (button_mask)
		{
		case BUTTON_EVT_RELEASED | BUTTON_LEFT: // REJECT
			uiCallback_reject(&promptState->callback);
			break;

		case BUTTON_EVT_RELEASED | BUTTON_RIGHT: // APPROVE
			uiCallback_confirm(&promptState->callback);
			break;
		}
	})
	return 0;
}

static const bagl_element_t ui_paginatedText[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_GO_LEFT, BAGL_GLYPH_ICON_LEFT),
	UI_ICON_RIGHT(ID_ICON_GO_RIGHT, BAGL_GLYPH_ICON_RIGHT),

	// TODO(ppershing): what are the following magical numbers?

	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, &displayState.paginatedText.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, &displayState.paginatedText.currentText),
	#ifdef HEADLESS
	HEADLESS_UI_ELEMENT(),
	#endif
};

static const bagl_element_t* ui_prepro_paginatedText(const bagl_element_t *element)
{
	paginatedTextState_t* ctx = paginatedTextState;
	assert_uiPaginatedText_magic();

	bool textFitsSinglePage = strlen(ctx->currentText) >= strlen(ctx->fullText);
	switch (element->component.userid) {
	case ID_ICON_GO_LEFT:
		return (ctx->scrollIndex != 0 || textFitsSinglePage)
		       ? element
		       : NULL;
	case ID_ICON_GO_RIGHT:
		return ((ctx->scrollIndex + SIZEOF(ctx->currentText)
		         < strlen(ctx->fullText) + 1)
		        || textFitsSinglePage)
		       ? element
		       : NULL;
	default:
		// Always display all other elements.
		return element;
	}
}

static void uiCallback_init(ui_callback_t* cb, ui_callback_fn_t* confirm, ui_callback_fn_t* reject)
{
	cb->state = CALLBACK_NOT_RUN;
	cb->confirm = confirm;
	cb->reject = reject;
}

static const bagl_element_t ui_prompt[] = {
	UI_BACKGROUND(),
	UI_ICON_LEFT(ID_ICON_REJECT, BAGL_GLYPH_ICON_CROSS),
	UI_ICON_RIGHT(ID_ICON_CONFIRM, BAGL_GLYPH_ICON_CHECK),
	UI_TEXT(ID_UNSPECIFIED, 0, 12, 128, displayState.prompt.header),
	UI_TEXT(ID_UNSPECIFIED, 0, 26, 128, displayState.prompt.text),
	#ifdef HEADLESS
	HEADLESS_UI_ELEMENT(),
	#endif
};

static const bagl_element_t* ui_prepro_prompt(const bagl_element_t *element)
{
	promptState_t* ctx = promptState;

	assert_uiPrompt_magic();
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

void ui_displayBusy()
{
	UX_DISPLAY(ui_busy, NULL);
}

void ui_displayPrompt_run()
{
	UX_DISPLAY(ui_prompt, ui_prepro_prompt);
}

void ui_displayPaginatedText_run()
{
	UX_DISPLAY(ui_paginatedText, ui_prepro_paginatedText);
}

#endif