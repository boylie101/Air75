# Air75 V2 → upstream QMK compatibility audit

**Audit date:** 2026-11-08  
**Scope:** NuPhy Air75 V2 ANSI implementation, including default/custom/VIA keymaps, its RF/UART transport, side lighting, EEPROM settings, and keyboard-specific USB sleep handling.  
**Repository:** [boylie101/Air75](https://github.com/boylie101/Air75)

## Executive result

The static source and upstream-change audit is complete. The port is **not ready to compile unchanged** against current QMK master. There are several definite source incompatibilities, plus one major RF/NKRO architecture issue that needs a deliberate design before implementation.

No firmware source was changed during this audit. No build or hardware test was run.

### Definite source blockers

1. The old one-argument EEPROM datablock calls must become the current pointer/offset/length API.
2. keyboard_protocol is no longer a public global; the Air75 RF code still writes it.
3. Deprecated GPIO helpers were removed in QMK 0.32; the Air75 code uses them throughout.
4. The default, custom and VIA keymaps still use deprecated RGB_* keycodes removed in QMK 0.30.

### Major architecture issue

The NuPhy fork carries a local patch to QMK core tmk_core/protocol/host.c. Do not copy that patched core file into the upstream port. Its UART forwarding calls are partly redundant and three have mismatched function signatures. The keyboard already has an RF host_driver_t, and housekeeping_task_kb() already calls the periodic UART report refresh.

The remaining hard problem is NKRO selection: current upstream asks host_can_send_nkro(), which bases its decision on USB protocol or QMK's built-in Bluetooth connection. It does not know that the Air75 has selected its external NuPhy RF/Bluetooth module as a custom host driver. The old keyboard_protocol = 1 assignments were being used to force report/NKRO mode. Replacing them blindly with usb_device_state_set_protocol() would conflate the USB host's protocol with the external radio's capability. The port needs a clean host-driver capability hook or another explicit integration.

---

## 1. Provenance and comparison range

| Item | Verified revision/date | Meaning |
| --- | --- | --- |
| NuPhy fork revision used by the Air75 baseline | f1856912d603 — 2025-10-13 | NuPhy fork snapshot containing the Air75 V2 implementation |
| Git merge-base with official QMK | 260e9a546ebf — 2024-06-06 | Exact common ancestor for the upstream comparison |
| Existing qmk-master branch | f5db96211061 — 2026-10-29 | Clean official QMK snapshot used to create the port branch |
| Official QMK master at audit time | d194c077ed96 — 2026-11-08 | Current upstream master |

The NuPhy fork snapshot contains **24 commits not present in official QMK**. The upstream snapshot at f5db9621 is substantially newer than the fork. The official QMK master has since advanced by two commits to d194c077; those two commits are documentation/keyboard-readme changes, not firmware-core changes. The migration branch should nevertheless be fast-forwarded to the current official master before the code port begins.

The common ancestor is 6 June 2024. Therefore the relevant scheduled QMK breaking-change releases are **nine**, beginning in August 2024. February and May 2024 predate the merge-base and are not part of the post-divergence release window. The earlier migration note's count of ten was incorrect; it also omitted November 2024.

The NuPhy fork history includes Air75 V2 changes after the merge-base, including the December 2024 fixes and March 2025 Air75 RGB/keyboard fixes. These fork-specific keyboard changes are part of the code that must be preserved while replacing upstream core assumptions.

---

## 2. Nine QMK breaking-change windows reviewed

| Release | Air75 relevance | Audit result |
| --- | --- | --- |
| [2024-08-25 — 0.26](https://docs.qmk.fm/ChangeLog/20240825) | EEPROM driver-format abstraction; data-driven serial configuration; general driver/config cleanup | No direct new API call found in Air75 from this release. Do not confuse the data-driven SERIAL_DRIVER keyboard configuration migration with the Air75's ChibiOS UART driver aliases. The current UART header separately retains deprecated aliases for the old names. |
| [2024-11-24 — 0.27](https://docs.qmk.fm/ChangeLog/20241124) | RGB keycode overhaul; USB state centralisation; WS2812 API and RGB/HSV type changes; extended mouse wheel reports | Directly relevant. The global keyboard_protocol was moved into usb_device_state; old RGB keycodes entered a deprecation period; Air75 side-LED code uses the deprecated rgb_led_t alias. |
| [2025-02-23 — 0.28](https://docs.qmk.fm/ChangeLog/20250223) | UART headers unified; WS2812 base code moved under drivers/led; ChibiOS/ChibiOS-Contrib updated | Air75 already includes uart.h, so the unified header itself is not a blocker. Legacy UART config names remain deprecated. WS2812 DMA names in the Air75 config are compatibility aliases; one old PWM timing define is no longer consumed. |
| [2025-05-25 — 0.29](https://docs.qmk.fm/ChangeLog/20250525) | Non-volatile-memory repository pattern; Bluetooth bound to host_driver_t; NKRO/connection changes | Directly relevant. The EEPROM datablock API changed to pointer/offset/length. Host/NKRO decisions now use the current host/connection model rather than the old global USB protocol variable. |
| [2025-08-31 — 0.30](https://docs.qmk.fm/ChangeLog/20250831) | Deprecated RGB and mouse keycodes removed; process_action_kb removed | Directly relevant to old RGB keycodes in all three Air75 keymaps. No process_action_kb implementation or removed standard mouse keycode use was found in the Air75 source. |
| [2025-11-30 — 0.31](https://docs.qmk.fm/ChangeLog/20251130) | Tap Dance memory, debounce, mod-tap and I2C changes | No direct Air75 use of the affected Tap Dance or custom debounce APIs was found. The keyboard uses data-driven debounce configuration. No QMK I2C API calls were found in the Air75 V2 implementation. |
| [2026-02-22 — 0.32](https://docs.qmk.fm/ChangeLog/20260222) | Deprecated GPIO defines removed | Direct compile blocker. Air75 code uses the removed Arduino-style GPIO helpers in ansi.c, rf.c, side.c, side_driver.c and sleep.c. |
| [2026-05-31 — 0.33](https://docs.qmk.fm/ChangeLog/20260531) | FORCE_NKRO / usb.force_nkro deprecation and new host.default.nkro setting | No FORCE_NKRO definition was found. keyboard.json enables the NKRO feature; the Air75 also changes keymap_config.nkro from its physical OS switch. This is not itself a blocker, but the external-radio NKRO capability issue remains. |
| [2026-08-30 — 0.34](https://docs.qmk.fm/ChangeLog/20260830) | Automatic datablock initialisation; VIA feature migration; System Control HID usage range; community-module persistence | Datablock init is relevant and the current API now handles invalid datablocks consistently. VIA is a future migration concern, not an immediate blocker. Air75's System Control usage 0x009B is within the new default safe range, so no range override is required. |

The next scheduled breaking-change merge was 29 November 2026 and had not happened at the audit date.

---

## 3. Definite code changes required

### 3.1 EEPROM / NVM datablock

**Affected files:** ansi/config.h, ansi/ansi.c, ansi/side.c.

The Air75 declares EECONFIG_USER_DATA_SIZE 8 and stores an eight-byte user_config_t. The structure is eight bytes, so the allocation size is still appropriate.

The fork uses the old one-argument calls:

- eeconfig_read_user_datablock(&user_config)
- eeconfig_update_user_datablock(&user_config)

Current QMK requires:

- eeconfig_read_user_datablock(&user_config, 0, sizeof(user_config))
- eeconfig_update_user_datablock(&user_config, 0, sizeof(user_config))

Update every call in ansi.c and side.c, not just the initial load/save pair.

The EEPROM magic changed from 0xFEE5 in the NuPhy baseline to 0xFEE3 in current QMK's NVM implementation. Existing firmware's magic value will not be accepted by the new core, so the first boot will reinitialise QMK's persistent storage. This means **all persisted QMK state may reset**, not only side-light settings. The existing default_brightness_flag == 0xA5 check provides a path to initialise the Air75 side-light defaults after that reset.

Current QMK automatically handles invalid datablock initialisation (PR #26282). Keep the Air75's own sentinel/default logic, but define an explicit EECONFIG_USER_DATA_VERSION and bump it deliberately if the structure's meaning or layout changes. Add a compile-time size check that sizeof(user_config_t) matches the configured eight-byte allocation.

### 3.2 Removed GPIO helpers

**Affected files and replacements:**

| Existing helper | Current QMK helper |
| --- | --- |
| setPinOutput(pin) | gpio_set_pin_output(pin) |
| setPinInput(pin) | gpio_set_pin_input(pin) |
| setPinInputHigh(pin) | gpio_set_pin_input_high(pin) |
| writePinHigh(pin) | gpio_write_pin_high(pin) |
| writePinLow(pin) | gpio_write_pin_low(pin) |
| readPin(pin) | gpio_read_pin(pin) |

The old helpers occur in ansi.c, rf.c, side.c, side_driver.c and sleep.c. Migrate all call sites. This is a confirmed current-master incompatibility, not merely a style recommendation.

### 3.3 Removed RGB keycodes

**Affected files:** keymaps/default/keymap.c, keymaps/custom/keymap.c, keymaps/via/keymap.c.

These keymaps still use old RGB_* names. Air75 enables RGB Matrix, so use the RM_* names:

| Old keycode | RGB Matrix replacement |
| --- | --- |
| RGB_SPD | RM_SPDD |
| RGB_SPI | RM_SPDU |
| RGB_VAI | RM_VALU |
| RGB_MOD | RM_NEXT |
| RGB_VAD | RM_VALD |
| RGB_HUI | RM_HUEU |

No old standard KC_MS_* mouse keycodes were found in these keymaps. MS_WIGL is an Air75 custom keycode; it uses mousekey_get_report() and host_mouse_send(), not a removed standard mouse-key alias.

### 3.4 Removed keyboard_protocol global

**Affected files:** ansi/rf.c, ansi/rf_driver.c.

The old code assigns keyboard_protocol = 1 in the periodic RF sender and the RF host-driver keyboard/NKRO callbacks. Current QMK removed the global from host.h and centralised USB protocol state in usb_device_state. Current report selection calls host_can_send_nkro().

This is a compile blocker, but **do not mechanically replace every assignment with usb_device_state_set_protocol(USB_PROTOCOL_REPORT)**. The Air75's RF/Bluetooth module is a separate output transport. Its ability to accept an NKRO report is not the same thing as the USB host's selected boot/report protocol. See the host/NKRO architecture issue below.

---

## 4. NuPhy-specific QMK core patch — do not carry it forward

**Affected baseline file:** tmk_core/protocol/host.c.

The baseline's host.c is not clean upstream QMK. It adds:

- unconditional calls into Air75 UART report functions;
- outputselect.h / where_to_send() Bluetooth output selection;
- direct calls to bluetooth_send_keyboard(), bluetooth_send_mouse() and bluetooth_send_consumer().

Current upstream has a different host/connection architecture. The Air75 keyboard already defines rf_host_driver in ansi/rf_driver.c and selects it with host_set_driver(). Keep the radio transport in the keyboard driver rather than copying the NuPhy-modified core file.

There is also a concrete ABI defect in the fork patch:

- baseline host.c declares and calls uart_send_mouse_report(), uart_send_consumer_report() and uart_send_system_report() with **no arguments**;
- the actual functions in rf.c require report_mouse_t * or report_extra_t *.

Those calls therefore do not match the callee signatures and can pass an invalid pointer. The RF host-driver callbacks already call the functions with the correct report pointer. Remove the no-argument core hooks; do not reproduce them in the port.

The baseline core patch also calls uart_send_report_func() from host_keyboard_send(). That periodic resend is already called by the keyboard's own housekeeping_task_kb() in ansi.c, so carrying the core hook is unnecessary.

### Required NKRO integration decision

Current upstream host_can_send_nkro() checks USB report protocol, or the built-in Bluetooth connection's NKRO capability when CONNECTION_ENABLE is active. It does not automatically infer that the custom Air75 RF host driver supports NKRO.

The Air75's rf_host_driver has a send_nkro callback, but the current host-driver interface does not expose a matching "can send NKRO" capability callback. Before porting, choose one of:

1. add a small, explicit host-driver capability hook in the port (best architectural fit; consider proposing it upstream); or
2. use a documented, tested connection integration for the external radio.

Do not change the USB protocol state merely to force the external radio into NKRO mode.

---

## 5. UART / ChibiOS review

**Affected files:** ansi/config.h, ansi/halconf.h, ansi/mcuconf.h, ansi/rules.mk, ansi/rf.c.

The current uart.h still provides compatibility aliases for the old configuration names, but marks them deprecated. Migrate:

- SERIAL_DRIVER → UART_DRIVER
- SD1_TX_PIN → UART_TX_PIN
- SD1_RX_PIN → UART_RX_PIN
- SD1_TX_PAL_MODE → UART_TX_PAL_MODE
- SD1_RX_PAL_MODE → UART_RX_PAL_MODE

The intended values remain SD1, PB6/PB7, and PAL mode 0. UART_DRIVER_REQUIRED = yes remains a supported build option. HAL_USE_SERIAL TRUE and STM32_SERIAL_USE_USART1 TRUE are still part of the required ChibiOS serial setup.

rf.c includes the unified uart.h; the basic uart_init, uart_transmit, uart_read and uart_available APIs remain available. Remove redundant local prototypes where the header already declares the function.

The higher-risk part is rf_uart_init(): it modifies USART1->CR1 and GPIOB->PUPDR directly after uart_init(). Current QMK's ChibiOS UART driver builds a SerialConfig from UART_CR1, UART_CR2 and UART_CR3 before calling sdStart(). Prefer configuring the required 9-bit-plus-parity/even-parity mode through UART_CR1 and retaining only hardware-specific pull-up setup that cannot be expressed through the UART config. Validate the exact STM32F072/USART1 pin configuration on hardware.

---

## 6. WS2812 and lighting review

**Affected files:** ansi/config.h, ansi/side.c, ansi/side_driver.c, plus the RGB Matrix configuration in ansi/keyboard.json.

- The old WS2812 base header moved from drivers/ws2812.h to drivers/led/ws2812.h in QMK. The current header still exposes timing constants used by the custom side-light bit-bang routine.
- The old rgb_led_t type is now a deprecated alias in quantum/color.h; it still exists, so this is not currently a compile blocker. Prefer the current rgb_t type for generic RGB values or ws2812_led_t for a byte-ordered WS2812 buffer.
- WS2812_DMA_STREAM and WS2812_DMA_CHANNEL are still mapped by the current driver, but are deprecated. Rename them to WS2812_PWM_DMA_STREAM and WS2812_PWM_DMA_CHANNEL.
- WS2812_PWM_TARGET_PERIOD is not referenced by the current PWM driver and therefore no longer controls the timer period. Current PWM timing is derived from WS2812_TIMING / WS2812_PWM_FREQUENCY and WS2812_PWM_TICK_FREQUENCY. Revalidate the existing 850 ns / 700 ns / 150 ns timing values rather than assuming the old target-period define still applies.
- The public RGB Matrix functions used by the Air75 (rgb_matrix_set_color, rgb_matrix_sethsv, rgb_matrix_set_speed, row/column mapping and PWM buffer update) still exist in current QMK.

The custom side LED routine manually sends GRB bytes; do not replace it with the generic WS2812 API without checking the side-strip timing and ordering.

---

## 7. USB sleep/wake review

**Affected file:** ansi/sleep.c.

The sleep code directly reads USB_DRIVER.state and USB_DRIVER.status, checks USB_SUSPENDED / remote-wakeup enable, calls usb_lld_wakeup_host(), then calls usbWakeupHost() and restart_usb_driver().

Current ChibiOS still has the low-level usb_lld_wakeup_host macro, but documents it as an internal implementation detail; QMK's own ChibiOS suspend path uses the public usbWakeupHost() API. Prefer the public API and avoid double-triggering wakeup. The current QMK USB implementation also has asynchronous endpoint/event handling, so the explicit driver restart path needs a careful suspend/resume hardware test. This is a high-risk runtime review item, not a confirmed compile blocker.

---

## 8. RF report-size limitation

This is an existing transport constraint, not a new QMK API break.

Current QMK defines NKRO_REPORT_BITS as 30 bytes. The Air75 RF code processes NKRO_REPORT_BITS + 1 bytes (modifier byte plus bitmap), but sends only 16 bytes for CMD_RPT_BIT_KB. Consequently the radio receives the modifier byte plus only the first 15 bitmap bytes (120 keycode positions); bitmap positions above that are not transmitted.

The normal Air75 keymaps inspected use ordinary keycodes that appear to fit this range, but VIA lets users assign other keycodes. Do not expand the UART packet length without confirming the proprietary radio module's expected frame format. Record this as a hardware-protocol limit and test the highest keycodes that the supported keymaps/VIA are expected to use.

---

## 9. Reviewed areas with no immediate blocker

- **process_action_kb:** no Air75 implementation found; the removal does not require a keyboard-side replacement.
- **isLeftHand:** no Air75 use found.
- **FORCE_NKRO:** no define found. features.nkro in keyboard.json enables NKRO support; it is not the deprecated force-NKRO default.
- **HID System Control range:** Air75 sends 0x009B for its DND system usage, which is within the new default allowed range. No override is needed for this value.
- **I2C:** no direct QMK I2C API use found in the Air75 V2 source.
- **Tap Dance/custom debounce:** no affected custom API use found.
- **VIA:** keymaps/via/rules.mk uses VIA_ENABLE = yes and the repository contains a VIA v3 definition. This remains usable for local development today. QMK's VIA migration is a forward-looking change; keep the local keymap isolated and be prepared to move VIA integration to the VIA team's module/userspace arrangement. No VIA_EEPROM_MAGIC_ADDR override was found.

---

## 10. Prioritised implementation checklist

### P0 — unblock the source port

- [ ] Fast-forward the clean upstream base from f5db9621 to current official QMK master d194c077 (two non-code commits).
- [ ] Copy only the Air75 V2 keyboard implementation and required keymaps; do not copy the NuPhy-modified core host.c.
- [ ] Migrate all EEPROM datablock calls to pointer/offset/length.
- [ ] Replace all removed GPIO helpers.
- [ ] Replace the old RGB keycodes in default/custom/VIA keymaps with RM_*.
- [ ] Remove keyboard_protocol references and implement explicit RF-aware NKRO capability.
- [ ] Migrate legacy UART config names and configure parity through the current UART configuration.
- [ ] Remove or replace the stale WS2812_PWM_TARGET_PERIOD assumption and update deprecated WS2812 DMA names.

### P1 — preserve and validate behaviour

- [ ] Keep periodic RF state refresh in housekeeping_task_kb(); do not duplicate it in core.
- [ ] Verify 6KRO and NKRO reports through USB, 2.4 GHz and all three Bluetooth profiles.
- [ ] Verify RF packet limits against the actual radio module protocol, including high keycodes and simultaneous key presses.
- [ ] Verify mouse, consumer and system reports through the external radio.
- [ ] Verify USB suspend, remote wakeup and resume with the current endpoint/event implementation.
- [ ] Verify main RGB Matrix and side WS2812 lighting timings, order, brightness and sleep behaviour.
- [ ] Verify EEPROM defaults, persistence, version invalidation and first-boot reset behaviour.
- [ ] Build all supported targets/keymaps with the exact upstream revision and record the results.

### P2 — future maintenance

- [ ] Plan the VIA migration before QMK removes the compatibility path.
- [ ] Remove deprecated type/config aliases after the port builds and hardware behaviour is confirmed.
- [ ] Record the upstream revision and repeatable update procedure.

## Audit boundary

This is a completed **static source/API and release-history audit**. It identifies definite source blockers and unresolved design/test items. It is not a successful build report, and no item should be labelled build-verified or hardware-verified until those checks have actually been run on an Air75 V2.


---

## 11. Branch merge and integration audit

Compared `baseline` and `qmk-master` against common ancestor `260e9a546ebf` (2024-06-06).

- `baseline`: 47 commits ahead of the common ancestor.
- `qmk-master`: 2,213 commits ahead of the common ancestor.
- `air75-v2-qmk-port`: based on `qmk-master`, with migration/audit documentation commits.

A recursive tree comparison found two files changed on both sides:

1. **`tmk_core/protocol/host.c` — high risk.** NuPhy added RF/UART forwarding into core host-send functions. Upstream independently changed those same paths for active host-driver selection, Bluetooth routing, NKRO capability and extra reports. The NuPhy hooks should not be carried forward: they duplicate keyboard-level RF forwarding and include report calls whose declarations do not match the actual pointer-taking functions. Keep upstream `host.c` and integrate RF through the Air75 keyboard host driver.
2. **`quantum/rgb_matrix/rgb_matrix_drivers.c` — overlapping file, apparently separate hunks.** NuPhy added IS31FL3763 registration; upstream refactored WS2812 handling. This looks less likely to produce a textual conflict, but the IS31FL3763 registration/API must be checked against current RGB Matrix architecture. Do not copy this core file wholesale.

The Air75 V2 keyboard subtree is fork-side content not independently changed by upstream in this ancestry comparison. That lowers same-path merge risk for those files, but does not remove the API compatibility work identified elsewhere in this audit.

### Integration recommendation

- Do not merge `baseline` wholesale into `qmk-master`.
- Keep official QMK as the port base and port the Air75 keyboard subtree deliberately.
- Retain upstream `tmk_core/protocol/host.c`.
- Investigate whether the IS31FL3763 registration is required, and port only the necessary driver integration.
- Resolve compile errors, then validate hardware behaviour.

### Verification boundary

This is a branch-ancestry and three-way changed-path/source-overlap audit. **No actual Git merge, rebase, merge-tree test, or temporary merge PR was executed.** Therefore it identifies likely conflict areas and integration hazards, but does not claim a definitive Git mergeability result. No production branch was merged or rewritten.


---

## 12. GitHub pull-request mergeability test (2026-11-08)

A temporary **draft-only, do-not-merge** pull request was created to ask GitHub to calculate mergeability:

- PR: [#3 — TEST ONLY: baseline → qmk-master](https://github.com/boylie101/Air75/pull/3)
- Base branch: `qmk-master`, base SHA `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`
- Head branch: `baseline`, head SHA `98396e9acd96a41b0603592cf17bddc9d451f68e`
- GitHub result: `mergeable=false`, `mergeable_state=dirty`
- PR comparison reported 64 commits, 23,705 changed files, 4,771 additions and 1,562,183 deletions.

**Conclusion:** GitHub confirms that a wholesale merge of `baseline` into `qmk-master` is not cleanly mergeable. The enormous diff also confirms that this is not a sensible way to port the Air75: it proposes a very large amount of fork/upstream tree replacement, not just the Air75 keyboard changes.

The PR files endpoint exposes only paginated portions of this unusually large diff; the sampled pages primarily show removals of existing upstream keyboard files. The PR mergeability response does not provide a definitive list of every conflicting path, so do not interpret `dirty` as a complete conflict report. The earlier source-overlap analysis still identifies `tmk_core/protocol/host.c` and `quantum/rgb_matrix/rgb_matrix_drivers.c` as important integration hotspots.

**Action:** Do not merge PR #3. It is a test artifact, not a proposed integration. The intended strategy remains a clean upstream base plus a selective Air75 V2 port. No production branch was changed by this test.
