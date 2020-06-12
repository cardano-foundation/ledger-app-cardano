#ifndef H_CARDANO_APP_BIP44
#define H_CARDANO_APP_BIP44

#include "common.h"

static const uint32_t BIP44_MAX_PATH_LENGTH = 10;

typedef struct {
	uint32_t path[BIP44_MAX_PATH_LENGTH];
	uint32_t length;
} bip44_path_t;


static const uint32_t PURPOSE_BYRON = 44;
static const uint32_t PURPOSE_SHELLEY = 1852;
static const uint32_t ADA_COIN_TYPE = 1815;

static const uint32_t HARDENED_BIP32 = ((uint32_t) 1 << 31);

size_t bip44_parseFromWire(
        bip44_path_t* pathSpec,
        const uint8_t* dataBuffer, size_t dataSize
);

// Indexes into pathSpec
enum {
	BIP44_I_PURPOSE = 0,
	BIP44_I_COIN_TYPE = 1,
	BIP44_I_ACCOUNT = 2,
	BIP44_I_CHAIN = 3,
	BIP44_I_ADDRESS = 4,
	BIP44_I_REST = 5,
};


bool bip44_hasByronPrefix(const bip44_path_t* pathSpec);
bool bip44_hasShelleyPrefix(const bip44_path_t* pathSpec);
bool bip44_hasValidCardanoPrefix(const bip44_path_t* pathSpec);

bool bip44_containsAccount(const bip44_path_t* pathSpec);
bool bip44_hasReasonableAccount(const bip44_path_t* pathSpec);

bool bip44_containsChainType(const bip44_path_t* pathSpec);
bool bip44_hasValidChainTypeForAddress(const bip44_path_t* pathSpec);

bool bip44_containsAddress(const bip44_path_t* pathSpec);
bool bip44_hasReasonableAddress(const bip44_path_t* pathSpec);

bool bip44_isValidStakingKeyPath(const bip44_path_t* pathSpec);
void bip44_stakingKeyPathFromAddresPath(const bip44_path_t* addressPath, bip44_path_t* stakingKeyPath);

bool bip44_containsMoreThanAddress(const bip44_path_t* pathSpec);

bool isHardened(uint32_t value);

void bip44_printToStr(const bip44_path_t*, char* out, size_t outSize);


#ifdef DEVEL
void bip44_PRINTF(const bip44_path_t* pathSpec);
#endif


#endif // H_CARDANO_APP_BIP44
