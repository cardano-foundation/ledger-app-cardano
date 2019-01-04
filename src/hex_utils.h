#ifndef H_CARDANO_APP_HEX_UTILS
#define H_CARDANO_APP_HEX_UTILS

#include "stream.h"

uint8_t hex_parseNibble(const char c);

uint8_t hex_parseNibblePair(const char* str);

void stream_appendFromHexString(stream_t* s, const char* str);

void run_hex_test();
#endif
