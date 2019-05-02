#ifndef H_CARDANO_APP_MENU
#define H_CARDANO_APP_MENU

#include <os_io_seproxyhal.h>

#if defined(TARGET_NANOS)
extern const ux_menu_entry_t menu_main[4];
#elif defined(TARGET_NANOX)
extern const ux_flow_step_t* const ux_idle_flow [];
#endif

#endif
