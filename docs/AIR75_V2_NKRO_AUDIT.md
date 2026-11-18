# Air75 V2 upstream alignment and NKRO audit

## Current implementation — A–D (2026-11-18)

Implementation committed on `feature/nkro-rf-reliability` with explicit user
approval; no push or agent-performed flash. The user tested the custom firmware.
The earlier Linux slider change and its user-reported hardware test are preserved.
**Hardware validation is partial.** A short USB receiver-interruption trial
recovers held D without another key press; released A/S remain released. Native
event captures do not reproduce the initially reported stuck-key indications.
A subsequent seven-key trial verifies rollover before USB receiver removal and
no replay of Space after its physical release while disconnected. Recovery of an
E2 key held throughout the interruption remains untested.

### Hardware results and remaining coverage

After testing the A–D candidate, the user reports that keys released during a
brief receiver disconnection remain shown as pressed after reconnection. Keys
held throughout the interruption release normally afterward; pressing and
releasing an affected key clears it. Releasing every key during the interruption
leaves every key shown as pressed on reconnection. This observation alone does
not establish stale state at the Linux input layer.

The user confirmed USB receiver removal. Their `libinput debug-events
--show-keycodes` capture shows A/S/D presses at +7.159/+7.202/+7.594 s, then
releases for all three at +11.016 s immediately before device removal. After
re-enumeration there are no new A/S/D events; Ctrl+C is received normally. This
capture does not reproduce stuck keys in the Linux input events and is consistent
with a tester retaining highlights across removal. This first capture did not
establish whether D was still held after the receiver became ready.

In the follow-up capture, A/S/D are released at USB removal (+3.968 s).
After re-enumeration, F and the still-held D both register pressed at +14.150 s;
F releases at +14.211 s and D at +14.933 s. A/S do not reappear. Thus no stuck
A/S release is reproduced, and D releases correctly once reported. Passive held-key
recovery before the next key change is not demonstrated: D first appears with F.
The removal-to-F interval is about ten seconds. `sleep_handle()` can prepare
sleep after roughly five seconds in reported RF_DISCONNECT and requires new
activity to wake; CMD_24G_SUSPEND can also request sleep. This is a possible
explanation, not a confirmed cause, because this capture contains no module
status or sleep flags. The periodic sender deliberately respects these flags.

In the faster third trial, with no F press, A/S/D release at removal (+1.300 s).
After re-enumeration D alone registers pressed at +6.279 s and releases at
+7.969 s. A/S do not reappear. This verifies passive held-D recovery and correct
release in that physical trial; the user reports a shorter interruption. Device
addition lines have no timestamps, so exact reconnection latency is unknown.
The cause of the earlier F-dependent result remains unconfirmed. These three-key
tests do not exercise E2: each key fits in an E1 usage slot.

In the fourth capture, A/S/D/F/G/H are pressed sequentially on event15 and
Space registers at +8.806 s on event16 with no prior key releases. This verifies
seven simultaneous keys at the Linux input layer and, with the inspected encoder,
exercises the E2 overflow path. At receiver removal, the six event15 keys release
at +11.235 s and Space releases at +11.324 s. After re-enumeration there is no
Space event; A presses/releases at +21.245/+21.248 s. The user confirms Space was
physically released during disconnection. This passes the offline-release case
for that E2 key: it does not return as pressed after reconnection. It does not
test recovery of a bitmap key kept held across reconnection. The physical
sequence for the other six keys is unspecified. There are no timestamps on the
device-addition lines. An explicit all-up offline-release test, longer
interruptions, module reset and other transport coverage remain outstanding.

If longer-interruption passive recovery needs further diagnosis, capture radio status,
sleep flags and outgoing E1/E2 state before changing refresh or sleep behaviour.
If a real lost release is confirmed, capture the same data. The native tests
verify transmitted snapshots; they do not model receiver USB removal, module
duplicate suppression or forwarding after reconnection. Do not synthesize extra
keypresses, reset the radio on every recovery or change the protocol without
evidence and discussion.

Official master was fetched at `eea6258af51623a29c26a538f09b644c38f2c512`.
Two new keyboard-only commits (`0aa19c8d56`, `eea6258af5`) were integrated in
genuine merge commit `4aebe6682f`, following preparation with
`git merge --no-commit --no-ff upstream/master`. The fetched upstream tip is an
ancestor of this branch; no commits from that tip remain unintegrated. Shared
QMK transport/report code in these two upstream commits is unchanged from the
already reviewed base. Air75 work is separate: Linux slider policy `cfde361cd6`,
encoder A `70efa32c66`, capability B `6b2b0c6140`, clearing regressions C
`bb7c2efb7b`, and recovery D `6fc5e6db60`.

| Change | Production files | Result |
| --- | --- | --- |
| A — bounded encoder | `rf.c` | E2 buffer is 16 bytes; press/release bounds and both sends use its size. Full QMK comparison buffer stays 32 bytes; six E1 slots remain. Unsupported overflow changes no longer store invisible bits, generate redundant E2 frames or set its refresh flag. |
| B — capability | `tmk_core/protocol/host.c`, `host.h`, `rf_driver.c` | Adds a weak default-preserving `host_can_send_nkro_for_driver(driver, default_capability)` hook; Air75 enables capability for its selected RF/Bluetooth driver independently of USB protocol. No host-driver struct/ABI change. |
| C — clearing | Tests only | The exact conditional sequence fails without the Air75 override and passes with it using actual QMK report/clear functions. Ordinary RF→USB→RF and clear-before-mode-toggle sequences also pass. No standalone cache-reset helper added. |
| D — recovery | `rf.c` | Replays current E1/E2 snapshots after recovery, refreshes long-held state, and suppresses periodic output while disconnected, asleep or awaiting fresh reset status. |

`tests/air75_rf/native.inc` and `util/air75_rf_audit.py` contain the native
regressions. Review snapshots/source patches and stage logs are under
`/tmp/air75-nkro-review`; these are session artifacts, not committed files.

