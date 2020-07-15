#ifndef H_CARDANO_APP_TEXT_UTILS
#define H_CARDANO_APP_TEXT_UTILS

#include "common.h"

size_t str_formatAdaAmount(uint64_t amount, char* out, size_t outSize);

size_t str_formatTtl(uint64_t ttl, char* out, size_t outSize);

size_t str_formatMetadata(const uint8_t* metadataHash, size_t metadataHashSize, char* out, size_t outSize);

void run_textUtils_test();

#endif
