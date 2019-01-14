#ifndef H_CARDANO_APP_STATE
#define H_CARDANO_APP_STATE

#include "getVersion.h"
#include "stream.h"

typedef struct {
	stream_t s;
} testsGlobal_t;

typedef union {
	// Here should go states of all instructions
	testsGlobal_t testsGlobal;
} instructionState_t;

typedef struct {
	uint8_t currentInstruction;
} globalState_t;

extern instructionState_t instructionState;
extern globalState_t global;

#endif
