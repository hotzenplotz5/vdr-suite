# Phase 56.38 - Recording Action Validation Target Migration

## Navigation

- [Development Index](index.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)
- [Phase 56.37 - Recording Action Policy Target Migration](phase-56.37-recording-action-policy-target-migration.md)

---

## Status

Completed.

---

## Purpose

Phase 56.38 migrates the next safe group of recording-action Makefile targets away from the transitional `ACTIONS_SRC` aggregate.

This phase only touches validation targets that are recording-action core targets.

---

## Migrated Targets

The following targets now use `RECORDING_ACTION_CORE_SRC` instead of `ACTIONS_SRC`:

- `test-recording-action-validation-service`
- `test-recording-action-validation-result-json-serializer`

---

## Intentionally Not Migrated

The following targets were intentionally left unchanged:

- `test-recording-action-validation-controller`
- `test-recording-action`

Reason:

- `test-recording-action-validation-controller` is REST-facing and should not be classified as core-only.
- `test-recording-action` still covers RESTfulAPI backend executor and HTTP-related behavior, so it is not a pure recording-action domain target.

---

## Boundary Result

The validation service and validation result serializer tests now compile against the recording-action core source group.

This keeps the target dependencies aligned with the future recording-action core package boundary.

---

## Remaining ACTIONS_SRC Work

Remaining users still include:

- RESTfulAPI executor targets
- REST controller targets
- preview service/controller targets
- real smoke helpers
- VDR timer parser target
- mixed legacy recording-action domain/RESTfulAPI coverage

These require dedicated source groups or test separation before migration.

---

## Verification

Local verification used for this phase:

```bash
make test-recording-action-validation-service
make test-recording-action-validation-result-json-serializer
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
