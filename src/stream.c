#include "stream.h"
#include "assert.h"
#include <os.h>
#include <stdbool.h>

// paranoia
STATIC_ASSERT(STREAM_BUFFER_SIZE == sizeof(((stream_t*) 0)->buffer), __stream_buffer_bad_size);

void stream_init(stream_t* stream)
{
	STATIC_ASSERT(sizeof(*stream) > STREAM_BUFFER_SIZE, __paranoia);
	os_memset(stream, 0, sizeof(*stream));
	stream->isInitialized = STREAM_INIT_MAGIC;
}

// Checks stream internal state consistency
void stream_checkState(const stream_t* stream)
{
	// Should be initialized
	ASSERT(stream->isInitialized == STREAM_INIT_MAGIC);
	// Invalid buffer pos
	ASSERT(stream->bufferPos <= stream->bufferEnd);
	// Invalid buffer end
	ASSERT(stream->bufferEnd <= STREAM_BUFFER_SIZE);
	// Unexpected input pos
	ASSERT(stream->streamPos >= stream->bufferPos);
}

// Returns number of bytes that can be read from the stream right now
size_t stream_availableBytes(const stream_t* stream)
{
	stream_checkState(stream);
	return stream->bufferEnd - stream->bufferPos;
}

// Ensures that stream has at least @len available bytes
void stream_ensureAvailableBytes(const stream_t* stream, size_t len)
{
	if (stream_availableBytes(stream) < len) {
		// Soft error -- wait for next APDU
		THROW(ERR_NOT_ENOUGH_INPUT);
	}
}

// Advances stream by @len
void stream_advancePos(stream_t* stream, size_t len)
{
	// just in case somebody changes to signed
	ASSERT(len >= 0);
	// Wraparound
	ASSERT(stream->streamPos + len > stream->streamPos);

	stream_checkState(stream);
	stream_ensureAvailableBytes(stream, len);
	stream->bufferPos += len;
	stream->streamPos += len;
}

// Returns the first byte of the stream without advancing
uint8_t stream_peekByte(const stream_t* stream)
{
	stream_checkState(stream);
	stream_ensureAvailableBytes(stream, 1);

	return stream->buffer[stream->bufferPos];
}

// Returns pointer to the first byte of the stream
const uint8_t* stream_head(const stream_t* stream)
{
	stream_checkState(stream);
	return &stream->buffer[stream->bufferPos];
}

// Shifts stream buffer in order to accomodate more data
void stream_shift(stream_t* stream)
{
	stream_checkState(stream);
	size_t len = stream_availableBytes(stream);
	size_t shift = stream->bufferPos;

	os_memmove(stream->buffer, &stream->buffer[shift], len);
	stream->bufferPos -= shift;
	stream->bufferEnd -= shift;

	// Check shift consistency
	ASSERT(len == stream_availableBytes(stream));
	stream_checkState(stream);
}

// Returns the number of bytes that can be appended right now to the stream
size_t stream_unusedBytes(const stream_t* stream)
{
	stream_checkState(stream);
	return STREAM_BUFFER_SIZE - stream_availableBytes(stream);
}


void stream_appendData(stream_t* stream, const uint8_t* data, size_t len)
{
	stream_checkState(stream);
	if (len > stream_unusedBytes(stream)) {
		THROW(ERR_DATA_TOO_LARGE);
	}
	// Prepare space
	stream_shift(stream);

	// Check stream space
	ASSERT(stream->bufferEnd + len <= STREAM_BUFFER_SIZE);
	os_memmove(&stream->buffer[stream->bufferEnd], data, len);
	stream->bufferEnd += len;
	stream_checkState(stream);
}
