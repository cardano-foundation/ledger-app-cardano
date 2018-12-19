#ifndef H_CARDANO_APP_IO
#define H_CARDANO_APP_IO

#include <stdint.h>

// io_exchange_with_code is a helper function for sending response APDUs from
// button handlers. Note that the IO_RETURN_AFTER_TX flag is set. 'tx' is the
// conventional name for the size of the response APDU, i.e. the write-offset
// within G_io_apdu_buffer.
void io_send_code(uint16_t code, uint16_t tx);

void io_send_buf(uint16_t code, uint8_t* buffer, uint16_t len);

#endif
