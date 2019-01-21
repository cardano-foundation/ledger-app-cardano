#ifndef H_CARDANO_APP_HANDLERS
#define H_CARDANO_APP_HANDLERS

#include "common.h"

typedef void handler_fn_t(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireBuffer,
        size_t wireSize,
        bool isNewCall
);

handler_fn_t* lookupHandler(uint8_t ins);

#endif
