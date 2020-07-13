#ifndef H_CARDANO_APP_ADDRESS_UTILS_SHELLEY
#define H_CARDANO_APP_ADDRESS_UTILS_SHELLEY

#include "common.h"
#include "bip44.h"

#define PUBLIC_KEY_HASH_LENGTH 28

// supported address types
typedef enum {
	BASE       = 0b0000,  // 0x0
	POINTER    = 0b0100,  // 0x4
	ENTERPRISE = 0b0110,  // 0x6
	BYRON      = 0b1000,  // 0x8
	REWARD     = 0b1110,  // 0xE
} address_type_t;

address_type_t getAddressType(uint8_t addressHeader);
uint8_t getNetworkId(uint8_t addressHeader);
bool isSupportedAddressType(uint8_t addressHeader);


// describes which staking info should be incorporated into address
// (see stakingChoice in shelleyAddressParams_t)
enum {
	NO_STAKING = 0x11,
	STAKING_KEY_PATH = 0x22,
	STAKING_KEY_HASH = 0x33,
	BLOCKCHAIN_POINTER = 0x44
};

bool isValidStakingChoice(uint8_t stakingChoice);


typedef uint32_t blockchainIndex_t; // must be unsigned

typedef struct {
	blockchainIndex_t blockIndex;
	blockchainIndex_t txIndex;
	blockchainIndex_t certificateIndex;
} blockchainPointer_t;

typedef struct {
	uint8_t header;
	bip44_path_t spendingKeyPath;
	uint8_t stakingChoice;
	union {
		bip44_path_t stakingKeyPath;
		uint8_t stakingKeyHash[PUBLIC_KEY_HASH_LENGTH];
		blockchainPointer_t stakingKeyBlockchainPointer;
	};
} shelleyAddressParams_t;

bool isStakingInfoConsistentWithHeader(const shelleyAddressParams_t* addressParams);


size_t deriveAddress_shelley(const shelleyAddressParams_t* addressParams, uint8_t* outBuffer, size_t outSize);

void printBlockchainPointerToStr(blockchainPointer_t blockchainPointer, char* out, size_t outSize);

void run_addressUtilsShelley_test();

#endif
