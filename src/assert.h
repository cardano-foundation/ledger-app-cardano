#ifndef H_CARDANO_APP_UTIL
#define H_CARDANO_APP_UTIL

#include <stdint.h>
#include "errors.h"

#define STATIC_ASSERT(COND,MSG) typedef char static_assert__##MSG[(COND)?1:-1] __attribute__((unused))

extern void assert(int cond, const char* msg);

// Note(ppershing): I like macro-like uppercase version better
// because it captures reader's attention.
#define ASSERT(cond, msg) assert(cond, msg)

#endif
