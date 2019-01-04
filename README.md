# Cardano Ledger App

Cardano Ledger App for Ledger Nano S


## Building

### Loading the app

`make load`

Builds and loads the application into connected device. Just make sure to close the Ledger app on the device before running the command.

### Production

`make clean load`

Build the app with the PROD API only.

### Test API 

`make clean test`

Builds the app with the TEST API only. Useful for testing underlying implementations used in the production functions.

### Headless API - Non production

`make clean headless`

Build the PROD API bypassing the UI. [**WARNING!** Bypasses user confirmation screens. Not to be used for production builds].

### Deploying Release

Releases will need to be given to Ledger for signing with their private key.
This will allow the application to be installed via the Ledger App Manager without warning prompts on any Ledger device.

## Setup

Make sure your:
- SDK >= 1.5.2
- MCU >= 1.6

Environment setup and developer documentation is sufficiently provided in Ledger’s [Read the Docs](http://ledger.readthedocs.io/en/latest/). 

### Setting udev rules

In some Linux distros (e.g. Ubuntu), you might need to setup udev rules before your device can communicate with the system.

On Ubuntu, create a file under `/etc/udev/rules.d` called `01-ledger.rules` and paste this content inside: 

```
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0000", MODE="0660", TAG+="uaccess", TAG+="udev-acl" OWNER="__user__"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0001", MODE="0660", TAG+="uaccess", TAG+="udev-acl" OWNER="__user__"
```

replacing `__user__` with your system's user name.

Run `udevadm control --reload` in system's shell to load the changes.

## Development

To learn more about development process and individual commands, [check the desing doc](doc/design_doc.md).

## Deploying

The build process is managed with [Make](https://www.gnu.org/software/make/).

### Make Commands

* `clean`: Clean the build and output directories
* `delete`: Remove the application from the device
* `load`: Load signed app onto the Ledger device with regular api
* `test`: Load signed app onto the Ledger device with test api
* `build`: Build obj and bin api artefacts without loading
* `build-test`: Build obj and bin test api artefacts without loading
* `sign`: Sign current app.hex with CA private key
* `deploy`: Load the current app.hex onto the Ledger device
* `seed` : Seeds identity 1 on the device to a predetermined mnemonic. Intended for automated testing purposes. [**WARNING!** This will overwrite the identity on the device]

See `Makefile` for list of included functions.

## Troubleshooting

### make delete/load/test/headless

* `ledgerblue.commException.CommException: Exception : No dongle found
make: *** [load] Error 1` : Device not unlocked; Device not connected; VM does not have control of USB port

### make load/test/headless

* `ledgerblue.commException.CommException: Exception : Invalid status 6a80`: Cardano app already installed on device - uninstall it first.

### make delete
`Invalid status 6e00`: App is running
