#ifndef H_CARDANO_APP_UI_HELPERS
#define H_CARDANO_APP_UI_HELPERS

#include <stdint.h>

typedef void callback_t();

#define DISPLAY_TEXT_LEN 16
#define MAX_TEXT_LEN 200

typedef struct {
	uint8_t header[DISPLAY_TEXT_LEN + 1];
	uint8_t currentText[DISPLAY_TEXT_LEN + 1];
	uint8_t fullText[MAX_TEXT_LEN];
	uint16_t scrollIndex;
	callback_t *callback;
} scrollingState_t;

typedef struct {
	uint8_t header[DISPLAY_TEXT_LEN + 1];
	uint8_t text[DISPLAY_TEXT_LEN + 1];
	callback_t *confirm;
	callback_t *reject;
} confirmState_t;

typedef union {
	scrollingState_t scrolling;
	confirmState_t confirm;
} displayState_t;


void ui_idle(void);

void displayScrollingText(
        const char* header,
        const char* text,
        callback_t* callback);

void displayConfirm(
        const char* header,
        const char* text,
        callback_t* confirm,
        callback_t* reject
);

#endif
