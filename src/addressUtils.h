#ifndef H_CARDANO_APP_ADDRESS_UTILS
#define H_CARDANO_APP_ADDRESS_UTILS

#include "common.h"
#include "bip44.h"

uint32_t deriveAddress(
        const bip44_path_t* pathSpec,
        uint8_t* outBuffer, size_t outSize
);

size_t cborEncodePubkeyAddress(
        const uint8_t* addressRoot, size_t addressRootSize,
        uint8_t* outBuffer, size_t outSize
        /* potential attributes */
);

size_t cborPackRawAddressWithChecksum(
        const uint8_t* rawAddressBuffer, size_t rawAddressSize,
        uint8_t* outputBuffer, size_t outputSize
);

uint32_t deriveAddress(
        const bip44_path_t* pathSpec,
        uint8_t* outBuffer, size_t outSize
);

void run_address_utils_test();

#endif
