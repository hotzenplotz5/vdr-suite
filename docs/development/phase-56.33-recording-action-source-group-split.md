# Phase 56.33 - Recording Action Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.30 - Package Candidate Source Inventory](phase-56.30-package-candidate-source-inventory.md)
- [Phase 56.32 - Package Dependency Graph](phase-56.32-package-dependency-graph.md)

---

## Status

Completed.

---

## Purpose

Phase 56.33 performs the first concrete source-group cleanup from the Phase 56 package inventory.

The previous `ACTIONS_SRC` aggregate mixed recording-action core code, a REST request parser and a VDR SearchTimer preview cache dependency. That made the future package boundary unclear.

This phase splits the aggregate into named subgroups while preserving the existing `ACTIONS_SRC` compatibility aggregate so current tests and smoke helpers continue to build.

---

## Changed Source Groups

New source groups:

```make
RECORDING_ACTION_CORE_SRC
RECORDING_ACTION_REST_PARSER_SRC
RECORDING_ACTION_VDR_CACHE_SRC
```

Transitional aggregate kept:

```make
ACTIONS_SRC
```

---

## New Group Ownership

### Recording Action Core

```make
RECORDING_ACTION_CORE_SRC := \
        core/recordings/src/RecordingActionUtils.cpp \
        core/recordings/src/RecordingActionValidationResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionExecutionResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionValidationService.cpp
```

Classification:

```text
recording action core / package candidate after mutation policy audit
```

This group contains reusable recording-action validation and result serialization code.

### REST Parser

```make
RECORDING_ACTION_REST_PARSER_SRC := \
        api/rest/src/RecordingActionValidationRequestParser.cpp
```

Classification:

```text
REST implementation
```

This file belongs to the REST layer and must not be part of a future recording-action core package.

### VDR Cache Dependency

```make
RECORDING_ACTION_VDR_CACHE_SRC := \
        core/vdr/src/SearchTimerPreviewEpgCache.cpp
```

Classification:

```text
VDR/SearchTimer cache dependency
```

This file belongs to VDR/SearchTimer preview infrastructure and must not be hidden inside a recording-action package.

---

## Compatibility Aggregate

`ACTIONS_SRC` remains as a compatibility aggregate:

```make
ACTIONS_SRC := \
        $(RECORDING_ACTION_CORE_SRC) \
        $(RECORDING_ACTION_REST_PARSER_SRC) \
        $(RECORDING_ACTION_VDR_CACHE_SRC)
```

This prevents unrelated build churn in existing tests and smoke helpers.

Later phases may migrate individual targets from `ACTIONS_SRC` to the narrower source groups.

---

## Boundary Result

Before:

```text
ACTIONS_SRC
  -> recording action core
  -> REST request parser
  -> VDR SearchTimer preview cache
```

After:

```text
RECORDING_ACTION_CORE_SRC
RECORDING_ACTION_REST_PARSER_SRC
RECORDING_ACTION_VDR_CACHE_SRC

ACTIONS_SRC = transitional aggregate
```

---

## Packaging Impact

This phase does not create a package, but it makes these future package candidates possible:

| Package candidate | Source group impact |
| --- | --- |
| `vdr-suite-recording-action-validation` | Can use `RECORDING_ACTION_CORE_SRC` without pretending the REST parser is core. |
| `vdr-suite-rest-implementation` | Can own `RECORDING_ACTION_REST_PARSER_SRC`. |
| `vdr-suite-searchtimer-readonly` or VDR preview package | Can own `RECORDING_ACTION_VDR_CACHE_SRC` after VDR split. |

---

## Follow-up

Next cleanup options:

1. Migrate tests that only need serializers to `RECORDING_ACTION_CORE_SRC`.
2. Migrate REST parser tests to `RECORDING_ACTION_CORE_SRC` plus `RECORDING_ACTION_REST_PARSER_SRC`.
3. Remove VDR cache dependency from targets that do not need it.
4. Continue with VDR source-group subdivision from Phase 56.22.

---

## Verification

Local verification used for this phase:

```bash
make test-recording-action-execution-result-json-serializer
make test-recording-action-validation-request-parser
make test-recording-action-execution-controller-safety-preview
make test-recording-action-execution-service-registry-safety
make test-recording-action-backend-executor-registry-capabilities
make test-restfulapi-recording-action-executor-capabilities
make test-restfulapi-executor-preserves-http-error-status
make test-recording-action-execution-service-capability-safety
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
