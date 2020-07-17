#ifndef H_CARDANO_APP_ADDRESS_UTILS_BYRON
#define H_CARDANO_APP_ADDRESS_UTILS_BYRON

#include "common.h"
#include "bip44.h"

size_t deriveAddress_byron(
        const bip44_path_t* pathSpec,
        uint32_t protocolMagic,
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
        uint32_t protocolMagic,
        uint8_t* outBuffer, size_t outSize
);

// Note: validates the overall address structure at the same time
uint32_t extractProtocolMagic(
        const uint8_t* addressBuffer, size_t addressSize
);

void run_addressUtilsByron_test();

#endif
