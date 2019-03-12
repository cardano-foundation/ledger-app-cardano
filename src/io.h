#ifndef H_CARDANO_APP_IO
#define H_CARDANO_APP_IO

#include <os_io_seproxyhal.h>
#include <stdint.h>

// io_send_* arehelper functions for sending response APDUs.
// Note that the IO_RETURN_AFTER_TX flag is set so that the function
// does not receive next APDU.
// 'tx' is the conventional name for the size of the response APDU,
// i.e. the write-offset within G_io_apdu_buffer.

void _io_send_G_io_apdu_buffer(uint16_t code, uint16_t tx);

// Normal code should use just this helper function
void io_send_buf(uint16_t code, uint8_t* buffer, size_t bufferSize);

// Asserts that the response fits into response buffer
void CHECK_RESPONSE_SIZE(unsigned int tx);

// This was added for sanity checking -- our program should always be awaiting on something
// and it should be exactly the expected handler
typedef enum {
	// We are doing IO, display handlers should not fire
	IO_EXPECT_IO = 42, // Note: random constants
	// We are displaying things, IO handlers should not fire
	IO_EXPECT_UI = 47,
	// We should not be handling events
	IO_EXPECT_NONE = 49,
} io_state_t;

extern io_state_t io_state;

// Everything below this point is Ledger magic

void io_seproxyhal_display(const bagl_element_t *element);
unsigned char io_event(unsigned char channel);

typedef void timeout_callback_fn_t();
void set_timer(int ms, timeout_callback_fn_t* cb);
void clear_timer();
#endif
