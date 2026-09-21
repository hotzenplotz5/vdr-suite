# Phase 56.47 - Recording Action Executor Adapter Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.46 - Recording Action REST Controller Source Group Split](phase-56.46-rest-controller-source-group.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)

---

## Status

Completed on branch `phase-56-executor-adapter-source-group`.

---

## Purpose

Phase 56.47 removes another set of recording-action targets from the transitional `ACTIONS_SRC` aggregate.

The migrated targets exercise backend executor adapter registration, capability exposure, RESTfulAPI executor capability reporting, HTTP error preservation and read-only real-client execution blocking.

The split keeps the packaging boundary clearer by separating adapter-level tests from the broader action aggregate and by reusing the already explicit RESTfulAPI executor source group where an executor implementation is required.

---

## Source Group Added

`mk/action-job-sources.mk` now defines:

```make
RECORDING_ACTION_EXECUTOR_ADAPTER_SRC := \
        $(RECORDING_ACTION_CORE_SRC)
```

This group is intentionally minimal.

It contains the recording-action core implementation required by adapter-facing tests, while keeping REST parser sources and VDR cache sources out of these targets.

---

## Migrated Targets

The following targets moved from `$(ACTIONS_SRC)` to `$(RECORDING_ACTION_EXECUTOR_ADAPTER_SRC)`:

```text
test-recording-action-execution-service-registry-safety
test-recording-action-backend-executor-registry-capabilities
test-restfulapi-recording-action-executor-capabilities
test-real-client-readonly-recording-action-executor-gate
```

The following target moved from `$(ACTIONS_SRC)` plus an explicit `RestfulApiRecordingActionExecutor.cpp` to the dedicated RESTfulAPI executor group:

```text
test-restfulapi-executor-preserves-http-error-status
```

It now uses:

```make
$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC)
```

---

## Boundary Result

This phase removes five more `ACTIONS_SRC` users.

The migrated adapter targets no longer pull in:

```text
$(RECORDING_ACTION_REST_PARSER_SRC)
$(RECORDING_ACTION_VDR_CACHE_SRC)
```

The RESTfulAPI HTTP error preservation target now uses the existing executor-specific source group instead of duplicating the executor source next to the broad aggregate.

This keeps the source graph closer to the future package layout documented in ADR-0037:

```text
core modules remain internal build boundaries
api/rest remains the REST-facing layer
runtime and test-only helpers stay separable
```

---

## Verification

The following target checks were run locally before committing:

```bash
make test-recording-action-execution-service-registry-safety
make test-recording-action-backend-executor-registry-capabilities
make test-restfulapi-recording-action-executor-capabilities
make test-restfulapi-executor-preserves-http-error-status
make test-real-client-readonly-recording-action-executor-gate
```

The following documentation and phase checks were also run:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Follow-Up

Remaining `ACTIONS_SRC` work after this phase:

```text
VDR timer parser target
legacy/core recording action targets
final ACTIONS_SRC removal audit
```

The install and packaging contract work remains tracked by ADR-0037 and later Phase 56 install-layout follow-ups.

---

## Back

- [Back to Development Index](index.md)
