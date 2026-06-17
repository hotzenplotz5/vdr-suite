# Recording Action Real Backend Smoke Test Plan

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document defines the first safe real-backend smoke-test plan for recording actions.

The goal is not to mutate a real VDR yet.

The goal is to prove that the complete policy-gated execution chain is ready before real mutation is enabled.

---

## Current Execution Chain

1. HTTP/API body or domain request
2. RecordingActionValidationRequestParser
3. RecordingActionExecutionController
4. BackendRegistry
5. RecordingActionBackendPolicyProvider
6. RecordingActionBackendPolicyMapper
7. RecordingActionExecutionService::evaluateSafety(request, policy)
8. RecordingActionExecutionService::execute(request, registry, policy)
9. RecordingActionBackendExecutorAdapterRegistry
10. backend executor adapter
11. RESTfulAPI transport request

The policy-aware path is mandatory for real backend tests.

---

## Safety Preconditions

| Condition | Required value |
| --- | --- |
| Backend registered | yes |
| Backend online | yes |
| Backend type | restfulapi |
| Backend policy provider active | yes |
| Safety preview endpoint/path | green |
| Execute path | policy-gated |
| Execute body path | policy-gated |
| Backend executor config readOnly | true for first real smoke test |
| Backend executor config allowExecution | false for first real smoke test |

The first real smoke test must not allow mutation.

---

## Forbidden In Phase 40.0

Phase 40.0 must not:

- delete a real recording
- move a real recording
- rename a real recording
- enable allowExecution
- disable readOnly
- bypass RecordingActionBackendPolicyProvider
- bypass RecordingActionExecutionService::evaluateSafety(request, policy)

---

## Smoke-Test Order

1. Verify backend registry entry exists.
2. Verify backend policy resolves from registry.
3. Verify safety preview for a destructive action returns blocked on read-only policy.
4. Verify execute() for the same action returns success=false.
5. Verify executeBody() for the same action returns success=false.
6. Verify no backend adapter call is dispatched when policy blocks execution.
7. Verify RESTfulAPI executor can still build requests in isolated mock tests.
8. Only after all blocked-path tests are green, prepare a dry-run-style real transport test.

---

## First Real Backend Candidate

The first real backend candidate should be a read-only RESTfulAPI backend.

| Field | Value |
| --- | --- |
| backendId | local test backend |
| backendType | restfulapi-readonly or equivalent registry mapping |
| enabled | true |
| online | true |
| readOnly | true |
| executionAllowed | false |

This backend may expose capabilities, but safety must block mutation.

---

## Expected Result for Blocked Delete

A delete request against a read-only backend must return an execution failure.

Expected result shape:

    {
      "success": false,
      "message": "recording action execution blocked by safety policy",
      "errors": [
        "recording action execution is blocked by read-only backend config",
        "recording action permission is denied",
        "missing permission: recording.permission.delete"
      ]
    }

---

## Expected Result for Missing Backend

A delete request against an unknown backend must return an execution failure.

Expected result shape:

    {
      "success": false,
      "message": "recording action execution blocked by safety policy",
      "errors": [
        "recording action backend is unavailable"
      ]
    }

---

## Verified Real RESTfulAPI Read Endpoint Result

A manual smoke test against the local yaVDR RESTfulAPI backend verified that the service is reachable through `BasicHttpClient`.

Observed local endpoint behavior:

| Path | Result |
| --- | --- |
| `/api/recordings.json` | HTTP 404 |
| `/api/channels.json` | HTTP 404 |
| `/api/timers.json` | HTTP 404 |
| `/api/events.json` | HTTP 404 |
| `/recordings.json` | HTTP 200 |
| `/channels.json` | HTTP 200 |
| `/timers.json` | HTTP 200 |
| `/events.json` | HTTP 200 |

Therefore the real local RESTfulAPI backend uses an empty base path.

Required real-backend config for this yaVDR system:

| Field | Value |
| --- | --- |
| host | `127.0.0.1` |
| port | `8002` |
| basePath | empty string |
| readOnly | `true` for first mutation-adjacent tests |
| allowExecution | `false` for first mutation-adjacent tests |

This result is also covered by the empty-base-path request-builder contract.

---

## Transition to Phase 40.1

Phase 40.1 may add an executable local smoke-test helper.

That helper should still default to:

- readOnly=true
- allowExecution=false
- no mutation
- visible request/response output
- explicit backend ID
- explicit action

Mutation must remain opt-in and must not be the default behavior.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Architecture Index](index.md)
