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


#define ASSERT_TYPE(expr, expected_type) \
	STATIC_ASSERT( \
	               __builtin_types_compatible_p(__typeof__((expr)), expected_type), \
	               "Wrong type" \
	             )

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
	do { \
		STATIC_ASSERT(sizeof(expected_type) == sizeof(*(ptr)), "bad memclear parameters"); \
		os_memset(ptr, 0, sizeof(expected_type)); \
	} while(0)

// Helper function to check APDU request parameters
#define VALIDATE(cond, error) \
	do {\
		if (!(cond)) { \
			PRINTF("Validation Error in %s: %d\n", __FILE__, __LINE__); \
			THROW(error); \
		} \
	} while(0)

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


// *INDENT-OFF*
// TODO(ppershing): maybe we can also join this with PARSER macros
// in attestUtxo.c

// Warning: Following macros are *NOT* brace-balanced by design!
// The macros simplify writing resumable logic that needs to happen over
// multiple calls.

// Example usage:
// UI_STEP_BEGIN(ctx->ui_step);
// UI_STEP(1) {do something & setup callback}
// UI_STEP(2) {do something & setup callback}
// UI_STEP_END(-1); // invalid state

#define UI_STEP_BEGIN(VAR) \
	{ \
		int* __ui_step_ptr = &(VAR); \
		switch(*__ui_step_ptr) { \
			default: { \
				ASSERT(false);

#define UI_STEP(NEXT_STEP) \
				*__ui_step_ptr = NEXT_STEP; \
				break; \
			} \
			case NEXT_STEP: {

#define UI_STEP_END(INVALID_STEP) \
				*__ui_step_ptr = INVALID_STEP; \
				break; \
			} \
		} \
	}

// Early exit to another state, unused for now
// #define UI_STEP_JUMP(NEXT_STEP) *__ui_step_ptr = NEXT_STEP; break;

// *INDENT-ON*


// Note: unused removes unused warning but does not warn if you suddenly
// start using such variable. deprecated deals with that.
#define MARK_UNUSED __attribute__ ((unused, deprecated))

#if DEVEL
#define TRACE(...) \
	do { \
		PRINTF("[%s:%d] ", __func__, __LINE__); \
		PRINTF("" __VA_ARGS__); \
		PRINTF("\n"); \
	} while(0)
#else
#define TRACE(...)
#endif
#endif