### Protocol and capability

The wire format is preserved: E1 is eight payload bytes (modifiers, zero reserved
byte, six usage slots); E2 is 16 payload bytes (zero leading byte, 15 bitmap bytes).
UART framing remains `5A / command / 01 / length / payload / checksum`, with
checksum `(sum(payload) & 0xff) ^ 0x5A`, five framing bytes and the existing triple
transmission. The sender's frame is not evidence of the over-air/dongle format.
High usages outside `0x00..0x77` still require a free E1 slot. No promotion,
report resizing, receiver reinterpretation or allocation rewrite was introduced.

| Selected transport | Capability decision | Mode selection |
| --- | --- | --- |
| USB BOOT | False | Normal six-key reports. |
| USB REPORT | Original QMK default: true | NKRO only when `keymap_config.nkro` is enabled. |
| Air75 RF / Bluetooth module | True, independent of USB protocol | Same configurable NKRO flag; six-key mode remains available. |
| QMK built-in Bluetooth / NONE | Original QMK paths, before the hook | Built-in Bluetooth capability / false for NONE. |

The hook is a small local QMK extension, not an existing official upstream API.
It is necessary because the present upstream decision cannot query a custom
driver's capability. Its weak implementation returns QMK's existing default;
the Air75 override changes only its own driver. Future upstream merges should
review this hook if QMK adds equivalent transport capability support. USB protocol
state is never forced or modified by the override.

### Recovery behaviour and limits

- Recovery sends both latest cached reports immediately, then twice more at the
  existing >50 ms interval. Each round retains the existing three UART copies.
  These are current snapshots, not a queue replaying presses from disconnection.
- Disconnect status invalidates recovery tracking immediately, including a brief
  disconnect/reconnect between housekeeping polls. Changing RF/Bluetooth profile
  also causes a full snapshot after the existing mode-switch clear.
- A local module reset retains current keyboard state but waits for a validated
  matching-link status **data payload** before periodic replay. An ACK-only frame,
  malformed checksum or another profile's status does not satisfy that wait.
- Held keys/modifiers keep keyboard refresh active beyond the old nominal 20 s
  inactivity window. The E2 activity flag is no longer cleared while held state
  needs refresh. All-up idle retains the existing recent-activity window and then
  stops; recovery's three rounds remain available even after a long all-up idle.
- Sleep entry and sleep preparation suppress periodic refresh. Wake/resume can
  replay current state. Activity, sleep, lighting and EEPROM timers are untouched.
- This adds traffic during long holds and bounded recovery; power, latency and
  sleep behaviour require physical regression tests. E1/E2 updates remain separate
  and non-atomic. No end-to-end ACK, sequence number or receiver implementation
  is available. Recovery is best-effort; silent downstream losses, module duplicate
  suppression and missed all-up reports beyond retry windows need receiver/host
  captures. Short taps completed entirely offline are not replayed.
- Consumer/system releases, UART ACK-wait/parser concerns and extended-mouse
  format support remain outside these keyboard-report fixes. The older ordinary
  reset-defect claim is withdrawn; the conditional issue is addressed by B.

### Verification results and commands

| Stage | Automated result |
| --- | --- |
| A | 12 encoder cases plus 10 byte-for-byte comparisons against `9c8d758b1c` encoder output; all four keymaps compile, lint and changed-source formatting pass. |
| B | 21 transport/default-hook/connection cases; all four keymaps compile; selected QMK suites pass 81 tests with five existing skips. |
| C | Four clearing checks: actual QMK insertion/deletion, report generation/dispatch and clear chains; conditional reproduction remains when the board override is disabled, and is absent when enabled. |
| D / final | All 63 native checks pass: 22 encoder/comparison, 21 transport, four clearing, 16 recovery. New recovery case 60 failed before D as expected. Fifteen Linux/legacy policy scenarios pass. QMK suites again pass 81 tests / five existing skips. |

Native tests compile copied, unmodified production function bodies and actual
report/buffer declarations; core weak hook and board override link from separate
translation units. They use actual QMK report/clear helpers with one-shot features
disabled, and simulate clocks, GPIO, UART output, USB protocol and connection
selection. Optional built-in Bluetooth/NONE branches and the unoverridden default
are compiled/tested separately. These are not matrix, radio, receiver or physical
USB tests. QMK suites separately cover basic actions/one-shots, mousekeys and
housekeeping; QMK's normal keyboard-report test utility does not support NKRO.

All native variants pass `-Wall -Wextra -Werror`, Clang static analysis with findings
treated as errors, AddressSanitizer and UBSan. The sanitizer run required execution
outside sandbox tracing because LeakSanitizer cannot operate under ptrace.
Core C formatting, changed RF line ranges, full native fixture formatting, Python
formatting/flake8 and staged/unstaged whitespace checks pass. Existing RF code was
not subjected to an unrelated whole-file reformat.

```sh
python3 util/air75_rf_audit.py --stage encoder --analyze
python3 util/air75_rf_audit.py --stage transport --analyze
python3 util/air75_rf_audit.py --stage all --analyze
python3 util/air75_nkro_mode_test.py
qmk test-c -t basic -t mousekeys -t housekeeping -j 2
qmk lint --strict -kb nuphy/air75_v2/ansi
qmk lint --strict -kb nuphy/air75_v2/ansi -km custom
qmk lint --strict -kb nuphy/air75_v2/ansi -km default_nuphy
qmk lint --strict -kb nuphy/air75_v2/ansi -km via
qmk compile -kb nuphy/air75_v2/ansi -km custom
qmk compile -kb nuphy/air75_v2/ansi -km default
qmk compile -kb nuphy/air75_v2/ansi -km default_nuphy
qmk compile -kb nuphy/air75_v2/ansi -km via
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-c -n tmk_core/protocol/host.c tmk_core/protocol/host.h
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-python -n util/air75_rf_audit.py util/air75_nkro_mode_test.py
PYTHONPATH=/tmp/air75-audit-tools python3 -m flake8 util/air75_rf_audit.py util/air75_nkro_mode_test.py
PYTHONPATH=/tmp/air75-audit-tools python3 /tmp/air75-nkro-review/check-format.py
git diff --check
git diff --cached --check
```

