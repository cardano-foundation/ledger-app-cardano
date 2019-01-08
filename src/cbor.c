#include "cbor.h"
#include "stream.h"
#include "assert.h"
#include "errors.h"
#include <os.h>
#include <stdbool.h>
#include "endian.h"

// Note(ppershing): consume functions should either
// a) *consume* expected value, or
// b) *throw* but not consume anything from the stream

static const uint64_t VALUE_MIN_W1 = 24;
static const uint64_t VALUE_MIN_W2 = (uint64_t) 1 << 8;
static const uint64_t VALUE_MIN_W4 = (uint64_t) 1 << 16;
static const uint64_t VALUE_MIN_W8 = (uint64_t) 1 << 32;

token_t cbor_peekToken(const stream_t* stream)
{

	const uint8_t tag = stream_peekByte(stream);
	token_t result;

	// tag extensions first
	if (tag == TYPE_ARRAY_INDEF || tag == TYPE_INDEF_END) {
		result.type = tag;
		result.width = 0;
		result.value = 0;
		return result;
	}

	result.type = tag & TYPE_MASK;

	switch (result.type) {
	case TYPE_UNSIGNED:
	case TYPE_BYTES:
	case TYPE_ARRAY:
	case TYPE_MAP:
	case TYPE_TAG:
		break;
	default:
		// We don't know how to parse others
		// (particularly TYPE_PRIMITIVES)
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	const uint8_t val = (tag & VALUE_MASK);
	if (val < 24) {
		result.width = 0;
		result.value = val;
		return result;
	}

	const uint8_t* buf = stream_head(stream) + 1;
	// Holds minimum value for a given byte-width.
	// Anything below this is not canonical CBOR as
	// it could be represented by a shorter CBOR notation
	uint64_t limit_min;
	switch (val) {
	case 24:
		stream_ensureAvailableBytes(stream, 1 + 1);
		result.width = 1;
		result.value = u1be_read(buf);
		limit_min = VALUE_MIN_W1;
		break;
	case 25:
		stream_ensureAvailableBytes(stream, 1 + 2);
		result.width = 2;
		result.value = u2be_read(buf);
		limit_min = VALUE_MIN_W2;
		break;
	case 26:
		stream_ensureAvailableBytes(stream, 1 + 4);
		result.width = 4;
		result.value = u4be_read(buf);
		limit_min = VALUE_MIN_W4;
		break;
	case 27:
		stream_ensureAvailableBytes(stream, 1 + 8);
		result.width = 8;
		result.value = u8be_read(buf);
		limit_min = VALUE_MIN_W8;
		break;
	default:
		// Values above 27 are not valid in CBOR.
		// Exception is indefinite length marker
		// but this has been handled separately.
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	if (result.value < limit_min) {
		// This isn't canonical CBOR
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	return result;
}

// TODO(ppershing): this naming is confusing for TYPE_BYTES!
void cbor_advanceToken(stream_t* stream)
{
	token_t token = cbor_peekToken(stream);
	stream_advancePos(stream, 1 + token.width);
}
