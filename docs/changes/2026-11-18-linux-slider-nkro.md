# Configurable NKRO for the custom Linux slider position

Commit: `cfde361cd6`.

The custom keymap uses the Mac slider position for Linux. Its former six-key
limit was a keyboard policy. The change lets this keymap select NKRO in both
slider positions while preserving its separate Linux and Windows mappings.

`custom/config.h` defines `AIR75_MAC_MODE_NKRO true`. Startup, live slider scans
and factory reset in `ansi.c` use that option. It defaults to `false` for other
keymaps; setting it to `false` restores the custom keymap's six-key compatibility
mode. USB BOOT handling and QMK's configurable NKRO flag remain available.
This commit does not change RF framing, capability selection or recovery.

Files: `ansi.c`, `keymaps/custom/config.h` and `util/air75_nkro_mode_test.py`.
Keyboard paths are under `keyboards/nuphy/air75_v2/ansi/`.

```sh
python3 util/air75_nkro_mode_test.py
```

All 15 native policy cases passed: startup, slider transitions and factory reset
across legacy defaults, custom enabled and custom explicitly disabled. Tests use
the actual function bodies with hardware stubs, warnings as errors and UBSan.
Formatting/Python lint, applicable QMK lint and all four firmware builds passed.

The user reported “tested and works” for this change. Individual transport and
factory-reset results were not enumerated, so this report does not verify every
RF/Bluetooth profile or reconnection case. Further checks should cover reboot,
both slider positions, more than six keys, Fn mappings and lighting. Back up
settings before factory-reset tests.

[Change index](../AIR75_V2_NKRO_AUDIT.md)
