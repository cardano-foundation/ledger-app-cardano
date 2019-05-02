#ifndef H_CARANO_APP_ENDIAN
#define H_CARANO_APP_ENDIAN

#include <stdint.h>
#include "assert.h"

inline void u1be_write(uint8_t* outBuffer, uint8_t value)
{
	outBuffer[0] = value;
}

inline void u2be_write(uint8_t* outBuffer, uint16_t value)
{
	u1be_write(outBuffer, value >> 8);
	u1be_write(outBuffer + 1, value & 0xFF);
}

inline void u4be_write(uint8_t* outBuffer, uint32_t value)
{
	u2be_write(outBuffer, value >> 16);
	u2be_write(outBuffer + 2, value & 0xFFff);
}

inline void u8be_write(uint8_t* outBuffer, uint64_t value)
{
	u4be_write(outBuffer, (value >> 32));
	u4be_write(outBuffer + 4, value & 0xFFffFFff);
}


inline uint8_t u1be_read(const uint8_t* inBuffer)
{
	return inBuffer[0];
}

inline uint16_t u2be_read(const uint8_t* inBuffer)
{
	STATIC_ASSERT(sizeof(uint32_t) == sizeof(unsigned), "bad unsigned size");

	// bitwise OR promotes unsigned types smaller than int to unsigned
	return (uint16_t) (
	               ((uint32_t) (u1be_read(inBuffer) << 8)) | ((uint32_t) (u1be_read(inBuffer + 1)))
	       );
}

inline uint32_t u4be_read(const uint8_t* inBuffer)
{
	return ((uint32_t) u2be_read(inBuffer) << 16) | (uint32_t)(u2be_read(inBuffer + 2));
}

inline uint64_t u8be_read(const uint8_t* inBuffer)
{
	return ((uint64_t) u4be_read(inBuffer) << 32u) | (uint64_t) (u4be_read(inBuffer + 4));
}

void run_endian_test();
#endif
