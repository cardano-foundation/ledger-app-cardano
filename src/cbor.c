#include "cbor.h"
#include "stream.h"
#include "assert.h"
#include "errors.h"
#include <os.h>
#include <stdbool.h>
#include "endian.h"
#include "utils.h"

// Note(ppershing): consume functions should either
// a) *consume* expected value, or
// b) *throw* but not consume anything from the stream

static const uint64_t VALUE_MIN_W1 = 24;
static const uint64_t VALUE_MIN_W2 = (uint64_t) 1 << 8;
static const uint64_t VALUE_MIN_W4 = (uint64_t) 1 << 16;
static const uint64_t VALUE_MIN_W8 = (uint64_t) 1 << 32;


cbor_token_t cbor_parseToken(const uint8_t* buf, size_t size)
{
#define ENSURE_AVAILABLE_BYTES(x) if (x > size) THROW(ERR_NOT_ENOUGH_INPUT);
	ENSURE_AVAILABLE_BYTES(1);
	const uint8_t tag = buf[0];
	cbor_token_t result;

	// tag extensions first
	if (tag == CBOR_TYPE_ARRAY_INDEF || tag == CBOR_TYPE_INDEF_END) {
		result.type = tag;
		result.width = 0;
		result.value = 0;
		return result;
	}

	result.type = tag & CBOR_TYPE_MASK;

	switch (result.type) {
	case CBOR_TYPE_UNSIGNED:
	case CBOR_TYPE_BYTES:
	case CBOR_TYPE_ARRAY:
	case CBOR_TYPE_MAP:
	case CBOR_TYPE_TAG:
		break;
	default:
		// We don't know how to parse others
		// (particularly CBOR_TYPE_PRIMITIVES)
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	const uint8_t val = (tag & CBOR_VALUE_MASK);
	if (val < 24) {
		result.width = 0;
		result.value = val;
		return result;
	}

	// shift buffer
	// Holds minimum value for a given byte-width.
	// Anything below this is not canonical CBOR as
	// it could be represented by a shorter CBOR notation
	uint64_t limit_min;
	switch (val) {
	case 24:
		ENSURE_AVAILABLE_BYTES(1 + 1);
		result.width = 1;
		result.value = u1be_read(buf + 1);
		limit_min = VALUE_MIN_W1;
		break;
	case 25:
		ENSURE_AVAILABLE_BYTES(1 + 2);
		result.width = 2;
		result.value = u2be_read(buf + 1);
		limit_min = VALUE_MIN_W2;
		break;
	case 26:
		ENSURE_AVAILABLE_BYTES(1 + 4);
		result.width = 4;
		result.value = u4be_read(buf + 1);
		limit_min = VALUE_MIN_W4;
		break;
	case 27:
		ENSURE_AVAILABLE_BYTES(1 + 8);
		result.width = 8;
		result.value = u8be_read(buf + 1);
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
#undef ENSURE_AVAILABLE_BYTES
}

cbor_token_t cbor_peekToken(const stream_t* stream)
{
	return cbor_parseToken(stream_head(stream), stream_availableBytes(stream));
};

// TODO(ppershing): this naming is confusing for CBOR_TYPE_BYTES!
void cbor_advanceToken(stream_t* stream)
{
	cbor_token_t token = cbor_peekToken(stream);
	stream_advancePos(stream, 1 + token.width);
}

size_t cbor_writeToken(uint8_t type, uint64_t value, uint8_t* buffer, size_t bufferSize)
{
	ASSERT(bufferSize < BUFFER_SIZE_PARANOIA);

#define CHECK_BUF_LEN(requiredSize) if ((size_t) requiredSize > bufferSize) THROW(ERR_DATA_TOO_LARGE);
	if (type == CBOR_TYPE_ARRAY_INDEF || type == CBOR_TYPE_INDEF_END) {
		CHECK_BUF_LEN(1);
		buffer[0] = type;
		return 1;
	}

	if (type & CBOR_VALUE_MASK) {
		// type should not have any value
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	// Check sanity
	switch (type) {
	case CBOR_TYPE_UNSIGNED:
	case CBOR_TYPE_BYTES:
	case CBOR_TYPE_ARRAY:
	case CBOR_TYPE_MAP:
	case CBOR_TYPE_TAG:
		break;
	default:
		// not supported
		THROW(ERR_UNEXPECTED_TOKEN);
	}

	// Warning(ppershing): It might be tempting but we don't want to call stream_appendData() twice
	// Instead we have to construct the whole buffer at once to make append operation atomic.

	if (value < VALUE_MIN_W1) {
		CHECK_BUF_LEN(1);
		u1be_write(buffer, (uint8_t) (type | value));
		return 1;
	} else if (value < VALUE_MIN_W2) {
		CHECK_BUF_LEN(1 + 1);
		u1be_write(buffer, type | 24);
		u1be_write(buffer + 1, (uint8_t) value);
		return 1 + 1;
	} else if (value < VALUE_MIN_W4) {
		CHECK_BUF_LEN(1 + 2);
		u1be_write(buffer, type | 25);
		u2be_write(buffer + 1, (uint16_t) value);
		return 1 + 2;
	} else if (value < VALUE_MIN_W8) {
		CHECK_BUF_LEN(1 + 4);
		u1be_write(buffer, type | 26);
		u4be_write(buffer + 1, (uint32_t) value);
		return 1 + 4;
	} else {
		CHECK_BUF_LEN(1 + 8);
		u1be_write(buffer, type | 27);
		u8be_write(buffer + 1, value);
		return 1 + 8;
	}
#undef CHECK_BUF_LEN
}

void cbor_appendToken(stream_t* stream, uint8_t type, uint64_t value)
{
	uint8_t buf[1 + 8]; // 1 for preamble, 8 for possible uint64 value data
	size_t bufLen = cbor_writeToken(type, value, buf, SIZEOF(buf));
	stream_appendData(stream, buf, bufLen);
}

// Expect & consume CBOR token with specific type and value
void cbor_takeTokenWithValue(stream_t* stream, uint8_t expectedType, uint64_t expectedValue)
{
	const cbor_token_t token = cbor_peekToken(stream);
	if (token.type != expectedType || token.value != expectedValue) {
		THROW(ERR_UNEXPECTED_TOKEN);
	}
	cbor_advanceToken(stream);
}

// Expect & consume CBOR token with specific type, return value
uint64_t cbor_takeToken(stream_t* stream, uint8_t expectedType)
{
	const cbor_token_t token = cbor_peekToken(stream);
	if (token.type != expectedType) {
		THROW(ERR_UNEXPECTED_TOKEN);
	}
	cbor_advanceToken(stream);
	return token.value;
}

// Is next CBOR token indefinite array/map end?
bool cbor_peekNextIsIndefEnd(stream_t* stream)
{
	cbor_token_t head = cbor_peekToken(stream);
	return (head.type == CBOR_TYPE_INDEF_END);
}
