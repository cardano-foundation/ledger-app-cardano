#include <os_io_seproxyhal.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "uiHelpers.h"
#include "getVersion.h"
#include "getPubKey.h"
#include "errors.h"
#include "io.h"

// handleGetVersion is the entry point for the getVersion command. It
// unconditionally sends the app version.
void handleGetVersion(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength,
        volatile unsigned int *flags,
        volatile unsigned int *tx)
{
	uint8_t response[3];
	response[0] = APPVERSION[0] - '0';
	response[1] = APPVERSION[2] - '0';
	response[2] = APPVERSION[4] - '0';
	io_send_buf(SW_OK, response, 3);
}


void handleShowAboutConfirm()
{
	// send response
	io_send_buf(SW_OK, NULL, 0);
	ui_idle();
}

void handleShowAbout(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength,
        volatile unsigned int *flags,
        volatile unsigned int *tx)
{

	const char* header = "Header line";
	const char* text = "This should be a long scrolling text";
	displayScrollingText(
	        header, strlen(header),
	        text, strlen(text),
	        &handleShowAboutConfirm
	);

	// Set the IO_ASYNC_REPLY flag. This flag tells sia_main that we aren't
	// sending data to the computer immediately; we need to wait for a button
	// press first.
	*flags |= IO_ASYNCH_REPLY;
}
