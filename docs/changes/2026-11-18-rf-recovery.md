# D — Held-state refresh and radio recovery snapshots

Commit: `6fc5e6db60`. The user tested the custom candidate containing A–D.

The former recent-activity window could stop refresh during long holds. A local
report cache could also advance while disconnected without an unconditional
snapshot on recovery. This change improves sender resynchronisation while
retaining the existing E1/E2 payloads and UART framing.

In `rf.c`, recovery sends current E1/E2 snapshots immediately and twice more at
the existing >50 ms interval. Each round retains the existing UART triple-send.
Held keys/modifiers keep refresh active beyond the nominal 20-second window;
all-up idle eventually stops refreshing. These are current snapshots, not a
queue replaying taps completed offline.

Disconnect status invalidates recovery tracking, even between housekeeping
polls; profile changes also trigger a snapshot. A local module reset waits for
a fresh validated matching-link status data payload. Periodic refresh respects
sleep and wake-preparation flags. Existing sleep/activity/lighting/EEPROM timers
and the packet format are unchanged.

## Automated validation

```sh
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

At this commit, all 63 native checks passed: 22 encoder/comparison, 21 transport,
four clearing and 16 recovery checks. Recovery case 60 failed before D as expected.
Fifteen slider-policy cases and 81 QMK tests passed, with five existing QMK skips.
Clang static analysis, ASan/UBSan, relevant C/Python formatting, Python lint,
applicable QMK lint and all four builds passed. Explicit VIA lint rejects the
keymap name by upstream policy; its firmware builds. The native runner's later
[ACK follow-up](2026-11-18-rf-ack-only-dispatch.md) adds nine checks, making 72.

Native tests use actual production/QMK report and clear-function bodies, with
one-shot features disabled and clocks, GPIO, UART, protocol and connection state
simulated. They do not model module forwarding, receiver USB hotplug or the RF
channel. QMK's separate suites cover actions, mousekeys and housekeeping; their
standard keyboard-report utility does not support NKRO.

## Physical observations

The user unplugged the USB receiver and captured all its Linux input interfaces
with `libinput debug-events --show-keycodes`. Linux released the held keys at
device removal; the logs did not reproduce the initially reported stuck-key
highlights as stale Linux key presses.

| Trial | Observed result |
| --- | --- |
| Initial A/S/D capture | All three released at removal (+11.016 s); none reappeared. Whether D remained physically held after readiness was unconfirmed. |
| Longer A/S/D interruption | D first appeared together with a new F press (+14.150 s), then released (+14.933 s). A/S did not reappear. |
| Faster trial, no F | D appeared after reconnection (+6.279 s) and released (+7.969 s); A/S stayed released. Held E1-key recovery passed for this trial. |
| A/S/D/F/G/H plus Space | Seven keys registered before removal, with Space on the second keyboard interface. The user released Space while disconnected; it did not return pressed. This verifies that E2 offline-release case, not recovery of an E2 key kept held. |

Device-addition lines lack timestamps, so exact reconnection latency is unknown.
Sleep after roughly five seconds of reported disconnection, or a module suspend
notification, could explain the longer trial's dependence on F. This remains an
inference: neither module status nor sleep flags were captured.

## Limits and remaining hardware checks

Recovery remains best-effort: there is no end-to-end ACK, sequence number or
receiver source. E1/E2 updates are separate and non-atomic. Extra traffic during
long holds/recovery needs power and latency checks. Consumer/system releases
do not receive keyboard snapshot refresh. The RF high-usage limit remains.

Still test an E2 key held through reconnection, explicit all-up offline release,
>30-second holds/outages, simultaneous releases, modifiers, module/receiver
resets, USB protocol changes, all Bluetooth profiles, six-key mode and sleep/wake.
Compare keymaps, Fn lighting, battery/status indicators and persistence. If UART
snapshots and host events disagree, capture PB6/PB7 plus the wake line and receiver
HID reports before changing the protocol or generating artificial key presses.

[Architecture](../AIR75_V2_NKRO_ARCHITECTURE.md) · [Change index](../AIR75_V2_NKRO_AUDIT.md)
