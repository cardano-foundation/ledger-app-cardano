#*******************************************************************************
#   Ledger Nano S
#   (c) 2016 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
	$(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

SIGNKEY = `cat sign.key`
APPSIG = `cat bin/app.sig`
NANOS_ID = 1
WORDS = "void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold"
PIN = 6666

APPNAME = "Cardano ADA"
APPVERSION = "1.1.0"

APP_LOAD_PARAMS =--appFlags 0x240 --curve ed25519 --path "44'/1815'"
APP_LOAD_PARAMS += $(COMMON_LOAD_PARAMS)

ifeq ($(TARGET_NAME),TARGET_NANOX)
	ICONNAME=icon_ada_nanox.gif
else
	ICONNAME=icon_ada_nanos.gif
endif

################
# Default rule #
################
all: default

############
# Platform #
############
DEFINES += OS_IO_SEPROXYHAL
DEFINES += HAVE_BAGL HAVE_SPRINTF
DEFINES += APPVERSION=\"$(APPVERSION)\"

## USB HID?
DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU

## USB U2F
DEFINES += HAVE_U2F HAVE_IO_U2F U2F_PROXY_MAGIC=\"ADA\" USB_SEGMENT_SIZE=64 

## WEBUSB
WEBUSB_URL = https://www.ledger.com/pages/supported-crypto-assets
DEFINES += HAVE_WEBUSB WEBUSB_URL_SIZE_B=$(shell echo -n $(WEBUSB_URL) | wc -c) WEBUSB_URL=$(shell echo -n $(WEBUSB_URL) | sed -e "s/./\\\'\0\\\',/g")

## BLUETOOTH
ifeq ($(TARGET_NAME),TARGET_NANOX)
	DEFINES += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000 HAVE_BLE_APDU
endif

## Protect stack overflows
DEFINES += HAVE_BOLOS_APP_STACK_CANARY

ifeq ($(TARGET_NAME),TARGET_NANOX)
	DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=300
	DEFINES += HAVE_GLO096
	DEFINES += HAVE_BAGL BAGL_WIDTH=128 BAGL_HEIGHT=64
	DEFINES += HAVE_BAGL_ELLIPSIS # long label truncation feature
	DEFINES += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
	DEFINES += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
	DEFINES += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
	DEFINES += HAVE_UX_FLOW
else
	DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=128
endif

DEFINES += RESET_ON_CRASH

## Use developer build
#DEVEL = 1
#DEFINES += HEADLESS

# Enabling debug PRINTF
ifeq ($(DEVEL), 1)
	DEFINES += DEVEL HAVE_PRINTF
	ifeq ($(TARGET_NAME),TARGET_NANOX)
		DEFINES += PRINTF=mcu_usb_printf
	else
		DEFINES += PRINTF=screen_printf
	endif
else
    DEFINES += PRINTF\(...\)=
endif

##############
#  Compiler  #
##############
ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
	CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
	GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
	$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
	$(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
	$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC       := $(CLANGPATH)clang
CFLAGS   += -O3 -Os -Wall -Wextra -Wuninitialized

AS     := $(GCCPATH)arm-none-eabi-gcc
LD       := $(GCCPATH)arm-none-eabi-gcc

LDFLAGS  += -O3 -Os -Wall
LDLIBS   += -lm -lgcc -lc

##Enable to strip debug info from app
#LDFLAGS  += -Wl,-s


##################
#  Dependencies  #
##################

# import rules to compile glyphs
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl lib_u2f
ifeq ($(TARGET_NAME),TARGET_NANOX)
	SDK_SOURCE_PATH  += lib_blewbxx lib_blewbxx_impl
	SDK_SOURCE_PATH  += lib_ux
endif
##############
#   Build    #
##############
load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

seed:
	python -m ledgerblue.hostOnboard --id $(NANOS_ID) --words $(WORDS) --pin $(PIN)


# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile

listvariants:
	@echo VARIANTS COIN cardano_ada