# Phase 56.39 - Recording Action Preview Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)
- [Phase 56.38 - Recording Action Validation Target Migration](phase-56.38-recording-action-validation-target-migration.md)

---

## Status

Completed.

---

## Purpose

Phase 56.39 introduces a dedicated source group for recording-action preview targets.

This avoids using the broad `ACTIONS_SRC` compatibility aggregate for preview tests and keeps the preview boundary explicit.

---

## New Source Group

The following source group was added to `mk/action-job-sources.mk`:

```make
RECORDING_ACTION_PREVIEW_SRC := \
        $(RECORDING_ACTION_CORE_SRC)
```

The group currently contains the recording-action core sources because the preview service is header-only and depends on core validation and payload construction behavior.

---

## Migrated Targets

The following targets now use `RECORDING_ACTION_PREVIEW_SRC` instead of `ACTIONS_SRC`:

- `test-recording-action-preview-controller`
- `test-recording-action-request-preview-service-json-contract`
- `test-recording-action-request-preview-service`

The controller target also explicitly adds:

```make
$(RECORDING_ACTION_REST_PARSER_SRC)
```

This keeps the REST request-parser dependency visible instead of hiding it inside `ACTIONS_SRC`.

---

## Boundary Result

Preview-related tests no longer pull in unrelated VDR cache sources through `ACTIONS_SRC`.

The target dependencies now distinguish between:

- preview/core behavior via `RECORDING_ACTION_PREVIEW_SRC`
- REST request parsing via `RECORDING_ACTION_REST_PARSER_SRC`
- explicit preview JSON serialization source
- explicit REST preview controller source

---

## Remaining ACTIONS_SRC Work

Remaining users still include:

- RESTfulAPI executor targets
- REST controller execution targets
- real smoke helpers
- VDR timer parser target
- mixed legacy recording-action domain/RESTfulAPI coverage

These require dedicated source groups or further test separation before migration.

---

## Verification

Local verification used for this phase:

```bash
make test-recording-action-preview-controller
make test-recording-action-request-preview-service-json-contract
make test-recording-action-request-preview-service
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
