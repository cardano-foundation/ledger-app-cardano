#ifndef H_CARDANO_APP_UI_HELPERS
#define H_CARDANO_APP_UI_HELPERS

#include <stdint.h>

typedef void callback_t();

#define DISPLAY_TEXT_LEN 32
#define MAX_TEXT_LEN 200

typedef struct {
	char header[DISPLAY_TEXT_LEN + 1];
	char currentText[DISPLAY_TEXT_LEN + 1];
	char fullText[MAX_TEXT_LEN + 1];
	uint16_t scrollIndex;
	callback_t *callback;
} scrollingState_t;

typedef struct {
	char header[DISPLAY_TEXT_LEN + 1];
	char text[DISPLAY_TEXT_LEN + 1];
	callback_t *confirm;
	callback_t *reject;
} confirmState_t;

typedef union {
	scrollingState_t scrolling;
	confirmState_t confirm;
} displayState_t;


void ui_idle(void);

void displayScrollingText(
        const char* headerStr,
        const char* bodyStr,
        callback_t* callback);

void displayConfirm(
        const char* headerStr,
        const char* bodyStr,
        callback_t* confirm,
        callback_t* reject
);

#endif
