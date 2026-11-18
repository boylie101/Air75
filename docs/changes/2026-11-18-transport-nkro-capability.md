# B — Select wireless NKRO capability independently of USB protocol

Commit: `6b2b0c6140`.

QMK's USB protocol-derived capability could select six-key reports for the
Air75 RF driver when a wired host chose BOOT protocol. That also allowed key
history to split between QMK's separate report buffers during wireless use.

A narrow weak hook, `host_can_send_nkro_for_driver(driver, default_capability)`,
was added in `tmk_core/protocol/host.c` and declared in `host.h`. Its default
returns the original QMK decision. The Air75 override in `rf_driver.c` returns
true for its selected RF/Bluetooth module driver. No host-driver structure or
ABI changes were introduced, and USB protocol state is never forced.

| Selected transport | Capability | Selected report mode |
| --- | --- | --- |
| USB BOOT | False. | Six-key report. |
| USB REPORT | Original QMK default. | Controlled by `keymap_config.nkro`. |
| Air75 RF/Bluetooth module | True, independently of USB protocol. | Controlled by the same NKRO flag; six-key mode remains available. |
| QMK built-in Bluetooth / NONE | Existing early decisions before the hook. | Existing upstream behaviour. |

This hook is a local QMK extension, not an existing official upstream API.
Future upstream merges should review whether QMK supplies an equivalent.
The [proprietary RF format](../AIR75_V2_NKRO_ARCHITECTURE.md) is unchanged; a
capability result does not expand the radio's bitmap or establish receiver limits.

```sh
python3 util/air75_rf_audit.py --stage transport --analyze
```

All 21 transport cases passed: USB/RF × BOOT/REPORT × six-key/NKRO, the weak
default without a board override, switching and built-in Bluetooth/NONE paths.
Clang static analysis and sanitizers, applicable formatting/lint, all four
firmware builds and 81 QMK tests passed, with five existing QMK skips.

Physical testing still needs an independent wired host changing USB protocol
while RF is active, plus USB BOOT compatibility and configurable six-key mode.
The associated conditional clearing regression is documented in
[C](2026-11-18-state-clearing-regressions.md).

[Change index](../AIR75_V2_NKRO_AUDIT.md)
