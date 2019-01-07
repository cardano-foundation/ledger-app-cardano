#ifndef H_CARDANO_APP_UTIL
#define H_CARDANO_APP_UTIL

#include <stdint.h>
#include "errors.h"

#define STATIC_ASSERT(COND,MSG) typedef char static_assert__##MSG[(COND)?1:-1] __attribute__((unused))

extern void assert(int cond, const char* msg);

// Note(ppershing): I like macro-like uppercase version better
// because it captures reader's attention.
#define ASSERT_WITH_MSG(cond, msg) assert(cond, msg)


// from https://stackoverflow.com/questions/19343205/c-concatenating-file-and-line-macros
#define _TO_STR1_(x) #x
#define _TO_STR2_(x) _TO_STR1_(x)

#define ASSERT(cond) assert(cond, __FILE__ ":" _TO_STR2_(__LINE__))

#endif
