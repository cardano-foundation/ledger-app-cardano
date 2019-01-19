#include "common.h"
#include "crc32.h"

// Code taken from: https://www.hackersdelight.org/hdcodetxt/crc.c.txt option crc32b

uint32_t crc32(const uint8_t* inBuffer, size_t inSize)
{
	ASSERT(inSize < BUFFER_SIZE_PARANOIA);

	uint32_t byte, crc, mask;

	crc = 0xFFFFFFFF;
	for(size_t i = 0; i < inSize; i++) {
		byte = inBuffer[i];
		crc = crc ^ byte;

		for (uint32_t j = 0; j < 8; j++) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}

	return ~crc;
}
