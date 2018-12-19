#ifndef H_CARDANO_APP_HANDLERS
#define H_CARDANO_APP_HANDLERS

#include <stdint.h>
// This is the function signature for a command handler. 'flags' and 'tx' are
// out-parameters that will control the behavior of the next io_exchange call
// in sia_main. It's common to set *flags |= IO_ASYNC_REPLY, but tx is
// typically unused unless the handler is immediately sending a response APDU.
typedef void handler_fn_t(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength,
        // out
        volatile unsigned int *flags,
        volatile unsigned int *tx
);

handler_fn_t* lookupHandler(uint8_t ins);

#endif
