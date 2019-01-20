#ifndef H_CARDANO_APP_UI_HELPERS
#define H_CARDANO_APP_UI_HELPERS

#include <stdint.h>
#include <stddef.h>
#include "securityPolicy.h"

typedef void callback_t();

static const size_t UI_DISPLAY_TEXT_LEN = 32;
static const size_t UI_MAX_TEXT_LEN = 200;

typedef struct {
	uint16_t initMagic;
	char header[UI_DISPLAY_TEXT_LEN + 1];
	char currentText[UI_DISPLAY_TEXT_LEN + 1];
	char fullText[UI_MAX_TEXT_LEN + 1];
	uint16_t scrollIndex;
	callback_t *callback;
} scrollingState_t;

typedef struct {
	uint16_t initMagic;
	char header[UI_DISPLAY_TEXT_LEN + 1];
	char text[UI_DISPLAY_TEXT_LEN + 1];
	callback_t *confirm;
	callback_t *reject;
} confirmState_t;

typedef union {
	scrollingState_t scrolling;
	confirmState_t confirm;
} displayState_t;


void ui_idle(void);

void ui_displayScrollingText(
        const char* headerStr,
        const char* bodyStr,
        callback_t* callback);

void ui_displayConfirm(
        const char* headerStr,
        const char* bodyStr,
        callback_t* confirm,
        callback_t* reject
);

void ui_checkUserConsent(
        security_policy_t policy,
        const char* headerStr,
        const char* bodyStr,
        callback_t* confirm,
        callback_t* reject
);

// responds to the host and resets
// processing
void respond_with_user_reject();

#endif
