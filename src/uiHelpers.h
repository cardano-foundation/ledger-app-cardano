#ifndef H_CARDANO_APP_UI_HELPERS
#define H_CARDANO_APP_UI_HELPERS

#include <stdint.h>
#include <stddef.h>
#include "securityPolicy.h"

typedef void callback_t(); // legacy
typedef void ui_callback_fn_t();

typedef enum {
	CALLBACK_NOT_RUN,
	CALLBACK_RUN,
} ui_callback_state_t;

typedef struct {
	ui_callback_state_t state;
	ui_callback_fn_t* confirm;
	ui_callback_fn_t* reject;
} ui_callback_t;


typedef struct {
	uint16_t initMagic;
	char header[30];
	char currentText[18];
	char fullText[200];
	size_t scrollIndex;
	ui_callback_t callback;
	#ifdef HEADLESS
	bool headlessShouldRespond;
	#endif
} scrollingState_t;

typedef struct {
	uint16_t initMagic;
	char header[30];
	char text[30];
	ui_callback_t callback;
	#ifdef HEADLESS
	bool headlessShouldRespond;
	#endif
} confirmState_t;

typedef union {
	scrollingState_t scrolling;
	confirmState_t confirm;
} displayState_t;


void ui_idle(void);

void ui_displayScrollingText(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* callback);

void ui_displayConfirm(
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* confirm,
        ui_callback_fn_t* reject
);

void ui_checkUserConsent(
        security_policy_t policy,
        const char* headerStr,
        const char* bodyStr,
        ui_callback_fn_t* confirm,
        ui_callback_fn_t* reject
);

void ui_displayBusy();

// responds to the host and resets
// processing
void respond_with_user_reject();

#endif
