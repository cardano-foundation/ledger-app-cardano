#ifndef H_CARDANO_APP_HEX_UTILS
#define H_CARDANO_APP_HEX_UTILS

#include "stream.h"
#include <stddef.h>

uint8_t hex_parseNibble(const char c);

uint8_t hex_parseNibblePair(const char* str);

void stream_appendFromHexString(stream_t* s, const char* str);

void stream_initFromHexString(stream_t* s, const char* str);

size_t parseHexString(const char* str, uint8_t* buf, size_t maxLen);

void run_hex_test();
#endif
