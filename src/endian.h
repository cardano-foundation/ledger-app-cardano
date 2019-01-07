#ifndef H_CARANO_APP_ENDIAN
#define H_CARANO_APP_ENDIAN

#include <stdint.h>

inline void u1be_write(uint8_t* dst, uint8_t value)
{
	dst[0] = value;
}

inline void u2be_write(uint8_t* dst, uint16_t value)
{
	u1be_write(dst, value >> 8);
	u1be_write(dst + 1, value & 0xFF);
}

inline void u4be_write(uint8_t* dst, uint32_t value)
{
	u2be_write(dst, value >> 16);
	u2be_write(dst + 2, value & 0xFFff);
}

inline void u8be_write(uint8_t* dst, uint64_t value)
{
	u4be_write(dst, (value >> 32));
	u4be_write(dst + 4, value & 0xFFffFFff);
}


inline uint8_t u1be_read(const uint8_t* buf)
{
	return buf[0];
}

inline uint16_t u2be_read(const uint8_t* buf)
{
	return ((uint16_t) u1be_read(buf) << 8) | (u1be_read(buf + 1));
}

inline uint32_t u4be_read(const uint8_t* buf)
{
	return ((uint32_t) u2be_read(buf) << 16) | (u2be_read(buf + 2));
}

inline uint64_t u8be_read(const uint8_t* buf)
{
	return ((uint64_t) u4be_read(buf) << 32) | (u4be_read(buf + 4));
}

void run_endian_test();
#endif
