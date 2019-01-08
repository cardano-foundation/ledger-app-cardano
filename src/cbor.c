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

void cbor_appendToken(stream_t* stream, uint8_t type, uint64_t value)
{
	uint8_t buf[1+8]; // 1 for preamble, 8 for possible uint64 value data
	uint8_t bufLen = 0;
	if (type == TYPE_ARRAY_INDEF || type == TYPE_INDEF_END) {
		buf[0] = type;
		stream_appendData(stream, buf, 1);
		return;
	}

	if (type & VALUE_MASK) {
		// type should not have any value
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	// Check sanity
	switch (type) {
	case TYPE_UNSIGNED:
	case TYPE_BYTES:
	case TYPE_ARRAY:
	case TYPE_MAP:
	case TYPE_TAG:
		break;
	default:
		// not supported
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	// Warning(ppershing): It might be tempting but we don't want to call stream_appendData() twice
	// Instead we have to construct the whole buffer at once to make append operation atomic.

	if (value < VALUE_MIN_W1) {
		u1be_write(buf, type | value);
		bufLen = 1;
	} else if (value < VALUE_MIN_W2) {
		u1be_write(buf, type | 24);
		u1be_write(buf + 1, value);
		bufLen = 1 + 1;
	} else if (value < VALUE_MIN_W4) {
		u1be_write(buf, type | 25);
		u2be_write(buf + 1, value);
		bufLen = 1 + 2;
	} else if (value < VALUE_MIN_W8) {
		u1be_write(buf, type | 26);
		u4be_write(buf + 1, value);
		bufLen = 1 + 4;
	} else {
		buf[0] = type | 27;
		u8be_write(buf + 1, value);
		bufLen = 1 + 8;
	}
	stream_appendData(stream, buf, bufLen);
}
