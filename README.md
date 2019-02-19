# Cardano Ledger App

Cardano Ledger App for Ledger Nano S


## Building

### Loading the app

`make load`

Builds and loads the application into connected device. Just make sure to close the Ledger app on the device before running the command.


### Debug version

Uncomment `#DEFINE+=DEVEL` and `#DEFINE+=HEADLESS` in Makefile. Then `make clean load`

### Setup

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
* `load`: Load signed app onto the Ledger device
* `build`: Build obj and bin api artefacts without loading
* `sign`: Sign current app.hex with CA private key
* `deploy`: Load the current app.hex onto the Ledger device

See `Makefile` for list of included functions.
