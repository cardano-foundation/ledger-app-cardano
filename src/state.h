#ifndef H_STATE
#define H_STATE

#include "getVersion.h"

typedef union {
	showAboutState_t showAboutState;
} instructionState_t;

typedef struct {
	uint8_t currentInstruction;
} globalState_t;

extern instructionState_t instructionState;
extern globalState_t global;

#endif
