#ifndef H_CARDANO_APP_UI_HELPERS
#define H_CARDANO_APP_UI_HELPERS

#include <stdint.h>

typedef void confirm_cb_t();

#define DISPLAY_TEXT_LEN 16
#define MAX_TEXT_LEN 200

typedef struct {
	uint8_t header[DISPLAY_TEXT_LEN + 1];
	uint8_t currentText[DISPLAY_TEXT_LEN + 1];
	uint8_t fullText[MAX_TEXT_LEN];
	uint16_t scrollIndex;
	confirm_cb_t *callback;
} displayState_t;

void displayScrollingText(
        char* header, uint16_t header_len,
        char* text, uint16_t text_len,
        confirm_cb_t* callback);

#endif
