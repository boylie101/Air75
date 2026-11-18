# Air75 V2 firmware change index

Implementation, validation and remaining limits are recorded separately for each
change commit. New work gets its own note; this page stays a short index.

| Commit | Change documentation |
| --- | --- |
| `9c8d758b1c` | [Initial genuine QMK merge, including the USB control-buffer fix](changes/2026-11-18-qmk-merge-8252b63097.md). |
| `4aebe6682f` | [QMK merge through eea6258af5](changes/2026-11-18-qmk-merge-eea6258af5.md). |
| `cfde361cd6` | [Configurable Linux slider NKRO](changes/2026-11-18-linux-slider-nkro.md). |
| `70efa32c66` | [A — bounded RF encoder](changes/2026-11-18-rf-encoder-bounds.md). |
| `6b2b0c6140` | [B — transport-aware NKRO capability](changes/2026-11-18-transport-nkro-capability.md). |
| `bb7c2efb7b` | [C — state-clearing regressions](changes/2026-11-18-state-clearing-regressions.md). |
| `6fc5e6db60` | [D — RF recovery, validation and hardware results](changes/2026-11-18-rf-recovery.md). |
| `d523003c9f` | [ACK-only dispatch hardening](changes/2026-11-18-rf-ack-only-dispatch.md). |
| With this index | [Per-commit documentation organisation](changes/2026-11-18-per-commit-documentation.md). |
| `21f8d8210b` | [EEPROM settings validation](changes/2026-11-18-eeprom-settings-validation.md). |

[Architecture reference](AIR75_V2_NKRO_ARCHITECTURE.md) explains the USB, RF and
Bluetooth paths and preserved E1/E2 format. The radio/dongle implementation is
unavailable; sender tests do not establish its delivery guarantees.

User-provided Linux captures verify seven-key RF rollover, recovery of a held
E1 key and no stale Space replay after its offline release. Held-E2 recovery and
other outstanding cases are listed in the [D note](changes/2026-11-18-rf-recovery.md).
Those captures precede the ACK fix. The user subsequently reports flashing and
fully testing its candidate successfully; individual post-fix captures were not supplied.

The original audit was committed as `f0d1e29839`. Its content plus the subsequent
ACK follow-up is preserved in the [frozen historical snapshot](archive/AIR75_V2_NKRO_AUDIT_2026-11-18.md).
Historical claims and commands there describe earlier states. Existing commits
have not been split, amended or rewritten; these notes improve documentation
organisation going forward.
