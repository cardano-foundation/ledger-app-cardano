#ifndef H_CARDANO_APP_TEXT_UTILS
#define H_CARDANO_APP_TEXT_UTILS

#include "common.h"

size_t str_formatAdaAmount(char* out, size_t outSize, uint64_t amount);

void run_textUtils_test();

#endif
