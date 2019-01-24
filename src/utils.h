#ifndef H_CARDANO_APP_UTILS
#define H_CARDANO_APP_UTILS

#include <os.h>
#include "assert.h"


// Does not compile if x is pointer of some kind
// See http://zubplot.blogspot.com/2015/01/gcc-is-wonderful-better-arraysize-macro.html
#define ARRAY_NOT_A_PTR(x) \
     (sizeof(__typeof__(int[1 - 2 * \
           !!__builtin_types_compatible_p(__typeof__(x), \
                 __typeof__(&x[0]))])) * 0)


// Safe array length, does not compile if you accidentally supply a pointer
#define ARRAY_LEN(arr) \
    (sizeof(arr) / sizeof((arr)[0]) + ARRAY_NOT_A_PTR(arr))

// Does not compile if x *might* be a pointer of some kind
// Might produce false positives on small structs...
// Note: ARRAY_NOT_A_PTR does not compile if arg is a struct so this is a workaround
#define SIZEOF_NOT_A_PTR(var) \
    (sizeof(__typeof(int[0 - (sizeof(var) == sizeof((void *)0))])) * 0)

// Safe version of SIZEOF, does not compile if you accidentally supply a pointer
#define SIZEOF(var) \
    (sizeof(var) + SIZEOF_NOT_A_PTR(var))


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
    STATIC_ASSERT(sizeof(expected_type) == sizeof(*(ptr)), "bad memclear parameters"); \
    os_memset(ptr, 0, sizeof(expected_type)); \
}

// Helper function to check APDU request parameters
#define VALIDATE(cond, error) {if (!(cond)) THROW(error);}

// Helper functions for ranges
// TODO(ppershing): make more type safe?
#define BEGIN(buf) buf
// Note: SIZEOF would not work if buf is not uin8_t*
#define END(buf) (buf + ARRAY_LEN(buf))

// Any buffer claiming to be longer than this is a bug
// (we anyway have only 4KB of memory)
#define BUFFER_SIZE_PARANOIA 1024

#define PTR_PIC(ptr) ((__typeof__(ptr)) PIC(ptr))

#define ITERATE(it, arr) for (__typeof__(&(arr[0])) it = BEGIN(arr); it < END(arr); it++)



// Note(ppershing): following helper macros *UPDATE* ptr

#define BUF_PTR_APPEND_TOKEN(ptr, end, type, value) \
	{ \
		STATIC_ASSERT(SIZEOF(*ptr) == 1, "bad pointer type"); \
		STATIC_ASSERT(SIZEOF(*end) == 1, "bad pointer type"); \
		ASSERT(ptr <= end); \
		ptr += cbor_writeToken(type, value, ptr, end - ptr); \
		ASSERT(ptr <= end); \
	}

#define BUF_PTR_APPEND_DATA(ptr, end, dataBuffer, dataSize) \
	{ \
		STATIC_ASSERT(SIZEOF(*ptr) == 1, "bad pointer type"); \
		STATIC_ASSERT(SIZEOF(*end) == 1, "bad pointer type"); \
		ASSERT(ptr <= end); \
		ASSERT(dataSize < BUFFER_SIZE_PARANOIA); \
		if (ptr + dataSize > end) THROW(ERR_DATA_TOO_LARGE); \
		os_memcpy(ptr, dataBuffer, dataSize); \
		ptr += dataSize; \
		ASSERT(ptr <= end); \
	}

// Note: unused removes unused warning but does not warn if you suddenly
// start using such variable. deprecated deals with that.
#define MARK_UNUSED __attribute__ ((unused, deprecated))

#endif
