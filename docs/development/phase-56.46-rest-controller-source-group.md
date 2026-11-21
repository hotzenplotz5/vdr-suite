# Phase 56.46 - Recording Action REST Controller Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.45 - Remaining ACTIONS_SRC Re-Audit](phase-56.45-remaining-actions-src-reaudit.md)

---

## Status

Completed on branch `phase-56-rest-controller-source-group`.

---

## Purpose

Phase 56.46 removes the REST recording-action controller targets from the transitional `ACTIONS_SRC` aggregate.

The affected targets now compile through an explicit REST controller support source group instead of pulling parser and VDR cache sources indirectly through the broad compatibility aggregate.

---

## Source Group

The new source group is:

```make
RECORDING_ACTION_REST_CONTROLLER_SRC := \
        $(RECORDING_ACTION_CORE_SRC) \
        $(RECORDING_ACTION_REST_PARSER_SRC)
```

This keeps the controller boundary explicit:

- recording-action core services and serializers come from `RECORDING_ACTION_CORE_SRC`
- request parsing comes from `RECORDING_ACTION_REST_PARSER_SRC`
- VDR backend registry and snapshot read dependencies remain explicit per target
- HTTP mocks remain explicit only where a test needs them

---

## Migrated Targets

The following targets now use `RECORDING_ACTION_REST_CONTROLLER_SRC`:

- `test-recording-action-execution-controller-safety-preview`
- `test-recording-action-execution-controller-execute-body-policy-gate`
- `test-recording-action-execution-controller-policy-execute`
- `test-recording-action-execution-controller-policy-safety`
- `test-recording-action-execution-controller`
- `test-recording-action-validation-controller`

---

## Boundary Result

Before this phase these targets depended on:

```make
$(ACTIONS_SRC)
```

After this phase they depend on:

```make
$(RECORDING_ACTION_REST_CONTROLLER_SRC)
```

This removes accidental dependency on `RECORDING_ACTION_VDR_CACHE_SRC` from REST controller tests.

---

## Verification

Local verification targets:

```bash
make test-recording-action-execution-controller-safety-preview
make test-recording-action-execution-controller-execute-body-policy-gate
make test-recording-action-execution-controller-policy-execute
make test-recording-action-execution-controller-policy-safety
make test-recording-action-execution-controller
make test-recording-action-validation-controller
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
