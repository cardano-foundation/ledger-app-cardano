#include "common.h"
#include "handlers.h"

#include "uiHelpers.h"
#include "getVersion.h"
#include "getExtendedPublicKey.h"


enum {
	FLAG_DEVEL = 1,

};

void getVersion_handleAPDU(
        uint8_t p1,
        uint8_t p2,
        uint8_t *wireDataBuffer MARK_UNUSED,
        size_t wireDataSize,
        bool isNewCall MARK_UNUSED
)
{
	// Check that we have format "x.y.z"
	STATIC_ASSERT(SIZEOF(APPVERSION) == 5 + 1, "bad APPVERSION length");
#define ASSERT_IS_DIGIT(d) STATIC_ASSERT(APPVERSION[d] >= '0' && APPVERSION[d] <= '9', "bad digit in APPVERSION")
	ASSERT_IS_DIGIT(0);
	ASSERT_IS_DIGIT(2);
	ASSERT_IS_DIGIT(4);

	VALIDATE(p1 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(p2 == 0, ERR_INVALID_REQUEST_PARAMETERS);
	VALIDATE(wireDataSize == 0, ERR_INVALID_REQUEST_PARAMETERS);

	struct {
		uint8_t major;
		uint8_t minor;
		uint8_t patch;
		uint8_t flags;
	} response = {
		.major = APPVERSION[0] - '0',
		.minor = APPVERSION[2] - '0',
		.patch = APPVERSION[4] - '0',
		.flags = 0,
	};

	#ifdef DEVEL
	response.flags |= FLAG_DEVEL;
	#endif

	io_send_buf(SUCCESS, (uint8_t *) &response, sizeof(response));
	ui_idle();
}
