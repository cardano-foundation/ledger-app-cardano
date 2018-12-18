#include <stdint.h>
#include <stdbool.h>
#include <os_io_seproxyhal.h>
#include "sia.h"
//#include "ux.h"

#include "getVersion.h"
#include "getPubKey.h"

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
	io_exchange_with_response(SW_OK, response, 3);
}
