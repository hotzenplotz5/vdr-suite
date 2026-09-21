# Phase 56.36 - Remaining ACTIONS_SRC Audit

## Navigation

- [Development Index](index.md)
- [Phase 56.33 - Recording Action Source Group Split](phase-56.33-recording-action-source-group-split.md)
- [Phase 56.34 - Recording Action Target Migration](phase-56.34-recording-action-target-migration.md)
- [Phase 56.35 - Recording Action Core Target Migration](phase-56.35-recording-action-core-target-migration.md)

---

## Status

Completed.

---

## Purpose

This phase audits the remaining Makefile targets that still use the transitional `ACTIONS_SRC` aggregate.

No additional targets are migrated in this phase.

The remaining users are mixed targets: smoke helpers, RESTfulAPI executor tests, HTTP transport tests, REST controllers, VDR backend registry wiring, preview services and core validation tests. They need classification before another source-group migration.

---

## Already Migrated

The following targets no longer use `ACTIONS_SRC`:

- `test-recording-action-execution-result-json-serializer`
- `test-recording-action-validation-request-parser`
- `test-recording-action-execution-service-capability-safety`
- `test-recording-action-execution-service-safety`

The core-only targets use `RECORDING_ACTION_CORE_SRC`.

The REST parser target uses `RECORDING_ACTION_CORE_SRC` plus `RECORDING_ACTION_REST_PARSER_SRC`.

---

## Remaining Target Classes

| Class | Examples | Decision |
| --- | --- | --- |
| Real RESTfulAPI smoke helpers | `restfulapi-real-delete-smoke-helper`, `restfulapi-real-move-smoke-helper` | Keep for now; later use explicit RESTfulAPI executor group plus HTTP client. |
| VDR timer parser | `test-vdr-timer-action-request-parser` | Suspicious boundary; inspect separately, probably needs a VDR timer REST parser group. |
| Recording action execution controllers | `test-recording-action-execution-controller*` | Keep for now; needs controller support group with VDR backend registry and snapshot read wiring. |
| Backend executor registry tests | `test-recording-action-execution-service-registry-safety`, `test-recording-action-backend-executor-registry-capabilities` | Keep for now; mixed backend/executor boundary. |
| RESTfulAPI executor tests | `test-restfulapi-*`, executor transport, mapping and response tests | Keep for now; later use explicit `RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC`. |
| Policy and safety tests | `test-recording-action-policy-gated-execute`, `test-recording-action-backend-policy-safety`, `test-recording-action-safety-contract` | Good candidates for next targeted migration after inspection. |
| Preview service/controller tests | `test-recording-action-preview-controller`, request preview service tests | Keep for now; likely needs explicit preview group. |
| Validation/domain tests | `test-recording-action-validation-service`, `test-recording-action-validation-result-json-serializer`, `test-recording-action` | Good candidates for later core migration. Validation controller must remain REST-facing. |

---

## Future Source Group Candidates

Potential future groups:

- `RECORDING_ACTION_POLICY_SRC`
- `RECORDING_ACTION_PREVIEW_SRC`
- `RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC`
- `RECORDING_ACTION_CONTROLLER_SUPPORT_SRC`
- `VDR_TIMER_REST_PARSER_SRC`

These are candidates only. They are not introduced in this phase.

---

## Migration Order Recommendation

Recommended order after this audit:

1. Policy and safety core-like tests.
2. Validation service, validation result and domain tests.
3. Validation controller with explicit REST controller source.
4. Preview service split.
5. RESTfulAPI executor split.
6. Controller support split.
7. Real smoke helper split.
8. VDR timer parser split.

---

## Boundary Rule

Do not use `RECORDING_ACTION_CORE_SRC` for targets that require any of the following:

- `BasicHttpClient`
- `MockHttpClient`
- `RestfulApiRecordingActionExecutor`
- `RecordingActionExecutionController`
- `VdrConfig`
- `BackendRegistry`
- `VdrSnapshotReadService`
- `SearchTimerPreviewEpgCache`

Such targets need explicit mixed source groups or should remain on `ACTIONS_SRC` until their boundary is split.

---

## Phase 56 Scope Note

This audit supports package readiness and install-layout planning.

It does not start Ubuntu or Debian package building. Actual package build files, maintainer scripts and `.deb` production remain out of scope for Phase 56.

---

## Verification

Expected verification for this phase:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