Strict keyboard and in-tree keymap lint pass. Explicit VIA lint still fails
upstream's ban on the `via` name, including external userspace; its firmware builds
successfully. No keymap was removed or renamed to silence this existing policy.
Final binary sizes: custom 48,542; default 57,650; default_nuphy 47,878; VIA 52,818
bytes. No compiler warnings. The previously tested Linux policy remains intact.
The user has tested the custom candidate; physical coverage and remaining gaps
are recorded above. Other keymap builds are compile checks only.

Before the approved commits, formatting/flake8, strict keyboard/custom/default_nuphy
lint, all 63 native checks with static analysis and sanitizers, 15 slider-policy
cases, the QMK suites (81 passes / five existing skips), and all four firmware
builds were rerun successfully. Firmware source matches the tested candidate.

### Outstanding physical checks

1. USB BOOT/REPORT and six-key/NKRO modes: verify normal keys and modifiers;
   repeat RF typing while a separate wired test host changes USB protocol.
2. Reproduce C with seven held keys, BOOT, release/repress the overflow key,
   REPORT, clear; expect no phantom key in later refresh. Repeat RF→USB→RF.
3. Hold 7–12 keys and modifiers >30 s, then release together; verify all keys
   remain down until released and the final state stays empty. Test E1 slot
   turnover and `0x77/0x78`/high usages; unsupported overflow remains limited.
4. Disconnect briefly and >30 s, release some/all keys while offline, then
   reconnect with/without keys still held. Expect the current set without a
   further keypress, with no replay of completed offline taps.
5. Reset the radio and replug/reset the receiver while keys are held and after
   release. Capture PB6/PB7 UART and all receiver HID interfaces to establish
   whether the module forwards snapshots and how downstream state converges.
6. Repeat on BT1–3 and the six-key configuration; verify profile changes clear
   the old state. Exercise sleep/wake, extended idle, lighting load, Fn mappings,
   indicators, battery reporting and settings persistence. Compare latency and
   power during long holds; periodic refresh must cease during sleep.

## Historical investigation and pre-A–D findings

The remainder records earlier states and tests. Present-tense descriptions of
unfixed capability, buffer and recovery issues below refer to the audited
baseline; the current implementation and remaining limits are described above.

Audit date: 2026-11-18. Source inspected: `92bd10b82e06293a7bd2fcb58a29ca83677c5332`.
This report supersedes the current-status statements in the older migration notes.
No Air75 firmware behaviour was changed by the initial audit, which involved no
hardware testing. Subsequent user-reported hardware validation is recorded below.
The subsequent user-authorised Linux slider change is described below; it is
separate from both the audit findings and the committed upstream merge.

## Linux slider policy change (2026-11-18, uncommitted)

The user reports completing the earlier checks and requests NKRO for Linux as
well as Windows. Linux supports bitmap HID keyboard reports; the former six-key
limit was the Air75 slider policy, not a Linux operating-system limit. See the
[Linux HID report processing implementation](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-core.c).

`custom/config.h` now sets `AIR75_MAC_MODE_NKRO true`. All three Mac-position
assignments in `ansi.c` (startup, live slider scan and factory reset) use that
option, which defaults to `false` when absent. Thus `custom` uses NKRO in both
slider positions while keeping its distinct Linux/Windows mappings. Other
keymaps retain the Mac six-key policy; setting the option to `false` restores it
for `custom`. QMK's six-key reports, magic NKRO controls and USB boot-protocol
fallback remain intact. No RF framing, capability-selection or recovery fix is
included; both positions now use the existing Windows NKRO transport behaviour.

Regression command: `python3 util/air75_nkro_mode_test.py`. It runs actual startup,
slider-scan and factory-reset function bodies with hardware stubs for three
configurations (legacy default, custom enabled and custom explicitly disabled),
five scenarios each. It verifies layer selection as well as the NKRO flag.
All 15 scenarios pass with Clang warnings treated as errors and undefined-behaviour
sanitization. The ten existing RF scenarios and their Clang analysis also pass.
All four keymaps compile (custom 48,290 bytes); strict keyboard/custom lint passes.
Formatting was checked for the changed `ansi.c` lines and complete custom header;
the new Python test passes QMK formatting and flake8. Validation commands:

```sh
PYTHONPATH=/tmp/air75-audit-tools /tmp/air75-audit-tools/bin/clang-format --dry-run --Werror --lines=22:24 --lines=189:189 --lines=330:330 --lines=404:404 keyboards/nuphy/air75_v2/ansi/ansi.c
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-c -n keyboards/nuphy/air75_v2/ansi/keymaps/custom/config.h
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-python -n util/air75_nkro_mode_test.py
PYTHONPATH=/tmp/air75-audit-tools python3 -m flake8 util/air75_nkro_mode_test.py
python3 util/air75_nkro_mode_test.py
CC=clang python3 util/air75_rf_audit.py --analyze
qmk lint --strict -kb nuphy/air75_v2/ansi
qmk lint --strict -kb nuphy/air75_v2/ansi -km custom
qmk compile -kb nuphy/air75_v2/ansi -km custom
qmk compile -kb nuphy/air75_v2/ansi -km default
qmk compile -kb nuphy/air75_v2/ansi -km default_nuphy
qmk compile -kb nuphy/air75_v2/ansi -km via
git diff --check
```

**User-reported hardware validation (2026-11-18):** after the Linux slider change,
the user reports "tested and works". Record the custom Linux NKRO policy as
user-tested and working. Individual connection/profile, factory-reset and
interruption results were not enumerated; this does not establish that the RF
protocol/recovery findings below are fixed. For further regression coverage,
exercise more than six keys over USB, RF and Bluetooth after reboot and slider
transitions, and check Linux mappings and Fn lighting. Save settings before a
factory-reset test, since it resets them by design.

