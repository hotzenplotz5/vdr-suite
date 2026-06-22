# Real Recording Action Regression Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Real VDR Regression Coverage Audit](real-vdr-regression-coverage-audit.md)

---

## Purpose

This audit reviews the existing real VDR recording-action smoke helpers.

The goal is to determine whether recording move/delete behavior is already regression-tested against a real RESTfulAPI backend or whether the helpers only validate preview and HTTP execution.

---

## Existing Helpers

| Helper | Default mode | Real execution gate | Action |
| --- | --- | --- | --- |
| restfulapi_recording_action_real_move_smoke | preview-only | --execute plus VDR_SUITE_ALLOW_REAL_RECORDING_ACTION=YES | move recording |
| restfulapi_recording_action_real_delete_smoke | preview-only | --execute plus VDR_SUITE_ALLOW_REAL_RECORDING_ACTION=YES | delete recording |

---

## Safety Findings

Both helpers already contain important safety features:

- no mutation in default mode
- explicit --execute required
- explicit environment variable required for real execution
- request preview is printed before execution
- execution stops if preview fails

These protections are good and should remain mandatory.

---

## Coverage Findings

### Move helper

Current coverage:

- builds RESTfulAPI move preview
- prints method, URL and body
- refuses execution unless safety gates are enabled
- executes the RESTfulAPI request
- checks execution result message and error list

Current gap:

- does not re-read /recordings.json after execution
- does not verify that the source recording disappeared
- does not verify that the target recording appeared
- does not verify rollback or recovery behavior
- does not require a test-recording marker in the source path

### Delete helper

Current coverage:

- builds RESTfulAPI delete preview
- prints method, URL and body
- refuses execution unless safety gates are enabled
- executes the RESTfulAPI request
- checks execution result message and error list

Current gap:

- does not re-read /recordings.json after execution
- does not verify that the source recording disappeared
- does not require a test-recording marker in the source path
- does not verify that arbitrary real recordings are protected from accidental deletion

---

## Regression Requirements

Before recording-action helpers become part of a unified real VDR regression suite, they need stronger invariants.

### Required for move

- source path must contain a strong marker such as VDR-SUITE-TEST
- target path must contain a strong marker such as VDR-SUITE-TEST
- helper must read /recordings.json before execution
- helper must verify source exists before move
- helper must execute move only after both safety gates
- helper must read /recordings.json after execution
- helper must verify old source no longer exists
- helper must verify new target exists
- helper should print a structured PASS/FAIL summary

### Required for delete

- source path must contain a strong marker such as VDR-SUITE-TEST
- helper must read /recordings.json before execution
- helper must verify source exists before delete
- helper must execute delete only after both safety gates
- helper must read /recordings.json after execution
- helper must verify source no longer exists
- helper should print a structured PASS/FAIL summary

---

## Recommended Next Phase

Phase 47.71 - Unified real VDR regression command

Scope:

- keep existing safety gates
- add marker enforcement
- add /recordings.json pre-checks
- add /recordings.json post-checks
- add structured PASS/FAIL output
- do not add these helpers to automatic real-vdr-regression until a safe test fixture strategy exists

---

## Back

- [Back to Development Index](index.md)
- [Back to Real VDR Regression Coverage Audit](real-vdr-regression-coverage-audit.md)

## Phase 47.70 Hardening Result

Phase 47.70 hardened the existing real recording action smoke helpers.

Implemented safeguards:

- mandatory VDR-SUITE-TEST marker in destructive source paths
- mandatory VDR-SUITE-TEST marker in move target paths
- /recordings.json readback before execution
- source existence verification before execution
- /recordings.json readback after execution
- source disappearance verification after delete
- source disappearance and target presence verification after move
- structured PASS/FAIL output for real recording action readbacks

The helpers remain excluded from automatic unified regression until a safe test-recording fixture strategy exists.
