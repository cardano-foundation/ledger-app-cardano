#ifndef H_CARDANO_APP_BIP44
#define H_CARDANO_APP_BIP44

#include <os.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static const uint32_t MAX_PATH_LENGTH = 10;

typedef struct {
	uint32_t path[MAX_PATH_LENGTH];
	uint32_t length;
} path_spec_t;


static const uint32_t BIP_44 = 44;
static const uint32_t ADA_COIN_TYPE = 1815;

static const uint32_t HARDENED_BIP32 = ((uint32_t) 1 << 31);

size_t pathSpec_parseFromWire(
        path_spec_t* pathSpec,
        uint8_t* dataBuffer, size_t dataSize
);

// Checks for /44'/1815'/account'
bool isValidBIP44Prefix(const path_spec_t* pathSpec);

// We limit accounts
bool isAcceptableBIP44AccountValue(uint32_t value);

// Indexes into pathSpec
enum {
	BIP44_I_PURPOSE = 0,
	BIP44_I_COIN_TYPE = 1,
	BIP44_I_ACCOUNT = 2,
	BIP44_I_CHAIN = 3,
	BIP44_I_ADDRESS = 4,
};

bool isHardened(uint32_t value);
bool isValidBIP44ChainValue(uint32_t value);

#endif
