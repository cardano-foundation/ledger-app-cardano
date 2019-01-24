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

	VALIDATE(p1 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(wireDataSize == 0, ERR_INVALID_REQUEST_PARAMETERS);

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
