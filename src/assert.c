#include "common.h"
#include "assert.h"
#include "uiHelpers.h"

// Note(ppershing): Right now suitable for debugging only.
// We need to keep going because rendering on display
// takes multiple SEPROXYHAL exchanges until it renders
// the display
void assert(int cond, const char* msgStr)
{
	if (cond) return; // everything holds

	ui_displayConfirm("Assertion failed", msgStr, NULL, NULL);
	THROW(ERR_ASSERT);
}
