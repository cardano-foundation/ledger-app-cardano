#ifndef H_CARDANO_APP_UTIL
#define H_CARDANO_APP_UTIL

#include "common.h"

// from https://stackoverflow.com/questions/19343205/c-concatenating-file-and-line-macros
#define _TO_STR1_(x) #x
#define _TO_STR2_(x) _TO_STR1_(x)

#define STATIC_ASSERT _Static_assert

extern void assert(int cond, const char* msgStr);

// Note(ppershing): I like macro-like uppercase version better
// because it captures reader's attention.
#define ASSERT_WITH_MSG(cond, msg) assert(cond, msg)


#define _MAX_ASSERT_LENGTH_ 25
// Shortens a string literal by skipping some prefix
#define _SHORTEN_(strLiteral, size) \
	(sizeof(strLiteral) > size \
	 ? (strLiteral) + sizeof(strLiteral) - size \
	 : strLiteral \
	)

#define _FILE_LINE_ __FILE__ ":" _TO_STR2_(__LINE__)

#define ASSERT(cond) assert((cond), _SHORTEN_( _FILE_LINE_, _MAX_ASSERT_LENGTH_))

#endif
