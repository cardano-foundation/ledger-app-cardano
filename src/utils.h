#ifndef H_CARDANO_APP_UTILS
#define H_CARDANO_APP_UTILS

#include "assert.h"

// Safe array length, does not compile if you take ARRAY_LEN of ptr
// Note: this is also "safe" for function arguments i.e.
// `void fn(int z[100]) { ARRAY_LEN(z); }` fails to compile because
// C silently converts `int z[100]` -> `int* z` in this case
// Note: could yield false positives. In that case, use judiciously UNSAFE_ARRAY_LEN
#define ARRAY_LEN(x) \
	/* Note: this macro uses gcc extension: */ \
	/* https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html */ \
	({ \
	    STATIC_ASSERT(sizeof(x) != sizeof(void *), __possibly_array_len_of_ptr); \
	    UNSAFE_ARRAY_LEN(x); \
	})

#define UNSAFE_ARRAY_LEN(x) (sizof(x) / sizeof((x)[0]))

// Given that memset is root of many problems, a bit of paranoia is good.
// If you don't believe, just check out https://www.viva64.com/en/b/0360/
//
// TL;DR; We want to avoid cases such as:
//
// int[10] x; void fn(int* ptr) { memset(ptr, 0, sizeof(ptr)); }
// int[10][20] x; memset(x, 0, sizeof(x));
// struct_t* ptr; memset(ptr, 0, sizeof(ptr));
// int[10] x; memset(x, 0, 10);
//
// The best way is to provide an expected type and make sure expected and
// inferred type have the same size.
#define MEMCLEAR(ptr, expected_type) \
{ \
    STATIC_ASSERT(sizeof(expected_type) == sizeof(*(ptr)), __safe_memclear); \
    os_memset(ptr, 0, sizeof(expected_type)); \
}

#endif
