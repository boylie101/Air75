# Validate persisted keyboard settings before loading them

Status: implemented on `fix/air75-eeprom-validation`; user reports testing passed.

An EEPROM block with a valid `0xAD` marker could contain out-of-range values.
The loader copied them into runtime state, allowing side-light lookup tables
to be read out of bounds. This change repairs individual invalid fields before
applying the settings and persists all repairs in one datablock update.

| Field | Valid values | Replacement for an invalid value |
| --- | --- | --- |
| Side mode | 0–4. | Static mode (2). |
| Side brightness | 0–5. | 4. |
| Side speed | 0–4. | 2. |
| Rainbow flag | 0 or 1. | 1. |
| Side colour | 0–3. | 0. |
| Sleep enable | 0 or 1. | 1. |

`side_config_sanitize()` in `side.c` derives lighting bounds from the actual
tables. `loading_eeprom_data()` in `ansi.c` validates the sleep flag and writes
only if repairs occurred. Defaults match the existing factory-reset policy.
Brightness 5 remains valid: the existing increment control can save that value.

Valid fields, the reserved byte, main RGB settings, the eight-byte EEPROM
layout, marker and invalid-marker first-boot behaviour are preserved. A valid
block causes no write; a repaired block needs no further write on the next boot.
This does not change the separate sleep state machine or detect corruption that
happens to produce another valid value. Validation occurs at load time.

## Validation

```sh
python3 util/air75_eeprom_test.py --analyze
python3 util/air75_rf_audit.py --stage all --analyze
python3 util/air75_nkro_mode_test.py
qmk test-c -t basic -t mousekeys -t housekeeping -j 2
qmk lint --strict -kb nuphy/air75_v2/ansi
qmk lint --strict -kb nuphy/air75_v2/ansi -km custom
qmk lint --strict -kb nuphy/air75_v2/ansi -km default_nuphy
for keymap in custom default default_nuphy via; do
    qmk compile -kb nuphy/air75_v2/ansi -km "$keymap" || break
done
```

The native tests use actual loader, controls, reset defaults and side renderers;
EEPROM, LED transport, main RGB setters, time and unrelated indicators are mocked.
All 2,400 valid combinations give identical settings/render output to the loader
at `48ca65f4dc`. All 1,536 single-field byte cases pass, each followed by a
simulated reboot; 255 invalid-marker cases, mixed corruption and normal controls
also pass. The three original speed/colour/brightness faults reproduce under
sanitizers before the fix and render safely afterward. Clang static analysis,
ASan/UBSan, changed-source/full-fixture C formatting and Python formatting/lint pass.

All 72 RF regressions, 15 slider-policy cases, 81 QMK tests (five existing skips),
applicable strict lint and four firmware builds pass. VIA retains its documented
upstream lint-name restriction. Exact formatting commands and build logs are
session artifacts under `/tmp/air75-eeprom-validation`.

The user reports testing passed and authorises committing and pushing the fix.
The optional non-reset flashing procedure was declined, so preservation of
pre-existing preferences across a firmware installation is not physically
verified. Corrupted-setting repair and reboot persistence are covered by native
tests; no hardware corruption injection was performed.

Optional follow-up: preserve a non-default lighting/sleep setup, flash without an
EEPROM reset, and power-cycle twice. Verify side mode/speed/colour/brightness,
including maximum brightness 5, sleep preference and main lighting remain as
saved. Check normal USB/RF/Bluetooth operation. Back up settings before any
optional factory-reset test; corruption injection is covered by native tests.

[Change index](../AIR75_V2_NKRO_AUDIT.md)
