#ifndef H_CARDANO_APP_UTIL
#define H_CARDANO_APP_UTIL

#include <stdint.h>
#include "errors.h"

#define STATIC_ASSERT(COND,MSG) typedef char static_assert__##MSG[(COND)?1:-1] __attribute__((unused))


// use different name to avoid potential
// naming conflicts with assert
extern void checkOrFail(int cond, const char* msg);

#define CHECK_OR_FAIL(cond, msg) checkOrFail(cond, msg)

#endif
