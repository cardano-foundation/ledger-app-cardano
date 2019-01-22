#include "common.h"
#include "handlers.h"

#include "uiHelpers.h"
#include "getVersion.h"
#include "getExtendedPublicKey.h"

// handleGetVersion is the entry point for the getVersion command. It
// unconditionally sends the app version.
void handleGetVersion(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireDataBuffer MARK_UNUSED,
        size_t wireDataSize,
        bool isNewCall MARK_UNUSED
)
{
	STATIC_ASSERT(SIZEOF(APPVERSION) == 5 + 1, "bad APPVERSION length");
	STATIC_ASSERT(APPVERSION[0] >= '0' && APPVERSION[0] <= '9', "bad APPVERSION major");
	STATIC_ASSERT(APPVERSION[2] >= '0' && APPVERSION[2] <= '9', "bad APPVERSION minor");
	STATIC_ASSERT(APPVERSION[4] >= '0' && APPVERSION[4] <= '9', "bad APPVERSION patch");

	VALIDATE_REQUEST_PARAM(p1 == 0);
	VALIDATE_REQUEST_PARAM(p2 == 0);
	VALIDATE_REQUEST_PARAM(wireDataSize == 0);

	struct {
		uint8_t major;
		uint8_t minor;
		uint8_t patch;
	} response;
	response.major = APPVERSION[0] - '0';
	response.minor = APPVERSION[2] - '0';
	response.patch = APPVERSION[4] - '0';
	io_send_buf(SUCCESS, (uint8_t *) &response, SIZEOF(response));
	ui_idle();
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
	ui_displayConfirm(
	        "Confirm",
	        "something",
	        handleShowAboutConfirm,
	        handleShowAboutReject
	);

}

void handleShowAbout(
        uint8_t p1 MARK_UNUSED,
        uint8_t p2 MARK_UNUSED,
        uint8_t *wireDataBuffer MARK_UNUSED,
        size_t wireDataSize MARK_UNUSED,
        bool isNewCall MARK_UNUSED
)
{

	const char* header = "Header line";
	const char* text = "This should be a long scrolling text";
	ui_displayScrollingText(
	        header,
	        text,
	        &handleScrollConfirm
	);
}
