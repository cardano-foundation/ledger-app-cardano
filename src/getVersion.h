#ifndef H_CARDANO_APP_GET_VERSION
#define H_CARDANO_APP_GET_VERSION

#include "handlers.h"
#include "common.h"

// Must be in format x.y.z
#ifndef APPVERSION
#error "Missing -DAPPVERSION=x.y.z in Makefile"
#endif
handler_fn_t getVersion_handleAPDU;

#endif // H_GET_VERSION
