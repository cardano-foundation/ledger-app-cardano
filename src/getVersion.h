#ifndef H_GET_VERSION
#define H_GET_VERSION

#include "handlers.h"

// Must be in format x.y.z
#define APPVERSION "0.0.1"

handler_fn_t handleGetVersion;
handler_fn_t handleShowAbout;

#define DISPLAY_TEXT_LEN 16

typedef struct {
	uint8_t scrollIndex;
	// TODO(ppershing): why is there safety margin?
	uint8_t fullText[100];
	uint8_t currentText[DISPLAY_TEXT_LEN + 1];
} showAboutState_t;

#endif // H_GET_VERSION
