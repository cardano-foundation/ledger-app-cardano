#include <os_io_seproxyhal.h>
#include "io.h"
#include "assert.h"
#include "errors.h"

io_state_t io_state;

#if defined(TARGET_NANOS)
static timeout_callback_fn_t* timeout_cb;

void nanos_clear_timer()
{
	timeout_cb = NULL;
}

void nanos_set_timer(int ms, timeout_callback_fn_t* cb)
{
	// if TRACE() is enabled, set_timer must be called
	// before ui_ methods, because it causes Ledger Nano S
	// to freeze in debug mode
	// TRACE();
	ASSERT(timeout_cb == NULL);
	ASSERT(ms >= 0);
	timeout_cb = cb;
	UX_CALLBACK_SET_INTERVAL((unsigned) ms);
}

#define HANDLE_UX_TICKER_EVENT(ux_allowed) \
	do {\
		if (timeout_cb) \
		{ \
			timeout_callback_fn_t* callback = timeout_cb; \
			timeout_cb = NULL; /* clear first if cb() throws */ \
			callback(ux_allowed); \
		} \
	} while(0)
#elif defined(TARGET_NANOX)
#define HANDLE_UX_TICKER_EVENT(ux_allowed) do {} while(0)
#endif

void CHECK_RESPONSE_SIZE(unsigned int tx)
{
	// Note(ppershing): we do both checks due to potential overflows
	ASSERT(tx < sizeof(G_io_apdu_buffer));
	ASSERT(tx + 2u < sizeof(G_io_apdu_buffer));
}

// io_exchange_with_code is a helper function for sending response APDUs from
// button handlers. Note that the IO_RETURN_AFTER_TX flag is set. 'tx' is the
// conventional name for the size of the response APDU, i.e. the write-offset
// within G_io_apdu_buffer.
void _io_send_G_io_apdu_buffer(uint16_t code, uint16_t tx)
{
	CHECK_RESPONSE_SIZE(tx);
	G_io_apdu_buffer[tx++] = code >> 8;
	G_io_apdu_buffer[tx++] = code & 0xFF;
	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

	// From now on we can receive new APDU
	io_state = IO_EXPECT_IO;
}

void io_send_buf(uint16_t code, uint8_t* buffer, size_t bufferSize)
{
	CHECK_RESPONSE_SIZE(bufferSize);

	os_memmove(G_io_apdu_buffer, buffer, bufferSize);
	_io_send_G_io_apdu_buffer(code, bufferSize);
}


// Everything below this point is Ledger magic.

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element)
{
	io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

unsigned char io_event(unsigned char channel MARK_UNUSED)
{
	// can't have more than one tag in the reply, not supported yet.
	switch (G_io_seproxyhal_spi_buffer[0]) {
	case SEPROXYHAL_TAG_FINGER_EVENT:
		// This app is not supposed to work with Blue
		ASSERT(false);
		UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
		break;

	case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
		UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
		break;

	case SEPROXYHAL_TAG_STATUS_EVENT:
		if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
		    !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
		      SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
			THROW(EXCEPTION_IO_RESET);
		}
		UX_DEFAULT_EVENT();
		break;

	case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
		UX_DISPLAYED_EVENT({});
		break;

	case SEPROXYHAL_TAG_TICKER_EVENT:
		UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
			TRACE("timer");
			HANDLE_UX_TICKER_EVENT(UX_ALLOWED);
		});
		break;

	default:
		UX_DEFAULT_EVENT();
		break;
	}

	// close the event if not done previously (by a display or whatever)
	if (!io_seproxyhal_spi_is_status_sent()) {
		io_seproxyhal_general_status();
	}

	// command has been processed, DO NOT reset the current APDU transport
	return 1;
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len)
{
	switch (channel & ~(IO_FLAGS)) {
	case CHANNEL_KEYBOARD:
		break;
	// multiplexed io exchange over a SPI channel and TLV encapsulated protocol
	case CHANNEL_SPI:
		if (tx_len) {
			io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);
			if (channel & IO_RESET_AFTER_REPLIED) {
				reset();
			}
			return 0; // nothing received from the master so far (it's a tx transaction)
		} else {
			return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
		}
	default:
		THROW(INVALID_PARAMETER);
	}
	return 0;
}

STATIC_ASSERT(CX_APILEVEL == 9 || CX_APILEVEL == 10, "bad api level");
static const unsigned PIN_VERIFIED = BOLOS_UX_OK; // Seems to work for api 9/10

bool device_is_unlocked()
{
	return os_global_pin_is_validated() == PIN_VERIFIED;
}
