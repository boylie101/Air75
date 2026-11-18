# C — Conditional state-clearing regression coverage

Commit: `bb7c2efb7b` — tests only; no production reset-helper change.

The reproduced conditional sequence was:

1. Hold an RF NKRO overflow key.
2. Change the simulated USB protocol to BOOT.
3. Release and repress the overflow key.
4. Restore REPORT protocol, clear reports, then run periodic refresh.

Without the [transport capability override](2026-11-18-transport-nkro-capability.md),
the sequence could replay a stale E2 bit. With the override, RF consistently
uses its selected QMK report representation and the sequence clears correctly.

The regression tests exercise actual QMK insertion/deletion, report generation,
host dispatch and clear-function bodies with simulated protocol and UART state.
Four clearing checks pass, including the controlled failure when the override
is disabled, ordinary RF→USB→RF switching and clear-before-mode-toggle paths.

```sh
python3 util/air75_rf_audit.py --stage transport --analyze
```

The earlier broad claim that ordinary switching or `clear_keyboard()` generally
failed was withdrawn: a synthetic mixed-callback test had not established that.
An active flag refreshing an empty bitmap is harmless by itself. No standalone
cache-reset helper was justified by the reproduced normal path.

Before committing, the complete native suite with static analysis/sanitizers,
formatting/Python lint, applicable QMK lint, 81 QMK tests (five existing skips)
and all four firmware builds passed. The conditional BOOT/REPORT sequence has
not been reproduced on physical hardware; host-side success is not that proof.

[Change index](../AIR75_V2_NKRO_AUDIT.md)
