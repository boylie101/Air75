# NuPhy Air75 V2

The NuPhy Air75 V2 is a 75% wireless mechanical keyboard.

* Keyboard Maintainer: [boylie101](https://github.com/boylie101)
* Hardware Availability: [NuPhy Air75 V2](https://nuphy.com/products/air75-v2)

![NuPhy Air75 V2](https://cdn.shopify.com/s/files/1/0268/7297/1373/files/air75v2-winnmac_3840x.jpg)

## Bootloader

1. Set the keyboard's power switch to USB while the keyboard is unplugged.
2. Hold the Escape key and connect the USB-C cable to your computer.

## Flashing

To build and flash the default keymap with QMK CLI, run:

```sh
qmk flash -kb nuphy/air75_v2/ansi -km default
```

Alternatively, from the QMK Firmware directory, run:

```sh
make nuphy/air75_v2/ansi:default:flash
```
