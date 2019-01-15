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
APP_LOAD_PARAMS =--appFlags 0 --curve ed25519 --path "44'/1815'"
APP_LOAD_PARAMS += $(COMMON_LOAD_PARAMS)
APPVERSION_M=0
APPVERSION_N=1
APPVERSION_P=2
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

ICONNAME=icon.gif
################
# Default rule #
################
all: default

############
# Platform #
############
DEFINES += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=128
DEFINES += HAVE_BAGL HAVE_SPRINTF HAVE_PRINTF PRINTF=screen_printf
DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES += VERSION=\"$(APPVERSION)\"
DEFINES += LEDGER_MAJOR_VERSION=$(APPVERSION_M) LEDGER_MINOR_VERSION=$(APPVERSION_N) LEDGER_PATCH_VERSION=$(APPVERSION_P)
DEFINES += HAVE_BOLOS_APP_STACK_CANARY

##############
#  Compiler  #
##############
#GCCPATH   := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
#CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
CC       := $(CLANGPATH)clang

#CFLAGS   += -O0
CFLAGS   += -O3 -Os

AS     := $(GCCPATH)arm-none-eabi-gcc
LD       := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS  += -O3 -Os
LDLIBS   += -lm -lgcc -lc

##################
#  Dependencies  #
##################

# import rules to compile glyphs
include $(BOLOS_SDK)/Makefile.glyphs

### computed variables
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl

##############
#   Build    #
##############
build: all
build: DEFINES += INS_SIGN_TX_FUNC
build: DEFINES += INS_SET_TX_FUNC
build: DEFINES += INS_GET_PUBLIC_KEY_FUNC
build: DEFINES += INS_APP_INFO_FUNC
build: DEFINES += DEVEL

sign:
	python -m ledgerblue.signApp --hex bin/app.hex --key $(SIGNKEY) > bin/app.sig

deploy:
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS) --signature $(APPSIG)

load: build
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

#	python -m ledgerblue.signApp --hex bin/app.hex --key $(SIGNKEY) > bin/app.sig
#	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS) --signature $(APPSIG)


delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS) --rootPrivateKey $(SIGNKEY)

seed:
	python -m ledgerblue.hostOnboard --id $(NANOS_ID) --words $(WORDS) --pin $(PIN)


# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile
