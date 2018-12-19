#ifndef H_CARDANO_APP_STATE
#define H_CARDANO_APP_STATE

#include "getVersion.h"

typedef union {
	// Here should go states of all instructions
} instructionState_t;

typedef struct {
	uint8_t currentInstruction;
} globalState_t;

extern instructionState_t instructionState;
extern globalState_t global;

#endif
