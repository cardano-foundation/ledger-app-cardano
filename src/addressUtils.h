#ifndef H_CARDANO_APP_ADDRESS_UTILS
#define H_CARDANO_APP_ADDRESS_UTILS

#include "common.h"
#include "bip44.h"

size_t deriveAddress(
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

size_t deriveRawAddress(
        const bip44_path_t* pathSpec,
        uint8_t* outBuffer, size_t outSize
);

// Note: validates boxing
// TODO(ppershing): should we just return a pointer view?
size_t unboxChecksummedAddress(
        const uint8_t* addressBuffer, size_t addressSize,
        uint8_t* outputBuffer, size_t outputSize
);

void run_address_utils_test();

#endif