## Upstream status and completed merge

Initial branch: `air75-v2-qmk`, one commit ahead of its **local origin tracking
reference**. No tracked modifications or staged work; the user's untracked
`Agents.md` was read and preserved. It is the repository instruction file despite
the filename's case. Existing history includes the completed source port and
subsequent lighting/keymap work; the old notes' "port not started" status is stale.

| Reference | Verified state |
| --- | --- |
| `origin` | `ssh://git@ssh.github.com:443/boylie101/Air75.git` |
| `upstream` | `https://github.com/qmk/qmk_firmware.git` |
| Merge-base / previously fetched master | `d194c077ed964195ec16d255c7a4b89a794cf03b` (2026-11-08) |
| Fresh official master | `8252b6309752f0be6882c387e15989ba51323d8f` (2026-11-16) |
| Initial divergence | 57 project commits; 4 upstream commits |
| Audit branch | `audit/air75-upstream-nkro-2026-11-18` |
| Development branch after integration | `air75-v2-qmk` at `9c8d758b1c` |

The four new upstream commits were reviewed:

| Commit | Change | Treatment |
| --- | --- | --- |
| `ec8a1b6549` | ChibiOS control OUT buffer sizing (#26570) | Included in full, including its Anne Pro / GetawayVan memory reductions. |
| `3329239097` | BDN9 rev4 | Included unchanged through the upstream merge. |
| `19d66d336c` | OGR FRL TKL | Included unchanged through the upstream merge. |
| `8252b63097` | Kohaku Hotswap RGB | Included unchanged through the upstream merge. |

The applied fix replaces a two-byte EP0 destination with a 64-byte control-packet
buffer. Some ChibiOS low-level drivers write whole packets regardless of requested
length. Air75's STM32F072 USBv1 path is relevant. LED SET_REPORT still requests
two bytes; virtual-serial SET_LINE_CODING uses the same buffer and only copies a
complete line-coding structure after reception. GET_LINE_CODING stays unchanged.
The larger buffer costs 62 bytes of buffer storage before alignment effects.

There are no new report, host-driver, NKRO, UART, Bluetooth, schema, submodule or
breaking API changes in this four-commit range. All applicable shared source now
matches fetched master (`quantum`, `tmk_core`, `platforms`, `drivers`, `builddefs`,
`data`, Makefile and `.gitmodules` were compared). No keyboard-specific adaptation
was required. Air75 source files are untouched.

The initial selective patch was replaced with a **genuine merge prepared using
`git merge --no-commit --no-ff upstream/master`**, following the clarified
instructions in `Agents.md`. Git staged the full upstream change set;
`MERGE_HEAD` was exactly `8252b6309752f0be6882c387e15989ba51323d8f`. There were no
conflicts. The manual USB patch was backed up and removed before merging so that
Git applies the upstream version. All 21 incoming files match upstream exactly.
Air75 firmware source is unchanged. No history was rebased, squashed or rewritten.

With explicit user approval, merge commit
`9c8d758b1c1029878212e2e105c4c150a31e0893` was created with parents `92bd10b82e`
and `8252b63097`. Its scope is the full upstream merge plus `Agents.md` only.
The development branch `air75-v2-qmk` was then fast-forwarded to that commit.
`git merge-base --is-ancestor upstream/master air75-v2-qmk` succeeds;
`git rev-list --left-right --count air75-v2-qmk...upstream/master` reports
**58 project / 0 upstream-only commits**. The earlier selective integration has
therefore been corrected in both source and Git ancestry. No push or flash was
performed. This audit, the RF harness and other documentation changes remain
uncommitted and were excluded from the approved commit.

## Configuration and common QMK path

Relevant source links are relative to this checkout:

- [Keyboard metadata](../keyboards/nuphy/air75_v2/ansi/keyboard.json):
  `features.nkro: true` builds `NKRO_ENABLE`; 6×17 diode matrix, 84 physical keys,
  STM32F072, `usb.no_startup_check: true`. No QMK Bluetooth feature is enabled.
- [Keyboard control](../keyboards/nuphy/air75_v2/ansi/ansi.c):
  `dial_sw_fast_scan()`, `dial_sw_scan()` and factory-reset handling set
  `keymap_config.nkro = 1` for Windows and `AIR75_MAC_MODE_NKRO` for Mac (Linux
  in the custom keymap). The option defaults to `false`; `custom` enables it.
  This selection applies to USB, RF and all three Bluetooth profiles. It is not
  a per-transport capability test. A later QMK magic NKRO keycode can change the
  flag; the next OS-mode transition/reset reasserts the slider policy.
- [Report construction](../tmk_core/protocol/report.c): `add_key_to_report()` /
  `del_key_from_report()` select the bitmap or six-key array using **both**
  `host_can_send_nkro()` and `keymap_config.nkro`. Bitmap index is `usage >> 3`,
  mask `1 << (usage & 7)`. QMK actions/macros resolve layers, modifiers and basic
  HID usages before reaching this layer; Fn, lighting and connection keycodes
  are handled locally, not sent as bitmap bits.
- [Action reports](../quantum/action_util.c): `keyboard_report` and `nkro_report`
  hold separate states. `get_mods_for_report()` combines real/weak/one-shot
  modifiers; `send_keyboard_report()` selects `send_6kro_report()` or
  `send_nkro_report()`. Each suppresses unchanged reports using a static previous
  report. Those caches do not know about radio delivery or host-driver changes.
- [Host dispatch](../tmk_core/protocol/host.c): `host_keyboard_send()` and
  `host_nkro_send()` dispatch through the selected `host_driver_t`; the latter
  sets report ID 6. Air75 saves the USB driver in `m_host_driver`, and selects
  `rf_host_driver` using `host_set_driver()` in `switch_dev_link()` /
  `dev_sts_sync()` / initial mode scan. The physical slider invokes report clears.

[Report definitions](../tmk_core/protocol/report.h): `report_keyboard_t` contains
modifier, reserved and six usage bytes (8 bytes, or 9 with a shared report ID).
`report_nkro_t` is **32 bytes**: report ID, modifier, 30 bitmap bytes. The bitmap
addresses usages `0x00..0xEF`; the eight modifier usages `0xE0..0xE7` normally use
the dedicated modifier byte. The old 16-byte diagram in this upstream header's
comment is historical, not the current QMK structure definition.

## USB path

```text
matrix -> QMK actions -> report.c / action_util.c -> host_nkro_send
       -> USB host driver -> ChibiOS send_nkro -> shared interrupt IN endpoint
```

[USB descriptor](../tmk_core/protocol/usb_descriptor.c) declares report ID 6,
eight modifier bits and 240 keyboard bits. The shared endpoint is 32 bytes.
[ChibiOS USB](../tmk_core/protocol/chibios/usb_main.c) sends the entire 32-byte
NKRO structure through `send_report()` / `usb_endpoint_in_send()`; boot protocol
uses the eight-byte six-key report. Status LEDs return through USB OUT/control
reports. The upstream update fixes that control receive buffer, not rollover.

`host_can_send_nkro()` checks USB report versus boot protocol when built without
QMK's connection feature. USB report protocol starts enabled, and USB reset
restores it. Thus battery-only Windows use can enter NKRO without USB enumeration.
However, a USB host selecting boot protocol can also make the **radio** path fall
back to six keys: Air75's custom driver has no capability override. Merely
installing a `send_nkro` callback does not affect this decision.

## RF and Bluetooth path and formats

```text
QMK NKRO report -> rf_host_driver.rf_send_nkro -> uart_send_report_nkro
  -> uart_auto_nkro_send -> E1 six-key snapshot + E2 overflow bitmap snapshot
  -> UART/USART1 -> separate NuPhy radio module -> [unavailable RF firmware]
  -> 2.4 GHz receiver -> [unavailable receiver HID implementation] -> host
```

[rf_driver.c](../keyboards/nuphy/air75_v2/ansi/rf_driver.c) uses current QMK
`host_driver_t` report-pointer signatures. [rf.c](../keyboards/nuphy/air75_v2/ansi/rf.c)
implements a proprietary UART adapter, not standard USB NKRO forwarding.
[ansi.h](../keyboards/nuphy/air75_v2/ansi/ansi.h) defines link/command IDs and buffers.

| Storage / report | Exact content and role |
| --- | --- |
| `bitkb_report_buf[32]` | Previous QMK state, copied from `&report->mods` for 31 bytes; excludes QMK report ID. |
| `bytekb_report_buf[8]`, command `0xE1` | Modifier, reserved zero, six HID usage slots. Also used for NKRO-off reports and refresh. |
| `uart_bit_report_buf[32]`, command `0xE2` | Cached overflow bitmap. Only first **16 bytes** sent: byte 0 stays zero; bytes 1–15 encode usages `0x00..0x77`. It is not a full QMK NKRO report. |
| `USART_MGR_STRUCT` | 64-byte TX and RX buffers. E1 frame is 13 bytes, E2 frame is 21 bytes. Buffer capacity is not evidence the module accepts larger reports. |

The encoder XORs old/new QMK bitmaps and scans changed usages in ascending order.
A newly pressed key takes the first empty E1 slot; only when all six are occupied
does it set an E2 bit. Release searches E1 first, otherwise clears the E2 bit.
Held overflow keys are not promoted when a slot becomes free. Key order/history
therefore matters. A modifier change updates **E1 only**; E2 byte 0 is never
populated with modifiers. When both reports change, E2 is sent before E1. During
periodic refresh, E1 precedes E2. A multi-key update is not atomic at this boundary.

Each UART report frame is:

```text
5A | command | 01 | payload_length | payload | ((sum(payload) & FF) XOR 5A)
```

`uart_send_report()` drops transmission unless the dial is initialised, mode is
wireless and `rf_state == RF_CONNECT`. Local caches are updated even when it drops
the send. `UART_Send_Bytes()` sends each report three times, with wake-pin pulses,
50 µs guards, `50 + length*32` µs post-transmit delay and 200 µs gaps. UART is
460800 baud; the code sets M0/PCE directly after QMK `uart_init()` (intended 8 data
bits plus even parity). This is the **MCU-to-radio** frame; neither the over-air
packet length nor an over-air retransmission policy is specified by this code.

The encoded state can describe all ordinary keys in the existing keymaps at once
at the UART boundary, provided the module/receiver combines E1 and E2 correctly.
It is not arbitrary QMK NKRO: overflow usages `>= 0x78` are silently outside the
transmitted slice. Such a key still works in an E1 slot. For example `KC_INT1`
(`0x87`) can work as an early press and disappear as a seventh key. There is no
single fixed RF key-count limit derivable from the sender: six arbitrary usages
plus the low-usage overflow subset are representable; module/HID limits are unknown.

Bluetooth profiles 1–3 use exactly the same driver, encoder and UART commands;
`LINK_BT_1..3` versus `LINK_RF_24` selects the module mode. There is no separate
Bluetooth NKRO size/capability check in this firmware. It is unknown whether the
module forwards E2 over Bluetooth, merges it into another HID report, or imposes
a different bound. Historical user-reported Bluetooth >6-key testing in the older
notes is evidence only for that configuration, not a fresh RF verification.

Consumer/system usages bypass the keyboard bitmap and use `0xE3` / `0xE4`, two-byte
usage payloads; mouse uses `0xE0`, five bytes starting at `buttons`. Their releases
do not get the periodic keyboard refresh. Enabling extended QMK mouse reports
would require revisiting that fixed five-byte format; it is not enabled here.

**Receiver boundary:** this checkout contains STM32 keyboard firmware, not the
Air75 radio or dongle implementation/descriptors. There is no evidence that the
receiver reconstructs a QMK C structure. Unioning E1/E2, modifier ownership,
receiver rollover and over-air loss/order guarantees require receiver source,
protocol documentation or captures. End-to-end native RF NKRO is **unproven**.

## Recovery and issues

`uart_send_report_func()` runs in housekeeping and refreshes E1 every >50 ms
while `no_act_time <= 2000`. That counter advances once per nominal 10 ms tick:
about 20 seconds, potentially longer if housekeeping is delayed. E2 refresh runs
only with `f_bit_kb_act`; an E2 change sets it, inactivity clears it. Presses,
releases, consumer/mouse reports and keyboard processing reset inactivity, but a
modifier-only/E1-only update does not re-enable E2 refresh.

`dev_sts_sync()` polls status every 200 ms and requests a radio reset after five
missed sync cycles. Receiving a valid frame resets the sync counter. Reconnect
does not unconditionally resend both current keyboard states. Three duplicate
frames and recent-activity refresh can repair some loss, but there is no keyboard
report sequence number, delivery confirmation, backpressure or snapshot transaction.
The UART stream preserves local call order; over-air/dongle ordering is unknown.

| Priority | Finding | Evidence / consequence |
| --- | --- | --- |
| P1 | Wireless NKRO depends on USB protocol | Actual `host_can_send_nkro()` reproduced with report/boot states. A protocol change can select a different QMK state buffer while keys are held, without an Air75 transport clear. |
| P1 | Incomplete reconnect resynchronisation | Send dropped while disconnected still advances caches. A release followed by >20 s disconnect has no immediate keyboard snapshot replay on reconnect. New activity may repair it; receiver timeout behaviour is unknown. |
| P1 | Refresh stops for long-held overflow | After inactivity E2 refresh stops. Modifier-only activity restores E1 refresh but not E2. Radio/receiver resets during long holds can lose the overflow subset. |
| P2 | Conditional mixed-protocol state can survive clearing | The normal `switch_dev_link()` path clears through the current RF driver before changing modes; host-side tests confirm ordinary RF→USB→RF clearing. The earlier synthetic mixed-callback test did not establish a normal reset defect. A stronger host-side test using actual QMK report/clear functions reproduces stale E2 replay after USB report→boot→report transitions with an overflow key released/repressed in boot mode. This remains conditional on the separate USB/RF capability coupling and needs physical verification. A lingering active flag with a zero bitmap is not itself a stuck-key defect. |
| P2 | Usage-dependent rollover limit | High usages survive E1 but are lost in E2. Reproduced with `0x87`. Ordinary shipped mappings fit; dynamic VIA assignments can expose it. |
| P2 | Modifier and report-union assumptions | E2 modifier byte remains zero, E1 carries modifiers, split sends are non-atomic. Sender behaviour reproduced; a host-visible modifier fault needs module/receiver evidence. |
| P2 | Best-effort release transport | All three copies can be lost; no per-report ACK check. Consumer/system releases lack keyboard-style refresh. Compilation cannot establish delivery correctness. |
| P2 | Synchronous command ACK wait cannot parse replies | `uart_send_cmd()` clears `f_uart_ack`, sends, then waits without calling `uart_receive_pro()`. Only the foreground parser sets that flag. Normal waited calls time out; initialisation loops explicitly parse afterwards. Most callers ignore return values. Static finding, not a new upstream regression. |
| P2 | Receive framing assumptions | Parser treats all available bytes separated by a 200 µs idle as one frame: fragments/coalesced frames are discarded. Three-byte ACKs bypass payload-size validation yet still enter command handlers; unexpected ACK command IDs can consume stale payload bytes. RX checksum is additive without TX's XOR; asymmetry may be intentional and needs captures. |
| P3 | Timing/maintenance coupling | Direct USART register changes, positional host-driver initializer and hand-managed report lengths need future API review. Side LED bit-banging holds a ChibiOS system lock for its transfer; measure UART receive overruns under lighting load. Existing `rgb_led_t` is a deprecated alias. |

The defect characterisation tests use actual function bodies/structure definitions
from this checkout, not a reimplementation of the encoder. Hardware/time/UART
calls are stubbed. The clear test stubs QMK `clear_keyboard()` to isolate explicit
Air75 sends and injects mixed callback history. These tests do not model the full
scan loop, status parser, radio, receiver, USB host or RF channel.

Review of the suggested 16-byte-buffer patch used temporary native tests
(`/tmp/air75-review-proposal.py`) against both the current encoder and the proposed
bounds-checked variant, with actual QMK report/clear helpers and stubbed UART/USB
protocol state. Supported low-usage press/all-up frames were byte-for-byte equal;
ordinary RF→USB→RF clears passed in both. Unsupported high-usage overflow changes
sent redundant E2 snapshots in the current code and none in the candidate, so
packet format is preserved but transmission count/refresh activation changes.
Both variants reproduced the conditional mixed-protocol case described above.
No proposed firmware patch was applied. The bounds check is useful hardening,
not a repair for high-usage rollover or USB-dependent capability selection.

## Comparison and recommendations (approval required for behaviour fixes)

Current upstream retains separate six-key/NKRO report types, `host_driver_t`,
`host_set_driver()`, shared USB NKRO reports and built-in Bluetooth capability
queries. Air75's report-pointer APIs match. Its old NuPhy core UART forwarding
patch and `keyboard_protocol` assignments are absent; core is clean upstream.
Earlier EEPROM/GPIO/UART/WS2812/keycode migrations are already present and need
not be reapplied. Upstream offers `host.default.nkro`, but replacing the physical
  OS-switch policy with a global default would change behaviour.

QMK has a `CONNECTION_HOST_2P4GHZ` enum/keycode, but its candidate list disables
that host with `#if 0`; no equivalent Air75 radio/dongle implementation is supplied.
Simply enabling built-in Bluetooth/connection logic could select NONE or replace
Air75's driver and is not an appropriate mechanical migration.

1. **Capture the actual module/receiver contract first.** Record firmware versions,
   RF receiver HID descriptors and simultaneous UART/host reports. Establish E1/E2
   union, modifier ownership, accepted E2 length and reconnection/timeout behaviour.
   Do not lengthen E2 to 31 bytes on the strength of a 64-byte UART buffer.
2. **Propose explicit host-driver NKRO capability** independently of USB protocol,
   ideally as an upstreamable hook. Preserve USB boot behaviour and the slider
   policy for the selected keymap. Test held-key transitions; risks include host switching and action
   buffer/cached-report migration. Do not spoof USB protocol state.
3. **Propose full state replay at reconnect/reset/wake**, retaining pending state
   until sent, and make Air75's clear reset every report cache. Derive E2 refresh
   eligibility from retained state/pending releases rather than inactivity alone.
   Preserve the proven packet contract. Extra traffic/power use and held-key policy
   after reconnect must be agreed and tested before implementation.
4. **Resolve high-usage and modifier handling only with receiver evidence.** Options
   include a negotiated larger bitmap or a documented allocator/rollover policy;
   reordering/promoting keys can alter event order and key availability. Add tests
   for simultaneous E1/E2 releases before changing allocation.
5. **Then improve ACK/parsing/UART configuration incrementally.** Parse framed replies
   during command waits, match commands, validate ACK-only dispatch, and use QMK's
   `UART_CR1` configuration after parity capture. Test lighting and sleep under load.
   Designated host-driver fields/shared prototypes can be a separate cleanup.

No recommendation above has been implemented in the Air75 firmware.

## Automated verification

QMK CLI 1.2.0; QMK ARM toolchain GCC 15.2.0; host Clang 22.1.8. Commands below
were run from the repository root. Formatter dependencies were installed only in
`/tmp/air75-audit-tools` (clang-format 20.1.8, yapf 0.43.0, flake8 7.5.0).

```sh
git fetch --no-tags upstream master
# Merge preparation checks; MERGE_HEAD exists only before the merge commit.
git merge --no-commit --no-ff upstream/master
git rev-parse HEAD MERGE_HEAD upstream/master
git diff --name-only --diff-filter=U
git diff --cached --check
git rev-list --left-right --count HEAD...upstream/master
git diff --exit-code upstream/master -- tmk_core/protocol/chibios/usb_main.c
git diff upstream/master -- quantum tmk_core drivers platforms builddefs Makefile data .gitmodules
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-c -n tmk_core/protocol/chibios/usb_main.c
PYTHONPATH=/tmp/air75-audit-tools PATH=/tmp/air75-audit-tools/bin:$PATH qmk format-python -n util/air75_rf_audit.py
PYTHONPATH=/tmp/air75-audit-tools python3 -m flake8 util/air75_rf_audit.py
qmk format-text tmk_core/protocol/chibios/usb_main.c util/air75_rf_audit.py docs/AIR75_V2_NKRO_AUDIT.md docs/AIR75_V2_UPSTREAM_MIGRATION.md docs/AIR75_V2_UPSTREAM_COMPATIBILITY_AUDIT.md keyboards/nuphy/air75_v2/ansi/readme.md
qmk lint --strict -kb nuphy/air75_v2/ansi
qmk lint --strict -kb nuphy/air75_v2/ansi -km custom
qmk lint --strict -kb nuphy/air75_v2/ansi -km default_nuphy
qmk lint --strict -kb nuphy/air75_v2/ansi -km via
qmk compile -kb nuphy/air75_v2/ansi -km default
qmk compile -kb nuphy/air75_v2/ansi -km default_nuphy
qmk compile -kb nuphy/air75_v2/ansi -km custom
qmk compile -kb nuphy/air75_v2/ansi -km via
qmk compile -kb nuphy/air75_v2/ansi -km default -e VIRTSER_ENABLE=yes -e CONSOLE_ENABLE=no
qmk test-c -t basic -t mousekeys -t housekeeping -j 2
CC=clang python3 util/air75_rf_audit.py --analyze
git diff --check
# Post-commit verification on the development branch.
git merge-base --is-ancestor upstream/master air75-v2-qmk
git rev-list --left-right --count air75-v2-qmk...upstream/master
```

- Completed merge includes every upstream change; all 21 incoming files match
  fetched upstream exactly, and relevant shared-source diff is empty. No conflicts
  remain. Post-commit ancestry verification passes with zero upstream-only commits.
- Changed-core C formatting, Python formatting/lint and unstaged-change whitespace
  checks pass. `git diff --cached --check` reports one inherited upstream whitespace
  error: the blank line at `keyboards/cannonkeys/ogr_frltkl/readme.md:14` contains
  spaces. That incoming file is retained byte-for-byte from upstream; this is not
  an Air75 formatting regression. No unrelated keyboard-wide reformat was applied.
- Strict keyboard/default, custom and default_nuphy lint pass. Explicit **VIA lint
  fails**: upstream rejects a keymap named `via` by policy, including this configured
  external userspace keymap. It is not a compiler/schema failure; no keymap was
  removed or renamed to silence it. VIA is sourced from the existing
  `/home/liam/Keyboard/qmk_userspace_via`, not restored under this repository.
- All four keymaps and the extra virtual-serial USB-path variant compile. Normal
  default was rebuilt after the variant so its artifact has normal features.
  Binary sizes: default 57,398; default_nuphy 47,626; custom 48,294; VIA 52,566;
  virtual-serial/no-console variant 54,982 bytes. No compiler warnings were emitted.
- QMK suites: basic 61 passed / 5 pre-existing skips, mousekeys 19 passed,
  housekeeping 1 passed (81 passed total). These suites do not test the Air75 RF
  transport. Upstream `tests/test_common/keyboard_report_util.cpp` explicitly
  rejects `NKRO_ENABLE`; these are not end-to-end NKRO unit tests.
- The ten host characterisation scenarios pass under `-Wall -Wextra -Werror` and
  Clang undefined-behaviour sanitization; Clang static analysis of the extracted
  functions also passes with analyzer findings treated as errors. Passing
  `REPRODUCED` cases confirms the
  documented limitation, not correct wireless operation. GCC sanitizer linking
  failed because the installed `libubsan.so.1.0.0` was missing; Clang was used.
- Initial builds compiled but sandbox-blocked the final userspace binary copy;
  approved reruns completed those copies. Missing formatter/network access was
  resolved with an approved temporary tool install. No checks were silently skipped.

After preparing the genuine merge, the four normal keymap builds, Air75 lint,
changed-core/Python formatting, Python lint, Clang analysis/sanitized RF harness
and selected QMK suites were rerun. Results above remain valid, with the inherited
staged whitespace error now recorded explicitly. All 21 incoming files were
compared byte-for-byte against upstream, all 21 tracked Air75 files in the index
against pre-merge HEAD, and `MERGE_HEAD` against fetched master; those integrity
checks passed. The pre-commit ancestry check exited 1 as expected; after the
approved merge commit and development-branch fast-forward, the ancestry check
succeeds and the divergence is 58 / 0.

Logs are `/tmp/air75-{compile-*,lint*,format-*,qmk-tests,rf-audit}.log`; they are
session artifacts, not permanent project files. The merge-preparation reruns use
the same compile logs and `/tmp/air75-merge-{lint*,qmk-tests,rf-audit}.log` for the
other checks. No hardware result is implied.

## Required physical verification

Record keyboard/radio/dongle firmware versions, host OS, keymap, OS-slider
position and connection mode. Capture raw HID reports (`hid-recorder`/hidraw or
USB capture) and OS key events (`evtest` on Linux). Browser testers alone can
miss system shortcuts and multiple HID collections. Observe all receiver input
interfaces; exclude host auto-repeat from transition counts. Keep the same OS,
keymap and key sequence when comparing USB and RF. Save captures with timestamps.

| Test | Procedure | Expected result / failure evidence |
| --- | --- | --- |
| USB baseline / boot | Windows slider: hold 7, 12, then as many distinct ordinary keys as possible; repeat in USB boot protocol. Mac/Linux slider: repeat. | With `custom`, both positions select NKRO and all distinct held usages should appear in report protocol. Legacy Mac policy, boot protocol or NKRO-off: at most six ordinary keys plus modifiers. Unsupported extra presses in six-key mode must not leave stuck keys. |
| RF rollover | Repeat the identical holds over 2.4 GHz, with USB data disconnected; then with USB attached in report and boot protocols. | Intended Windows NKRO should track the complete ordinary-key set; a seventh missing key, USB-dependent change or early release is a failure to record. Current capability coupling may cause this test to fail. |
| Matrix coverage | Press Q/W/E/R/T/Y/U; then A/S/D/F/G/H/J; then 1/Q/A/Z/Space/Left/Up/Right. Release in forward, reverse and simultaneous groups. | Each supported held key remains present until its own release, with no ghost usage or duplicate down transition. Expand across every row/column with a fixture for near-all-key coverage. |
| Slot history | Hold A–F, add G–J, release A–C, add K–M, release everything together; repeat reversed and with modifiers. | E1 and E2 union matches physical state at each stable step; final state is empty and stays empty through refresh. Observe transient split-report ordering separately. |
| Modifiers | With >6 ordinary keys held, add/remove left/right Ctrl/Shift/Alt/GUI individually and in combinations. Use an isolated input logger to avoid desktop shortcuts. | Correct eight modifier bits with every supported key; no transient modifier loss due to zero E2 modifier byte and no stuck modifier after release. |
| High usages | In a temporary test keymap or backed-up VIA mapping, assign 0x77, 0x78 and `KC_INT1` 0x87. Press each first, then after six ordinary keys, then free a slot without releasing it. Restore the original mappings. | USB reports each assigned usage. Record RF loss/history dependence at the boundary; the current sender is expected to omit high usages in overflow. This test does not authorise permanent remapping. |
| Rapid changes | Repeat 7–12-key chords, overlapping presses/releases and all-up transitions at 5–20 ms intervals using a fixture; run for 60 s with lighting active. | Host transition sequence matches the stimulus apart from measured transport latency; no missed taps, stuck state, reordering or duplicates. Quantify latency and loss; three UART copies do not prove delivery. |
| Short interruption | Hold >6 keys and modifiers, interrupt receiver/link briefly, release some/all while disconnected, restore within 20 s; repeat with keys still held. | Receiver converges to actual current state without another keypress; capture whether refresh repairs lost releases and how long it takes. |
| Long interruption / hold | Repeat with >30 s outage; separately hold >6 keys >30 s then reset/reconnect the receiver and change only a modifier or one E1 key. | Full held state/released state is restored; no stale E2 replay or missing overflow subset. Current refresh/reconnect defects may fail this test. |
| Channel/slider/sleep | While keys are held, switch USB↔RF↔BT1/2/3 and Windows↔Mac/Linux; test sleep/wake, dongle replug and host suspend/resume. | Old host receives releases where reachable; new host starts from the agreed clear/current-state policy. No key is stuck on either host; established layer/lighting/persistence behaviour remains. |
| Bluetooth parity | Run rollover, modifiers, slot history and reconnection on each of BT1–3. | Document actual profile/host capability separately from RF. Sender equivalence alone is insufficient. |
| Other features | Compare custom/default_nuphy mappings, Fn illumination, Fn+backslash on UK host layout, lighting settings/effects/persistence, media/system keys and mouse wiggler before/after. Exercise Caps/Num/Scroll LEDs over USB and RF. | Existing behaviour preserved; no lost media releases or unexpected mouse movement. USB LED tests exercise the upstream control-buffer change. |

Use a logic analyser on PB6/PB7 and the wake line when UART and host results
disagree. Capture E1/E2 before/after interruption, verify parity/checksum and map
them to receiver HID reports. This distinguishes sender, radio, receiver, matrix
and host failures. No claim of correct RF NKRO should be made until these paths
have been measured.
