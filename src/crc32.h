#ifndef H_CARDANO_APP_CRC32
#define H_CARDANO_APP_CRC32

#include <stdint.h>

uint32_t crc32(const uint8_t *buffer, uint32_t length);

void run_crc32_test();
#endif
