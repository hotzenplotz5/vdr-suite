# Phase 56.41-56.44 - ACTIONS_SRC Batch Migration

## Navigation

- [Development Index](index.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)
- [Phase 56.40 - RESTfulAPI Executor Source Group Split](phase-56.40-restfulapi-executor-source-group.md)

---

## Status

Completed as a batch on branch `phase-56-actions-src-batch-2`.

---

## Purpose

Phases 56.41 through 56.44 continue reducing use of the transitional `ACTIONS_SRC` aggregate in the Makefile.

The batch keeps related RESTfulAPI recording-action targets together so the branch can be validated with one local test pass and one GitHub Actions run before merging to `main`.

---

## Phase 56.41 - RESTfulAPI Live Executor Targets

The following targets now use `RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC` while keeping `core/http/src/BasicHttpClient.cpp` explicit:

- `restfulapi-real-delete-smoke-helper`
- `restfulapi-real-move-smoke-helper`
- `test-restfulapi-rename-live-error-contract`
- `test-restfulapi-delete-live-error-contract`
- `test-restfulapi-executor-basic-http-client-socket-smoke`

Boundary result:

- concrete executor behavior comes from `RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC`
- live HTTP transport remains explicit through `BasicHttpClient.cpp`
- unrelated parser and VDR cache sources are no longer pulled through `ACTIONS_SRC`

---

## Phase 56.42 - RESTfulAPI Request Contract Targets

The following request-builder contract targets no longer use `ACTIONS_SRC`:

- `test-restfulapi-upstream-action-endpoint-contract`
- `test-restfulapi-move-tilde-mapping-regression`
- `test-restfulapi-action-request-preview-contract`
- `test-restfulapi-recording-action-empty-basepath-contract`
- `test-restfulapi-recording-action-mapping-contract`

Reason:

These tests compile header-only RESTfulAPI request-builder behavior and do not require the recording-action implementation aggregate.

---

## Phase 56.43 - RESTfulAPI Dispatch Contract Targets

The following dispatch and safety-gate contract targets now use `RECORDING_ACTION_CORE_SRC`:

- `test-restfulapi-execution-dispatch-allowed`
- `test-restfulapi-execution-gate-blocks-dispatch`

Reason:

These tests validate execution-service dispatch and policy gating with fake executor adapters. They do not require REST parser sources, VDR cache sources or concrete RESTfulAPI executor compilation.

---

## Phase 56.44 - Legacy RESTfulAPI Executor Adapter Targets

The following legacy `RestfulApiRecordingActionBackendExecutorAdapter` targets now use `RECORDING_ACTION_CORE_SRC` while keeping `core/http/src/MockHttpClient.cpp` explicit:

- `test-restfulapi-recording-action-executor-transport-smoke`
- `test-restfulapi-recording-action-executor-response-contract`

Reason:

These tests cover the header-only legacy adapter path and mock HTTP transport. They do not compile `RestfulApiRecordingActionExecutor.cpp`.

---

## Remaining ACTIONS_SRC Work

After this batch, remaining `ACTIONS_SRC` users are expected to be limited to mixed or not-yet-classified targets, including:

- REST controller execution targets
- backend registry and capability targets
- VDR timer parser target
- broad legacy `test-recording-action`
- real or mixed smoke helpers not yet split by package role

These should be handled in later focused phases or in a final remaining-ACTIONS audit.

---

## Verification

Local verification for the batch used the target-specific tests for each migrated group plus:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

Before merge to `main`, the branch should also be validated by GitHub Actions once for the full batch.

---

## Back

- [Back to Development Index](index.md)
