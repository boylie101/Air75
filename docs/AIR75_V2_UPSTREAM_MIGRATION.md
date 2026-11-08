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
| `baseline` | `3ceeacff` | Protected known-good working baseline. |
| `air75-v2-qmk-style` | `e73e42f7` | Formatting/style cleanup; awaiting a local build. |
| `qmk-master` | `f5db9621` | Clean official QMK master snapshot. |
| `air75-v2-qmk-port` | `f5db9621` | Migration work branch; Air75 porting work has not yet started. |

These are the positions recorded at the time of writing. Re-check branch heads before relying on the abbreviated SHAs.

## Current assessment

The NuPhy fork is abandoned, so continuing to treat it as the long-term upstream is not a good strategy. The migration should use official QMK master as its base and explicitly account for the changes accumulated since the fork diverged.

The current port branch is still a clean QMK starting point. The compatibility items below are **known areas to investigate**, not claims that fixes have already been implemented or validated.

## Priority order

### P0 — Establish the exact migration delta

- Identify the exact NuPhy fork commit/revision from which the Air75 V2 implementation originates.
- Compare that point with the current `qmk-master` snapshot.
- Build a chronological inventory of relevant QMK breaking changes and API migrations across the full divergence period; do not stop at the first compile errors.
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
2. Produce a full, file-scoped divergence and breaking-change audit.
3. Start with EEPROM compatibility and UART configuration, while keeping RF/NKRO as explicit parallel audit areas.
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

At the time of this snapshot, the upstream port itself is **Not started**. EEPROM, UART, RF/wireless and NKRO are identified audit/compatibility priorities; they must not be represented as fixed or tested yet.

---

*This is a living engineering note, not a substitute for commit history or test evidence. Update the branch SHAs and progress labels whenever the project state changes.*
