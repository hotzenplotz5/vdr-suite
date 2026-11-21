# Phase 56.40 - RESTfulAPI Executor Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)
- [Phase 56.39 - Recording Action Preview Source Group Split](phase-56.39-recording-action-preview-source-group.md)

---

## Status

Completed.

---

## Purpose

Phase 56.40 introduces a dedicated source group for RESTfulAPI recording-action executor tests.

This removes another focused set of targets from the transitional `ACTIONS_SRC` aggregate while keeping HTTP test support and transport dependencies explicit.

---

## New Source Group

The following source group was added to `mk/action-job-sources.mk`:

```make
RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC := \
        $(RECORDING_ACTION_CORE_SRC) \
        core/recordings/src/RestfulApiRecordingActionExecutor.cpp
```

The group includes the recording-action core sources plus the concrete RESTfulAPI executor implementation.

---

## Migrated Targets

The following targets now use `RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC` instead of `ACTIONS_SRC`:

- `test-preview-execution-restfulapi-request-equivalence`
- `test-execution-service-restfulapi-executor-integration`
- `test-restfulapi-executor-http-result-mapping`
- `test-restfulapi-executor-http-transport-contract`

Each target keeps `core/http/src/MockHttpClient.cpp` explicit.

---

## Intentionally Not Migrated

The following target classes remain unchanged:

- BasicHttpClient live/error contracts
- socket smoke targets
- real-client readonly gate
- REST controller execution targets
- backend registry and capability targets
- mapping-only contract targets that do not compile `RestfulApiRecordingActionExecutor.cpp`

Reason:

These targets have different boundary requirements and should not be hidden behind the RESTfulAPI executor source group until their package role is explicit.

---

## Boundary Result

The RESTfulAPI executor tests now separate:

- core recording-action behavior via `RECORDING_ACTION_CORE_SRC`
- concrete executor behavior via `RestfulApiRecordingActionExecutor.cpp`
- test HTTP transport via explicit `MockHttpClient.cpp`

This avoids pulling unrelated parser or VDR cache sources through `ACTIONS_SRC`.

---

## Remaining ACTIONS_SRC Work

Remaining users still include:

- BasicHttpClient live/error targets
- real smoke helpers
- REST controller execution targets
- VDR timer parser target
- mapping and request contract targets
- mixed legacy recording-action domain/RESTfulAPI coverage

These require additional source groups or test separation before migration.

---

## Verification

Local verification used for this phase:

```bash
make test-preview-execution-restfulapi-request-equivalence
make test-execution-service-restfulapi-executor-integration
make test-restfulapi-executor-http-result-mapping
make test-restfulapi-executor-http-transport-contract
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
