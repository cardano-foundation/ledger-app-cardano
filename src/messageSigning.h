#include "bip44.h"

void getTxWitness(bip44_path_t* pathSpec,
                  const uint8_t* txHashBuffer, size_t txHashSize,
                  uint8_t* outBuffer, size_t outSize);
