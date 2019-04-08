#ifndef H_CARDANO_APP_BUF_VIEW
#define H_CARDANO_APP_BUF_VIEW

#include "common.h"
#include "cbor.h"
#include "assert.h"
#include "utils.h"
#include "endian.h"

typedef const uint8_t read_view__base_type;
typedef       uint8_t write_view__base_type;

#define __DEFINE_VIEW(name) \
	typedef struct { \
		name##__base_type* begin; \
		name##__base_type* ptr; \
		name##__base_type* end; \
	} name##_t; \
	\
	static inline name##_t make_##name(name##__base_type* begin, name##__base_type* end) \
	{ \
		name##_t result = { \
		                    .begin = begin, \
		                    .ptr = begin, \
		                    .end = end \
		                  }; \
		return result; \
	} \
	\
	static inline void name##_check(const name##_t* view) \
	{ \
		ASSERT(view->begin <= view->ptr); \
		ASSERT(view->ptr <= view->end); \
		ASSERT(view->end - view->begin <= BUFFER_SIZE_PARANOIA); \
	} \
	\
	static inline size_t name##_remaining_size(const name##_t* view) \
	{ \
		name##_check(view); \
		STATIC_ASSERT(sizeof(view->end - view->ptr) == sizeof(size_t), "bad size"); \
		return (size_t) (view->end - view->ptr); \
	} \
	\
	static inline size_t name##_processed_size(const name##_t* view) \
	{ \
		name##_check(view); \
		STATIC_ASSERT(sizeof(view->end - view->ptr) == sizeof(size_t), "bad size"); \
		return (size_t) (view->ptr - view->begin); \
	}


__DEFINE_VIEW(read_view)
__DEFINE_VIEW(write_view)


#define __DEFINE_VIEW_skip(name, err) \
	static inline void name##_skip(name##_t* view, size_t bytes) \
	{ \
		name##_check(view); \
		VALIDATE(bytes <= name##_remaining_size(view), err); \
		(view)->ptr += bytes; \
		name##_check(view); \
	}

__DEFINE_VIEW_skip(read_view, ERR_NOT_ENOUGH_INPUT)
__DEFINE_VIEW_skip(write_view, ERR_DATA_TOO_LARGE)

#define __VIEW_GENERIC_TEMPLATE(expr, suffix) \
	_Generic( \
	          (expr), \
	          read_view_t*: read_view_##suffix, \
	          write_view_t*: write_view_##suffix \
	        )

#define view_remainingSize(view)    __VIEW_GENERIC_TEMPLATE(view, remaining_size)(view)
#define view_processedSize(view)    __VIEW_GENERIC_TEMPLATE(view, processed_size)(view)
#define view_skipBytes(view, size)  __VIEW_GENERIC_TEMPLATE(view, skip)(view, size)
#define view_check(view)            __VIEW_GENERIC_TEMPLATE(view, check)(view)


static inline void view_appendToken(write_view_t* view, uint8_t type, uint64_t value)
{
	ASSERT(view_remainingSize(view) <= BUFFER_SIZE_PARANOIA);

	view->ptr += cbor_writeToken(type, value, view->ptr, view_remainingSize(view));
}

static inline void view_appendData(write_view_t* view, const uint8_t* dataBuffer, size_t dataSize)
{
	view_check(view);
	VALIDATE(dataSize <= view_remainingSize(view), ERR_DATA_TOO_LARGE);
	os_memcpy(view->ptr, dataBuffer, dataSize);
	view->ptr += dataSize;
	view_check(view);
}

static inline cbor_token_t view_readToken(read_view_t* view)
{
	const cbor_token_t token = cbor_parseToken(view->ptr, view_remainingSize(view));
	view_skipBytes(view, token.width + 1);
	return token;
}

// Note(ppershing): these macros expand to two arguments!
#define VIEW_REMAINING_TO_TUPLE_BUF_SIZE(view) (view)->ptr, view_remainingSize(view)
#define VIEW_PROCESSED_TO_TUPLE_BUF_SIZE(view) (view)->begin, view_processedSize(view)

typedef uint8_t uint_width1_t;
typedef uint16_t uint_width2_t;
typedef uint32_t uint_width4_t;
typedef uint64_t uint_width8_t;

#define __DEFINE_VIEW_parse_ube(width) \
	static inline uint_width##width##_t parse_u##width##be(read_view_t* view) { \
		VALIDATE(view_remainingSize(view) >= width, ERR_NOT_ENOUGH_INPUT); \
		uint_width##width##_t result = u##width##be_read(view->ptr); \
		view->ptr += width; \
		return result; \
	};



__DEFINE_VIEW_parse_ube(1)
__DEFINE_VIEW_parse_ube(2)
__DEFINE_VIEW_parse_ube(4)
__DEFINE_VIEW_parse_ube(8)


#endif
