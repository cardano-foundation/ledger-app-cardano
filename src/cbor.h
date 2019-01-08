#ifndef H_CARDANO_APP_CBOR
#define H_CARDANO_APP_CBOR

#include <stdint.h>
#include "stream.h"

// temporary handy values
enum {
	MT_UNSIGNED     = 0,
	MT_NEGATIVE     = 1,
	MT_BYTES        = 2,
	MT_TEXT         = 3,
	MT_ARRAY        = 4,
	MT_MAP          = 5,
	MT_TAG          = 6,
	MT_PRIMITIVES   = 7,

	VALUE_MASK      = 0b00011111,
	TYPE_MASK       = 0b11100000,
	INDEF           = VALUE_MASK,
};


enum {
	// raw tags
	TYPE_UNSIGNED     = MT_UNSIGNED << 5,
	//TYPE_NEGATIVE   = MT_NEGATIVE << 5,
	TYPE_BYTES        = MT_BYTES << 5,
	//TYPE_TEXT       = MT_TEXT << 5,
	TYPE_ARRAY        = MT_ARRAY << 5,
	TYPE_MAP          = MT_MAP << 5,
	TYPE_TAG          = MT_TAG << 5,
	TYPE_PRIMITIVES   = MT_PRIMITIVES << 5,

	// tag extensions
	TYPE_ARRAY_INDEF  = TYPE_ARRAY + INDEF,
	TYPE_INDEF_END    = TYPE_PRIMITIVES + INDEF,
};

typedef struct {
	uint8_t type;
	uint8_t width; // Contains number of *additional* bytes carrying the value
	uint64_t value;
} token_t;

token_t cbor_peekToken(const stream_t* s);
void cbor_advanceToken(stream_t* s);

void cbor_appendToken(stream_t* stream, uint8_t type, uint64_t value);

void run_cbor_test();
#endif
