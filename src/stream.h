#ifndef H_CARDANO_STREAM
#define H_CARDANO_STREAM

#include <stdint.h>
#include <stddef.h>

enum {
	STREAM_BUFFER_SIZE = 300u,
	STREAM_INIT_MAGIC = 4247,
};

typedef struct {
	// TODO(ppershing): buffer size, int->?
	uint8_t buffer[STREAM_BUFFER_SIZE]; // buffer
	uint16_t isInitialized; // last defense against buffer overflow corruption
	size_t bufferPos; // position inside current buffer
	size_t bufferEnd; // position of current buffer end buffer[bufferEnd] is *behind* last data
	size_t streamPos; // position inside whole input stream
} stream_t;

void stream_init(stream_t* stream);
size_t stream_availableBytes(const stream_t* stream);
void stream_ensureAvailableBytes(const stream_t* stream, size_t requiredSize);
void stream_advancePos(stream_t* stream, size_t advanceBy);
uint8_t stream_peekByte(const stream_t* stream);
const uint8_t* stream_head(const stream_t* stream);
size_t stream_unusedBytes(const stream_t* stream);
void stream_appendData(stream_t* stream, const uint8_t* inBuffer, size_t inSize);

// internal (declared for testing)
void stream_shift(stream_t* stream);
void run_stream_test();
#endif
