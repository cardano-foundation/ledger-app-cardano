#include "stream.h"
#include "assert.h"
#include <os.h>
#include <stdbool.h>

STATIC_ASSERT( STREAM_BUFFER_SIZE == sizeof(((stream_t*) 0)->buffer), __stream_buffer_bad_size);

void stream_init(stream_t* stream)
{
	STATIC_ASSERT(sizeof(*stream) > STREAM_BUFFER_SIZE, __paranoia);
	os_memset(stream, 0, sizeof(*stream));
	stream->isInitialized = STREAM_INIT_MAGIC;
}

// Checks stream internal state consistency
void stream_checkState(const stream_t* stream)
{
	ASSERT(
	        stream->isInitialized == STREAM_INIT_MAGIC,
	        "stream not initialized"
	);
	ASSERT(
	        stream->bufferPos <= stream->bufferEnd,
	        "Invalid buffer pos"
	);
	ASSERT(
	        stream->bufferEnd <= STREAM_BUFFER_SIZE,
	        "Invalid buffer end"
	);
	ASSERT(
	        stream->streamPos >= stream->bufferPos,
	        "Unexpected input pos"
	);
}

// Returns number of bytes that can be read from the stream right now
streamSize_t stream_availableBytes(const stream_t* stream)
{
	stream_checkState(stream);
	return stream->bufferEnd - stream->bufferPos;
}

// Ensures that stream has at least @len available bytes
void stream_ensureAvailableBytes(const stream_t* stream, streamSize_t len)
{
	if (stream_availableBytes(stream) < len) {
		// Soft error -- wait for next APDU
		THROW(ERR_NOT_ENOUGH_INPUT);
	}
}

// Advances stream by @len
void stream_advancePos(stream_t* stream, streamSize_t len)
{
	// just in case somebody changes to signed
	ASSERT(len >= 0, "Bad length");
	ASSERT(
	        stream->streamPos + len > stream->streamPos,
	        "Wraparound"
	);
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
	streamSize_t len = stream_availableBytes(stream);
	streamSize_t shift = stream->bufferPos;

	os_memmove(stream->buffer, &stream->buffer[shift], len);
	stream->bufferPos -= shift;
	stream->bufferEnd -= shift;

	ASSERT(
	        len == stream_availableBytes(stream),
	        "bad shift"
	);
	stream_checkState(stream);
}

// Returns the number of bytes that can be appended right now to the stream
streamSize_t stream_unusedBytes(const stream_t* stream)
{
	stream_checkState(stream);
	return STREAM_BUFFER_SIZE - stream_availableBytes(stream);
}


uint8_t stream_appendData(stream_t* stream, const uint8_t* data, streamSize_t len)
{
	stream_checkState(stream);
	if (len > stream_unusedBytes(stream)) {
		THROW(ERR_DATA_TOO_LARGE);
	}
	// Prepare space
	stream_shift(stream);

	ASSERT(stream->bufferEnd + len <= STREAM_BUFFER_SIZE,
	       "unexpected stream space");
	os_memmove(&stream->buffer[stream->bufferEnd], data, len);
	stream->bufferEnd += len;
	stream_checkState(stream);
}
