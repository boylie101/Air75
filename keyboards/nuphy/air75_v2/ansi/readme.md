# NuPhy Air75 V2

The NuPhy Air75 V2 is a 75% wireless mechanical keyboard.

NKRO support depends on the keymap and transport. Windows selects NKRO. The
`custom` keymap also selects NKRO in the Mac slider position, which it uses for
Linux. Other keymaps retain six-key rollover in the Mac position. To restore
that compatibility mode for `custom`, set `AIR75_MAC_MODE_NKRO` to `false` in its
`config.h` and rebuild. Startup, slider changes and factory reset use this policy;
normal QMK six-key reports and USB boot-protocol handling remain available.
Wireless uses a NuPhy UART protocol with six usage slots plus a limited overflow
bitmap. Its NKRO capability is independent of wired USB BOOT/REPORT protocol.
The firmware refreshes held state and sends bounded current-state snapshots on
recovery, retaining the original packet format. Higher usages outside the bitmap
still need a free usage slot. See the [NKRO audit and hardware test
plan](../../../../docs/AIR75_V2_NKRO_AUDIT.md) for limitations and verification.

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
