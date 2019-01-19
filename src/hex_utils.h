#ifndef H_CARDANO_APP_HEX_UTILS
#define H_CARDANO_APP_HEX_UTILS

#include "common.h"
#include "stream.h"

uint8_t hex_parseNibble(const char c);

uint8_t hex_parseNibblePair(const char* buffer);

void stream_appendFromHexString(stream_t* s, const char* inStr);

void stream_initFromHexString(stream_t* s, const char* inStr);

size_t parseHexString(const char* inStr, uint8_t* outBuffer, size_t outMaxSize);

void run_hex_test();
#endif
