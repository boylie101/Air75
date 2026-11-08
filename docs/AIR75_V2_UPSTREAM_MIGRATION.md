# Air75 V2 upstream QMK migration — status and priorities

**Status snapshot:** 2026-11-08  
**Repository:** [boylie101/Air75](https://github.com/boylie101/Air75)  
**Scope:** NuPhy Air75 V2 only.

This document records the current migration strategy, branch roles, known compatibility work, and the order in which work should proceed. Update it as findings are verified so it remains the hand-off point for future work.

## Objective

Move the Air75 V2 keyboard implementation off the abandoned NuPhy QMK fork and onto current official QMK master, while preserving the keyboard's hardware-specific behaviour and the existing known-good firmware baseline.

The goal is a maintainable upstream-based port, not simply a successful compile. Keep the migration reviewable, isolate changes, and verify behaviour before considering the port complete.

## Guardrails

- **Protect `baseline`.** It is the known-good reference point; do not use it as the migration work branch or rewrite its history.
- Keep upstream migration work isolated on `air75-v2-qmk-port`.
- Keep the clean upstream snapshot on `qmk-master`; do not mix Air75 port changes into that branch.
- Limit scope to the Air75 V2. Do not make unrelated keyboard changes.
- Preserve NuPhy/Air75-specific functionality unless a deliberate, documented replacement is required.
- Do not compile/flash experimental firmware as a substitute for reviewing compatibility. Build and validate changes first; do not flash until the candidate has passed the agreed checks.
- Make small, logically separated commits. Record the reason for compatibility changes and any behaviour changes.

## Branch map (2026-11-08)

| Branch | Recorded position | Role / status |
| --- | --- | --- |
| `baseline` | `98396e9a` | Protected known-good baseline; includes the style-cleanup merge (PR #2). |
| `air75-v2-qmk-style` | `e73e42f7` (historical head at merge) | Preserved source branch; its firmware was built/flashed successfully by the user before merge. Keep as historical reference. |
| `qmk-master` | `f5db9621` | Clean official QMK snapshot from 2026-10-29; needs fast-forward to current upstream. |
| `air75-v2-qmk-port` | Documentation-only commits after `f5db9621` | Isolated migration branch; firmware port has not started. |

These are the positions recorded at the time of writing. Re-check branch heads before relying on the abbreviated SHAs.

## Current assessment

The NuPhy fork is abandoned, so continuing to treat it as the long-term upstream is not a good strategy. The migration should use official QMK master as its base and explicitly account for the changes accumulated since the fork diverged. The exact common ancestor is 260e9a546ebf (2024-06-06); the detailed audit covers the nine subsequent scheduled breaking-change releases through 2026-08-30.

The port branch began as a clean QMK starting point. A previous audit has already identified concrete compatibility issues and risks (documented below), but no Air75 port implementation or hardware validation had yet been completed.

## Work already completed in the previous baseline audit

**Important correction:** the initial version of this document understated the work already done. The previous `Clean Up QMK Baseline` investigation had completed a meaningful source/API compatibility review and identified the scheduled QMK breaking-change release windows after the 2024-06-06 merge-base through August 2026. This was an audit, not an implementation or hardware-validation pass.

### Branch and repository checks already performed

At the time of the previous audit:

- `baseline` was advanced by PR #2 to merge the tested style-cleanup branch; its recorded merge commit is `98396e9acd96a41b0603592cf17bddc9d451f68e`.
- `air75-v2-qmk-style` was at `e73e42f7`, 16 commits ahead; the user subsequently built/flashed that firmware successfully and it was merged into `baseline` through PR #2.
- `qmk-master` and `air75-v2-qmk-port` were both at `f5db9621`.
- The port branch was a clean QMK checkout with no Air75 keyboard files copied into it yet.
- No firmware files had been changed, copied, committed, pushed or merged as part of that audit.

The migration notes and compatibility audit have since been expanded in documentation-only commits. The port branch still contains no Air75 firmware implementation. Re-check live branch heads before any code changes.

### Concrete compatibility findings

| Area | Finding from the previous audit | Status |
| --- | --- | --- |
| EEPROM API | Existing EEPROM datablock calls in Air75 `ansi.c` / `side.c` use the old call shape. Current QMK expects the pointer, offset and length form: `(ptr, 0, sizeof(...))`. | Identified; not ported |
| EEPROM magic/layout | Legacy magic was `0xFEE5`; current QMK uses `0xFEE3`. The core EEPROM structure has shifted. Existing side-light settings may therefore be reset rather than safely retained across the change. | Risk identified; migration/reset policy still required |
| UART | Legacy configuration uses `SERIAL_DRIVER SD1`, `SD1_TX_PIN B6`, `SD1_RX_PIN B7` and PAL mode definitions. Current configuration needs the unified `UART_DRIVER` / `UART_TX_PIN` / `UART_RX_PIN` style, with correct driver/pin setup. | Identified; not ported |
| ChibiOS UART | USART1/HAL appeared to be a suitable direction for the Air75 UART configuration. This was a compatibility assessment, not a hardware-verified result. | Candidate; verify during port |
| GPIO | Removed GPIO helper/functions were identified as an upstream compatibility concern. | Requires file-by-file replacement review |
| RGB/keycodes | Legacy RGB and mouse-related keycodes were among the removed/deprecated keycode interfaces identified in the release audit. | Requires Air75 keymap/code mapping review |
| Keyboard protocol | The old `keyboard_protocol` interface was identified as removed. | Requires search and replacement review |
| VIA | VIA support itself was not identified as a blocker. Migration to the current community-module arrangement still needs planning and validation. | Partially assessed |
| HID | No direct HID conflict was found in the checks performed. This is not proof that all report paths are compatible. | No issue found in reviewed scope |
| RF / NKRO | User confirmed 6KRO in Mac mode and more than six simultaneous keys in Windows mode over Bluetooth using the custom keymap. Other RF/2.4 GHz report limits remain outstanding. | Partially hardware-verified |
| Bluetooth host driver | Upstream `host_driver_t` / Bluetooth host-driver changes were identified as an area requiring review. | Outstanding |

### Upstream breaking-change release windows reviewed

The previous investigation identified nine scheduled release-note windows after the 2024-06-06 merge-base:

- August and November 2024
- February, May, August and November 2025
- February, May and August 2026

The Air75-relevant change themes extracted during that review included:

- Asynchronous ChibiOS USB changes.
- I2C and LED API changes.
- Deprecated/removed Quantum keycodes.
- Unified UART configuration.
- EEPROM/NVM architecture and layout changes.
- Bluetooth `host_driver_t` changes.
- Bluetooth NKRO/reporting changes.
- Removal of `process_action_kb`.
- Removed RGB and mouse-related keycodes.
- VIA matrix-testing mitigation/workflow changes.
- Tap Dance API changes.
- Removed GPIO functions.

This list is the recorded audit summary, not a claim that every item affects the Air75 V2. Each theme must be matched against the actual Air75 source and the relevant upstream release/commit before changing code. Do not infer that a change is applicable merely because it appears in the release notes.

### Checks and source areas already inspected

The earlier investigation compared the current and legacy EEPROM interfaces, inspected UART configuration and driver definitions, and reviewed the relevant ChibiOS, LED/RGB, VIA, HID, NKRO and RF areas. The concrete Air75 call-site findings above include `ansi.c` and `side.c`. The exact command transcript was not preserved in the recovered summary, so this document deliberately records verified outcomes rather than inventing command lines.

### Corrected status

The audit is **partially completed**, not "Not started":

- [x] Confirmed branch separation and clean upstream starting point.
- [x] Identified the nine post-merge-base QMK breaking-change release windows for review.
- [x] Identified EEPROM API and data-layout/reset risk.
- [x] Identified legacy UART configuration requiring migration.
- [x] Identified GPIO, RGB/keycode and keyboard-protocol compatibility areas.
- [x] Checked VIA/HID at a preliminary level; no direct HID conflict found and VIA itself was not considered blocked.
- [ ] Complete exact upstream-to-fork ancestry and commit-range mapping.
- [ ] Match every potentially relevant release-note change to specific Air75 V2 files and symbols.
- [ ] Review RF packet lengths and NKRO/report interactions.
- [ ] Complete Bluetooth host-driver and wireless compatibility review.
- [ ] Complete ChibiOS USB/driver compatibility review.
- [ ] Implement the port, compile, and perform hardware validation.


## Priority order

### P0 — Establish the exact migration delta

- Identify the exact NuPhy fork commit/revision from which the Air75 V2 implementation originates.
- Compare that point with the current `qmk-master` snapshot.
- The full static compatibility audit is recorded in [AIR75_V2_UPSTREAM_COMPATIBILITY_AUDIT.md](AIR75_V2_UPSTREAM_COMPATIBILITY_AUDIT.md). Use that audit as the checklist; do not stop at the first compile errors.
- Separate changes that affect the Air75 V2 code from unrelated upstream changes.
- Record each finding with affected file/API, upstream change or rationale, proposed adaptation, and verification status.

**Exit condition:** a concrete Air75 V2-specific migration checklist exists, with no assumption that the first compiler pass will reveal every incompatibility.

### P1 — Resolve known compatibility blockers

1. **EEPROM / persistent configuration**
   - Audit the old EEPROM API usage and migrate to the current QMK interfaces.
   - Check the persisted data layout, structure sizes, offsets, versioning and initialisation assumptions.
   - Review EEPROM magic/version handling and reset behaviour.
   - Avoid silently interpreting old stored data using a changed layout; define safe migration or reset behaviour where needed.

2. **UART configuration**
   - Audit legacy UART configuration names and migrate to the current driver configuration, including `UART_DRIVER`, `UART_TX_PIN` and `UART_RX_PIN` where applicable.
   - Verify the selected driver and pins against the Air75 V2 hardware and current QMK conventions. Do not perform a blind text replacement.

3. **RF / wireless implementation**
   - Audit the Air75 V2 RF and wireless code against current QMK interfaces and build configuration.
   - Check timers, callbacks, transport assumptions, state handling and any changed APIs.
   - Preserve the existing 2.4 GHz / Bluetooth behaviour and status indication where supported.

4. **NKRO and report handling**
   - Review NKRO configuration, report structures and USB/HID handling for upstream changes.
   - Confirm the intended behaviour rather than treating compilation alone as proof of compatibility.

**Exit condition:** each blocker has a code change or documented resolution, with its verification status explicit.

### P2 — Complete the Air75 V2 port

- Audit all Air75 V2 keyboard files, headers, configuration, rules and keymap integration against current QMK.
- Review every relevant upstream breaking change since the NuPhy fork point, including changes not surfaced by the initial build.
- Remove obsolete fork-specific compatibility code only after confirming it is no longer required.
- Keep hardware definitions and keyboard-specific behaviour intact.
- Avoid broad cleanup mixed into compatibility fixes; separate refactors from required port changes.

### P3 — Build and validate

Validation must cover more than a successful compiler exit:

- Build the intended Air75 V2 target and custom keymap with the current QMK toolchain.
- Check warnings and configuration output; resolve or explain relevant warnings.
- Review the resulting diff for accidental changes outside the Air75 V2 scope.
- Verify USB keyboard operation, key matrix behaviour and NKRO.
- Verify wireless modes and reconnection/state behaviour where hardware is available.
- Verify RGB/underglow and side LED behaviour, including mode/state indications.
- Verify EEPROM-backed settings, persistence and reset behaviour.
- Record the exact build command, toolchain/QMK revision, result and hardware test results.

Do not mark a behaviour as tested unless it has actually been tested on hardware. Do not flash until the candidate has passed the agreed build and review checks.

### P4 — Consolidate and maintain

- Rebase/merge only after the port is understood and validated.
- Keep the migration notes current with completed work, decisions and remaining limitations.
- Once stable, document the upstream base revision and the procedure for updating from future QMK master changes.

## Immediate next actions

1. Confirm the exact NuPhy fork ancestor/revision and the current upstream base.
2. Use the completed [Air75 V2 upstream compatibility audit](AIR75_V2_UPSTREAM_COMPATIBILITY_AUDIT.md) as the file-scoped port checklist.
3. Review host-driver NKRO selection against the observed Windows/Mac Bluetooth behaviour and check remaining RF report-size limits.
4. Port incrementally on `air75-v2-qmk-port`; keep `baseline` untouched.
5. Build, review and test in stages before any firmware flash.

## Progress tracking

Use these labels in follow-up updates:

- `Not started` — no code or audit completed.
- `Investigating` — evidence is being collected; no final fix selected.
- `Implemented` — code change exists but has not completed validation.
- `Build verified` — target compiles successfully.
- `Hardware verified` — behaviour has been tested on the actual Air75 V2.
- `Blocked` — waiting on information, hardware or a decision.

At the time of the previous audit, the implementation port was **Not started**, but the compatibility audit was **partially completed**. EEPROM and UART issues were specifically identified; GPIO/keycode/protocol changes were flagged; RF/NKRO, Bluetooth host-driver and parts of the ChibiOS review remained outstanding. None of these findings should be represented as fixed or hardware-tested.

---

*This is a living engineering note, not a substitute for commit history or test evidence. Update the branch SHAs and progress labels whenever the project state changes.*


## Additional source-level inventory and hardware invariants (2026-11-08)

The following details were checked directly against the current `baseline` source tree after the initial audit. They are recorded here so the port does not accidentally lose hardware configuration or user-facing behaviour.

### Keyboard definition to preserve

The baseline keyboard definition is `keyboards/nuphy/air75_v2/ansi/keyboard.json`:

- Keyboard name/manufacturer: NuPhy Air75 V2 / NuPhy.
- MCU and bootloader: STM32F072 / `stm32-dfu`.
- USB identity: VID `0x19F5`, PID `0x3246`, device version `1.2.0`; `no_startup_check` is enabled. Preserve these unless there is a deliberate, documented reason to change them.
- Matrix: 6 rows × 17 columns, COL2ROW, 2 ms debounce. The configured row and column GPIOs are explicit in `keyboard.json`; copy them exactly and review against the physical board before changing.
- Enabled features: Bootmagic, Mouse Keys, Extra Keys, Console, NKRO, Key Lock and RGB Matrix. `command` is explicitly disabled.
- Layout: `LAYOUT_ansi_84`.
- RGB Matrix: WS2812 driver, 85 LED layout entries, centre point [80, 30], maximum brightness 128, with a large animation set. Preserve the exact LED-to-matrix map and flags; a successful build does not detect a physically incorrect LED map.
- The side strip is a separate 12-LED WS2812 chain driven on C8; it is not the 85-entry main RGB Matrix layout.

### Air75-specific GPIO and peripheral map

The current `config.h` assigns:

| Signal | Pin | Purpose |
| --- | --- | --- |
| DEV_MODE_PIN | C0 | Device/dial mode input |
| SYS_MODE_PIN | C1 | System/OS mode input |
| DC_BOOST_PIN | C2 | LED/power boost control |
| NRF_RESET_PIN | B4 | External RF module reset |
| NRF_TEST_PIN | B5 | RF module test/input |
| NRF_WAKEUP_PIN | B8 | RF module wake/transmit control |
| DRIVER_LED_CS_PIN | C6 | Main LED driver chip select/power control |
| DRIVER_SIDE_PIN | C8 | Side WS2812 data |
| DRIVER_SIDE_CS_PIN | C9 | Side LED driver chip select/power control |
| UART TX | B6 | USART1 / SD1 transmit |
| UART RX | B7 | USART1 / SD1 receive |

Treat these as hardware invariants until a board schematic or measurement proves otherwise. The code also configures USART1 serial support in `mcuconf.h` and enables HAL serial support in `halconf.h`; these files are part of the port, not incidental leftovers.

### Build and keymap inventory

- Keyboard-specific build sources in `ansi/rules.mk`: `side.c`, `rf.c`, `sleep.c`, `side_driver.c`, `rf_driver.c`.
- `UART_DRIVER_REQUIRED = yes` is currently set.
- Keymaps present in the baseline tree: `default`, `custom` and `via`. The VIA keymap enables `VIA_ENABLE = yes`; verify the actual keymap directories/files in the target branch rather than assuming all three are buildable after the port.
- The keyboard header defines custom RF/link selection, macOS shortcuts, side-light controls, device reset, sleep, battery display, RGB test and mouse-wiggle keycodes. Preserve the custom keycode namespace and check it for collisions with current QMK quantum/keycode ranges.
- `MS_WIGL` is a keyboard-specific custom keycode, not one of the removed standard `KC_MS_*` aliases. Its implementation depends on the mouse report path and must be tested over each output transport.

### RF transport invariants and review risks

The baseline RF protocol is a proprietary external-module protocol. It is not QMK's built-in Bluetooth implementation.

- UART baud rate is 460800. `rf_uart_init()` currently enables parity and changes USART1 word-length bits directly through `USART1->CR1`, then configures pull-ups on PB6/PB7. This is a hardware-sensitive area: move word length/parity into the current UART driver's supported configuration where possible, and verify the actual framing (the legacy CR1 setup is intended to provide the module's required data/parity format).
- Frames use header `0x5A`, command, acknowledgement/status, payload length, payload and checksum. The checksum is an 8-bit additive payload checksum XORed with `0x5A`. Three-byte acknowledgement-only frames are handled separately. Preserve this wire format unless the module protocol is independently documented and tested.
- Receive parsing currently requires exact data-frame length `RXDLen == RX_LEN + 5`, checks the payload checksum, and applies command-specific minimum payload checks. A community implementation reportedly tolerates trailing bytes; do not relax this check without confirming packet boundaries, buffer bounds, checksum location and real module behaviour.
- RX data is polled from QMK's UART FIFO in `housekeeping_task_kb()`; the receiver treats an idle interval as end-of-frame and waits 200 microseconds after the FIFO becomes empty. Review this against current UART buffering and interrupt behaviour.
- `UART_Send_Bytes()` drives the RF wake pin low before transmit and high after a calculated delay. The generic report sender currently sets `uart_repeat_flag`, which sends every report three times. This may be intentional for some report classes but can multiply relative mouse movement if the receiver does not deduplicate. Split retransmission policy by report type only after testing the module's semantics.
- `housekeeping_task_kb()` already calls `uart_receive_pro()`, `uart_send_report_func()` and `dev_sts_sync()`. Keep periodic RF work at the keyboard layer; do not reintroduce duplicate core-level forwarding.
- The selected `rf_host_driver` is swapped with the USB host driver according to link mode. Keep this routing model in the keyboard implementation and explicitly solve the current upstream NKRO-capability gap; do not patch upstream `host.c` with Air75-specific report calls.

### Lighting and interrupt timing

- Main RGB Matrix and the 12 side LEDs are separate paths with separate configuration/state. Side-light modes, speed, brightness, colour and sleep enable are persisted in the eight-byte `user_config_t`.
- The side LED bit-bang driver sends GRB byte order and wraps the full 12-LED transfer in `chSysLock()/chSysUnlock()`. A frame is roughly 288 data bits; at WS2812 timing this creates a few-hundred-microsecond interrupt-disabled interval. At 460800 baud, UART characters arrive on a timescale of tens of microseconds. This is a credible UART receive-latency/data-loss risk that must be measured on hardware; consider DMA/timer-driven output or another interrupt-safe implementation if drops are observed.
- The side driver uses explicit WS2812 timing constants and a reset-low interval. Preserve GRB ordering and validate pulse timing on the actual STM32F072 clock. Do not assume the old `WS2812_PWM_TARGET_PERIOD` still controls the current QMK RGB Matrix PWM driver.
- Sleep/wake controls DC boost and both LED driver control pins, sends RF sleep/handshake commands, and has a USB remote-wakeup path. Test transitions in USB, 2.4 GHz and Bluetooth modes; LED state and RF state must recover together.

### Persistent configuration policy

The eight-byte `user_config_t` currently contains a first-boot sentinel, five side-light settings, sleep enable and one reserved byte. There is no explicit schema/version field today. The first-boot sentinel is `0xA5`.

Because the upstream NVM magic/layout change can reset the core EEPROM region, do not promise seamless preservation of old firmware settings. Define and document the first-boot behaviour. Before changing the struct, decide whether to consume the reserved byte for a schema version, and ensure the configured allocation size and compile-time size assertion agree. Never read an old struct layout as though it were the new layout without a version/compatibility check.

### Current upstream base freshness

At the time of this update, official QMK master is `d194c077ed964195ec16d255c7a4b89a794cf03b` (2026-11-08). The repository's recorded `qmk-master` and initial port base are `f5db962110615fd263c3ebf9f0402e7c2b08fbe4` (2026-10-29). The intervening two upstream commits are documentation/keyboard-readme changes, but fast-forward `qmk-master` and `air75-v2-qmk-port` to the current upstream head before beginning code changes so the documented base is exact.

### Newly recorded port sequence

1. Fast-forward the clean base to current official QMK master.
2. Copy the keyboard definition and Air75 keyboard-specific source/configuration/keymaps only; retain upstream core files.
3. Get metadata, matrix, build rules and a minimal keymap compiling before touching RF behaviour.
4. Migrate GPIO, EEPROM, keycodes and UART configuration as separate, reviewable changes.
5. Resolve host-driver/NKRO capability deliberately before restoring RF report routing.
6. Validate side LED timing against UART reception, then validate RF packet/report semantics.
7. Build all intended keymaps and record USB, RF, Bluetooth, lighting, sleep/wake and persistence hardware results.

No firmware source was changed by this documentation update.

## Documentation update record (2026-11-08)

Two documentation-only updates were committed to `air75-v2-qmk-port` after the original audit:

- `aec692946e3882c1eafae5deaf16a7ead0afa64f` — expanded migration inventory, hardware pin map, keyboard definition invariants, RF framing/retry risks, side LED/UART timing risk, EEPROM policy and staged port sequence.
- `3ad4e0cdbe8f5abbc8ff53a455124b9824fc2e97` — added the corresponding source-level findings and explicit validation gates to the compatibility audit.

Neither commit changes firmware source. The port remains documentation-only.

### Test-only mergeability PR

PR #3 (`baseline` → `qmk-master`) was created solely to ask GitHub to calculate mergeability. GitHub reported it as `dirty` / not mergeable, and the diff is an unsuitable wholesale fork-to-upstream comparison. **Do not merge PR #3.** It is not the intended migration PR. Close it once its mergeability evidence is no longer needed; the intended port remains a selective Air75 keyboard-subtree migration from a clean upstream base.


## Initial firmware import status (2026-11-08)

The first source-import pass has now been committed to `air75-v2-qmk-port`. The keyboard subtree exists at `keyboards/nuphy/air75_v2/ansi/` with:

- `keyboard.json`, `config.h`, `mcuconf.h`, `halconf.h`;
- `ansi.h`, `ansi.c`, `rf.c`, `rf_driver.c`, `rf_driver.h`;
- `side.c`, `side_table.h`, `side_driver.c`, `sleep.c`, and keyboard `rules.mk`;
- `default`, `custom`, and `via` keymaps, with VIA enabled only in the VIA keymap rules.

### Compatibility edits already applied

- Migrated the legacy GPIO helper names to current `gpio_*` functions in the imported C sources.
- Migrated user EEPROM datablock reads/writes to the current pointer/offset/length signatures, preserving offset zero and the eight-byte user payload.
- Replaced old UART pin/driver macros with `UART_DRIVER`, `UART_TX_PIN`, `UART_RX_PIN` and their PAL mode settings.
- Replaced removed RGB Matrix keycodes in the imported keymaps with current `RM_*` keycodes.
- Removed the call to the no-longer-present low-level `usb_lld_wakeup_host()`; the existing public `usbWakeupHost()` and restart path remain for hardware validation.
- Removed assignments to the old global `keyboard_protocol`, which is no longer an exposed global in current QMK; USB protocol state is managed by the current USB stack.
- Selected the PWM WS2812 transport in the keyboard metadata and updated the DMA configuration macro names.

### Important current limitations

This is an initial source-import and compatibility pass. On 2026-11-08, the `default`, `custom`, and `via` keymaps all compiled successfully against the branch's checked-out QMK base after enabling ChibiOS PWM/TIM3 and removing a duplicate `ansi.c` source entry. This is **build-verified**, not a completed or hardware-verified port. No QMK lint, CI build or Air75 hardware test has been run. Do not flash until the remaining compatibility review and agreed hardware checks are complete.

The port branch is based on QMK commit `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`; current official master was observed at `d194c077ed964195ec16d255c7a4b89a794cf03b`, two upstream commits ahead. Those intervening commits are documentation changes; synchronise the branch to current upstream before finalising the port base.

### Build verification recorded (2026-11-08)

Run these builds from the QMK checkout after pulling `air75-v2-qmk-port`:

```sh
qmk compile -kb nuphy/air75_v2/ansi -km default
qmk compile -kb nuphy/air75_v2/ansi -km custom
qmk compile -kb nuphy/air75_v2/ansi -km via
```

All three commands above completed successfully from the working tree based on port commit `29e78ffd15c87908d9b9b3e8b3f4dac1c534545e` (QMK base `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`), using QMK CLI and arm-none-eabi-gcc 15.2.0. The build exposed and fixed two issues: PWM support and TIM3 were not enabled in the ChibiOS configuration, and `ansi.c` was included both automatically and through `rules.mk`.

Build output sizes: default 57,002 bytes; custom 57,786 bytes; VIA 61,314 bytes. These are compile artifacts only; lint and CI have not been run. Initial user-reported hardware checks are recorded below. Upstream was last fetched locally at `f5db9621`; the documented `d194c077` upstream head is two documentation commits ahead, so refresh/synchronise the port before treating this as the current upstream base.

### Initial Air75 V2 hardware verification (2026-11-08)

The user flashed the `custom` keymap and reported brief testing on an Air75 V2:

- USB typing works.
- Main lighting and side lighting work.
- Main and side-light settings persist after powering the keyboard off and back on.
- Bluetooth works.
- Mouse wiggler works.
- The keyboard sleeps and wakes again.

This is user-reported hardware verification, not a full regression pass. The tested Bluetooth profile(s), RF/2.4 GHz mode, and sleep/wake across each connection mode were not specified. The user confirmed that main and side-light settings persist through power cycling. They also confirmed six simultaneous keys in Mac mode and more than six in Windows mode over Bluetooth, verifying intended 6KRO/NKRO behavior in that tested configuration. Next, test RF/2.4 GHz, other Bluetooth profiles, consumer/system reports, repeated mouse movement, and extended idle/suspend and wake cycles. VIA firmware was build-verified but was not part of this hardware test.
