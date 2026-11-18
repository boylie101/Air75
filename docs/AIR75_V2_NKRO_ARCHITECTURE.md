# Air75 V2 NKRO architecture

Reference for the [per-commit change notes](AIR75_V2_NKRO_AUDIT.md). This describes
the A–D implementation and the separately documented ACK-only follow-up.
It is not receiver-side protocol documentation.

## QMK state and transport selection

The 6×17 matrix has 84 physical keys. QMK actions resolve layers, modifiers and
HID usages before report generation. Fn, lighting and link-selection keycodes
are handled locally. [Keyboard metadata](../keyboards/nuphy/air75_v2/ansi/keyboard.json)
enables NKRO; [ansi.c](../keyboards/nuphy/air75_v2/ansi/ansi.c) applies the slider
policy at startup, transitions and factory reset. Custom enables NKRO in both
Linux/Windows positions; other keymaps retain Mac-position six-key defaults.

[report.c](../tmk_core/protocol/report.c) and
[action_util.c](../quantum/action_util.c) select six-key or NKRO state using both
`host_can_send_nkro()` and `keymap_config.nkro`. These are separate report buffers
with unchanged-report suppression. The capability hook enables the Air75 module
driver independently of USB protocol; USB BOOT still uses six-key reporting.

| QMK structure | Contents |
| --- | --- |
| `report_keyboard_t` | Modifiers, reserved byte and six usages; eight bytes, or nine with a shared report ID. |
| `report_nkro_t` | Report ID, modifiers and 30 bitmap bytes: 32 bytes total. |

The bitmap addresses usages `0x00..0xEF`; modifiers normally use the dedicated
modifier byte. [host.c](../tmk_core/protocol/host.c) dispatches report pointers
through the selected `host_driver_t`. Air75 retains the USB driver and selects
`rf_host_driver` for its module. Link switching clears through the old transport
before changing it.

## USB

```text
matrix → QMK actions → report generation → host_nkro_send()
       → USB driver → ChibiOS shared interrupt IN endpoint
```

The [descriptor](../tmk_core/protocol/usb_descriptor.c) declares NKRO report ID 6,
eight modifier bits and 240 keyboard bits. ChibiOS sends the complete 32-byte
structure. USB BOOT uses an eight-byte six-key report. LED status returns through
USB OUT/control reports. The upstream EP0 buffer fix did not change rollover.

## RF and Bluetooth

```text
QMK NKRO → rf_host_driver → uart_send_report_nkro() → uart_auto_nkro_send()
         → E1 + E2 UART frames → internal NuPhy module
         → proprietary RF/BT → dongle or Bluetooth host
```

The [RF encoder](../keyboards/nuphy/air75_v2/ansi/rf.c) receives 31 bytes starting
at QMK's modifier field, excluding its report ID. It compares the full QMK state
and converts changed usages into two proprietary reports:

| Storage / command | Format and role |
| --- | --- |
| `bitkb_report_buf[32]` | Full previous QMK modifier/bitmap comparison state; 31 bytes copied. |
| `bytekb_report_buf[8]`, E1 (`0xE1`) | Modifiers, zero reserved byte, six arbitrary usage slots. |
| `uart_bit_report_buf[16]`, E2 (`0xE2`) | Leading zero byte, 15 bitmap bytes for usages `0x00..0x77`. The leading byte does not carry modifiers. |
| UART TX/RX buffers | 64 bytes each. This allocation does not establish the module's accepted report sizes. |

A press takes a free E1 slot, otherwise its supported E2 bit. Release searches
E1 first, otherwise clears E2. Held overflow keys are not promoted when slots
become free. A usage at or above `0x78` needs an E1 slot; it is omitted if those
slots are full. The limit therefore depends on usages and press history, not a
single fixed simultaneous-key count. Dynamic mappings can expose this limit.

Modifier changes update E1 only. Immediate mixed updates send E2 before E1;
periodic refresh sends E1 first. Their union is not updated atomically.

```text
5A | command | 01 | payload length | payload | ((sum(payload) & FF) XOR 5A)
```

E1 frames are 13 bytes and E2 frames 21 bytes, including five framing bytes.
Reports retain triple transmission with wake-pin pulses and existing delays.
UART is 460800 baud, with parity configured in the keyboard. Immediate report
transmission requires an initialised dial, wireless mode and RF_CONNECT; local
state is retained when a send is suppressed. Recovery refreshes held state and
replays current snapshots, while respecting sleep/reset-status gates.

BT1–3 share the RF driver, encoder and UART commands; selecting a profile changes
the module's link mode. Built-in QMK Bluetooth is not enabled for this keyboard.
There is no independent module Bluetooth rollover-capability query. E3/E4 carry
two-byte consumer/system usages. E0 carries five mouse bytes starting at buttons;
extended QMK mouse formats would require an explicit adaptation.

## Receiver and validation limits

This repository contains STM32 keyboard firmware, not radio-module or dongle
source/descriptors. UART frames do not prove the on-air format, receiver report
union, duplicate suppression, ordering or delivery guarantees. Seven-key RF
rollover and specific receiver-removal cases have user-supplied Linux evidence;
held-E2 recovery and the full hardware matrix remain unverified.

The [ACK fix](changes/2026-11-18-rf-ack-only-dispatch.md) prevents status/
configuration receipt ACKs from consuming stale payloads or completing data
requests. It preserves payload-free notifications and the wire format. Command
waits, stream framing and other review suggestions remain separate work.
