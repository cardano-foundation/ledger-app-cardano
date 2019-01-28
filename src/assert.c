#include "common.h"
#include "assert.h"
#include "uiHelpers.h"

// Note(ppershing): In DEBUG we need to keep going
// because rendering on display takes multiple SEPROXYHAL
// exchanges until it renders the display
void assert(int cond, const char* msgStr)
{
	if (cond) return; // everything holds
	#ifdef DEVEL
	PRINTF("Assertion failed %s\n", msgStr);
	ui_displayConfirm("Assertion failed", msgStr, NULL, NULL);
	THROW(ERR_ASSERT);
	#else
	io_seproxyhal_se_reset();
	#endif
}
