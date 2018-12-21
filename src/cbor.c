#include "cbor.h"
#include "stream.h"
#include "assert.h"
#include "errors.h"
#include <os.h>

#define U1BE(buf, off) ((uint8_t)(buf[off]))
#define U8BE(buf, off) (((uint64_t)(U4BE(buf, off))     << 32) | ((uint64_t)(U4BE(buf, off + 4)) & 0xFFFFFFFF))


// Note(ppershing): consume functions should either
// a) *consume* expected value, or
// b) *throw* but not consume anything from the stream

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

	const uint8_t* buf = stream_head(stream);
	// Holds minimum value for a given byte-width.
	// Anything below this is not canonical CBOR as
	// it could be represented by a shorter CBOR notation
	uint64_t limit_min;
	switch (val) {
	case 24:
		stream_ensureAvailableBytes(stream, 1 + 1);
		result.width = 1;
		result.value = U1BE(buf, 1);
		limit_min = 24;
		break;
	case 25:
		stream_ensureAvailableBytes(stream, 1 + 2);
		result.width = 2;
		result.value = U2BE(buf, 1);
		limit_min = (uint64_t) 1 << 8;
		break;
	case 26:
		stream_ensureAvailableBytes(stream, 1 + 4);
		result.width = 4;
		result.value = U4BE(buf, 1);
		limit_min = (uint64_t) 1 << 16;
		break;
	case 27:
		stream_ensureAvailableBytes(stream, 1 + 8);
		result.width = 8;
		result.value = U8BE(buf, 1);
		limit_min = (uint64_t) 1 << 32;
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
