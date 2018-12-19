#include <os_io_seproxyhal.h>
#include <os.h>
#include <string.h>

#include "assert.h"
#include "io.h"

// For debugging.
// TODO(ppershing): PROD build
// should mask this under a generic error

void checkOrFail(int cond, const char* msg)
{
	if (cond) return; // everything holds

	STATIC_ASSERT(sizeof(G_io_apdu_buffer) > 2, bad_apdu_buffer_size);
	size_t len = MIN(
	                     sizeof(G_io_apdu_buffer) - 2,
	                     strlen(msg)
	             );

	io_send_buf(SW_ASSERT, msg, len);
	while (1) {
		// halt
	}
}
