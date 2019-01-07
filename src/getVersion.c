#include <os_io_seproxyhal.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "uiHelpers.h"
#include "getVersion.h"
#include "getExtendedPublicKey.h"
#include "errors.h"
#include "io.h"
#include "assert.h"

// handleGetVersion is the entry point for the getVersion command. It
// unconditionally sends the app version.
void handleGetVersion(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{
	uint8_t response[3];
	response[0] = APPVERSION[0] - '0';
	response[1] = APPVERSION[2] - '0';
	response[2] = APPVERSION[4] - '0';
	io_send_buf(SUCCESS, response, 3);
}


void handleShowAboutConfirm()
{
	ASSERT_WITH_MSG(0, "demo assert");
	// send response
	io_send_buf(SUCCESS, NULL, 0);
	ui_idle();
}

void handleShowAboutReject()
{
	io_send_buf(4747, NULL, 0);
	ui_idle();
}

void handleScrollConfirm()
{
	displayConfirm(
	        "Confirm",
	        "something",
	        handleShowAboutConfirm,
	        handleShowAboutReject
	);

}

void handleShowAbout(
        uint8_t p1,
        uint8_t p2,
        uint8_t *dataBuffer,
        uint16_t dataLength)
{

	const char* header = "Header line";
	const char* text = "This should be a long scrolling text";
	displayScrollingText(
	        header,
	        text,
	        &handleScrollConfirm
	);
}
