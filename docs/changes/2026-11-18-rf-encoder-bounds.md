# A — Bound the RF bitmap to its transmitted payload

Commit: `70efa32c66`.

The old RF bitmap cache held 32 bytes but transmitted only 16. Changes outside
the transmitted slice could set refresh activity and produce E2 reports unable
to communicate those changes. The old allocation itself was not out of bounds.

`rf.c` now uses a static 16-byte `uart_bit_report_buf`. Press and release paths
both check its actual size before accessing it; immediate and periodic E2 sends
use `sizeof` the buffer. The full 32-byte QMK comparison buffer remains intact,
as do the six E1 usage slots.

The [protocol](../AIR75_V2_NKRO_ARCHITECTURE.md) is unchanged: E2 byte zero stays
zero and the remaining 15 bytes cover usages `0x00..0x77`. Higher usages still
need a free E1 slot. This does not expand rollover or promote held overflow keys.
Supported output remains byte-for-byte equal to the old encoder. Unsupported
overflow changes no longer generate redundant E2 reports or activate its refresh
flag; this intentionally changes transmission count/activity for invisible keys.

Tests were added in `tests/air75_rf/native.inc` and `util/air75_rf_audit.py`:

```sh
python3 util/air75_rf_audit.py --stage encoder --analyze
```

All 22 checks passed: 12 current encoder cases and ten byte-for-byte comparisons
against the encoder at `9c8d758b1c`. Coverage includes `0x77/0x78`, all six E1
slots, high usages, modifiers, presses/releases, mixed changes, bounds and E1/E2
payloads. Clang static analysis, ASan/UBSan, changed-source formatting, applicable
lint and all four firmware builds passed. Explicit VIA lint has a separate,
existing upstream keymap-name restriction.

Subsequent user testing demonstrated seven simultaneous RF keys and no stale
Space replay after an offline release; see [recovery results](2026-11-18-rf-recovery.md).
Receiver limits and higher-usage handling remain constrained by the existing
protocol. Compilation and native frames do not prove end-to-end RF delivery.

[Change index](../AIR75_V2_NKRO_AUDIT.md)
