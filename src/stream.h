#ifndef H_CARDANO_STREAM
#define H_CARDANO_STREAM

#include <stdint.h>

typedef uint32_t streamSize_t;

enum {
	STREAM_BUFFER_SIZE = 300u
};

typedef struct {
	// TODO(ppershing): buffer size, int->?
	uint8_t buffer[STREAM_BUFFER_SIZE]; // buffer
	streamSize_t bufferPos; // position inside current buffer
	streamSize_t bufferEnd; // position of current buffer end buffer[bufferEnd] is *behind* last data
	streamSize_t streamPos; // position inside whole input stream
} stream_t;

streamSize_t stream_availableBytes(const stream_t* stream);
void stream_ensureAvailableBytes(const stream_t* stream, streamSize_t len);
void stream_advancePos(stream_t* stream, streamSize_t len);
uint8_t stream_peekByte(const stream_t* stream);
const uint8_t* stream_head(const stream_t* stream);
streamSize_t stream_unusedBytes(const stream_t* stream);
uint8_t stream_appendData(stream_t* stream, const uint8_t* data, streamSize_t len);

// internal (declared for testing)
void stream_shift(stream_t* stream);

#endif
