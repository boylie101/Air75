# ACK-only dispatch hardening

Status: implemented; user reports flashing and fully testing the candidate.

Follow-up to review item 1: `RF_Protocol_Receive()` now treats
three-byte `CMD_RF_STS_SYSC` and `CMD_READ_DATA` ACKs as receipt acknowledgements
only. They still set `f_uart_ack`, restore the existing sync counter and clear
the receive-frame bookkeeping, but do not read stale payload bytes, change
device/configuration state, count profile mismatches or mark a data request
complete. Status/reset recovery and configuration reads require a validated data
frame. Handshake, pairing and suspend notifications retain their previous ACK
and data-frame handling. The RF payloads, framing, checksums and transmit path
are unchanged; no dongle or radio-module firmware update is required by this fix.
Command ACK waits and stream framing remain separate outstanding review items.

Nine native receive regressions cover stale status/configuration buffers,
ACK and data notifications, invalid headers/lengths/checksums, ACK-then-data
completion and module-reset recovery. The status-ACK regression fails before
the fix. Run them with `python3 util/air75_rf_audit.py --stage receive --analyze`;
`--stage all --analyze` runs these plus the existing 63 checks (72 total).
These tests use the actual production parser with simulated module frames.
The [earlier A–D hardware results](../AIR75_V2_NKRO_AUDIT.md) apply to the preceding firmware.

Validation passes: all 72 native checks with Clang static analysis, ASan and
UBSan; the QMK basic/mousekeys/housekeeping suites (81 passes, five existing
skips); strict keyboard/custom/default_nuphy lint; and custom/default/
default_nuphy/VIA firmware builds. Changed RF lines and the full native fixture
pass clang-format; the runner passes QMK Python formatting and flake8; whitespace
checks pass. An older format helper additionally checked untouched `rf_driver.c`
and reported its two existing blank-line violations; that file is unchanged.
Commands used for this follow-up:

```sh
python3 util/air75_rf_audit.py --stage all --analyze
qmk test-c -t basic -t mousekeys -t housekeeping -j 2
qmk lint --strict -kb nuphy/air75_v2/ansi
qmk lint --strict -kb nuphy/air75_v2/ansi -km custom
qmk lint --strict -kb nuphy/air75_v2/ansi -km default_nuphy
for keymap in custom default default_nuphy via; do
    qmk compile -kb nuphy/air75_v2/ansi -km "$keymap" || break
done
PYTHONPATH=/tmp/air75-audit-tools python3 /tmp/air75-ack-validation/check-format.py
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-python -n util/air75_rf_audit.py
PYTHONPATH=/tmp/air75-audit-tools python3 -m flake8 util/air75_rf_audit.py
git diff --check
```

The user subsequently reports the keyboard was flashed and fully tested, and
authorised committing and pushing this change. This records user-reported
physical validation; individual test results and new captures were not supplied.

Regression procedure for this follow-up: cold-boot USB, RF and Bluetooth;
verify typing, battery/Caps Lock status, profile selection and long-press pairing.
Repeat RF receiver removal with seven keys, releasing some/all while offline
and keeping the seventh held in a separate trial. Expect current state after
reconnection and normal releases. Startup/profile reads must still complete
when the real module supplies its data response; host tests do not establish
the module's actual reply timing or physical wireless delivery.

[Change index](../AIR75_V2_NKRO_AUDIT.md)
