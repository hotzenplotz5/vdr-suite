# Recording Action Safety and Capability Model

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

The Recording Action Safety and Capability Model defines when a recording action may be executed.

It separates three concerns:

1. action validation
2. backend capability support
3. runtime safety blockers

This model is intentionally backend-neutral.

It is used before real mutation against a VDR backend.

---

## Main Types

| Type | Purpose |
| --- | --- |
| `RecordingActionCapabilityContract` | Maps actions to required capability strings. |
| `RecordingActionCapabilitySet` | Holds backend-supported capability strings. |
| `RecordingActionCapabilityCheckResult` | Describes whether a backend supports an action. |
| `RecordingActionSafetyContext` | Holds runtime safety inputs. |
| `RecordingActionSafetyResult` | Describes whether execution may proceed. |
| `RecordingActionSafetyService` | Evaluates safety with or without capabilities. |
| `RecordingActionSafetyResultJsonSerializer` | Serializes safety results for later API exposure. |
| `RecordingActionExecutionService::evaluateSafety()` | Exposes safety checks through the execution service boundary. |

---

## Capability Strings

| Action | Capability |
| --- | --- |
| `CHECK` | `recording.action.check` |
| `REPAIR` | `recording.action.repair` |
| `SHRINK` | `recording.action.shrink` |
| `CUT` | `recording.action.cut` |
| `PES2TS` | `recording.action.pes2ts` |
| `RENAME` | `recording.action.rename` |
| `MOVE` | `recording.action.move` |
| `DELETE` | `recording.action.delete` |
| `METADATA_REFRESH` | `recording.action.metadata-refresh` |
| `UNKNOWN` | unsupported |

Capability strings are stable domain values.

They are not RESTfulAPI endpoint names.

They are not LIVE handler names.

They are not VDR core method names.

---

## Reference Capability Sets

### RESTfulAPI Default

The default RESTfulAPI action capability set currently contains:

- `recording.action.move`
- `recording.action.rename`
- `recording.action.delete`

### LIVE Reference

The LIVE reference capability set currently contains:

- `recording.action.cut`
- `recording.action.rename`
- `recording.action.move`
- `recording.action.delete`
- `recording.action.metadata-refresh`

LIVE remains the functional reference for expected user-facing behavior.

---

## Safety Context

| Field | Meaning |
| --- | --- |
| `dryRun` | Request is a dry-run and must not mutate backend state. |
| `backendAvailable` | Backend adapter or backend service is available. |
| `backendReadOnly` | Backend is configured read-only. |
| `executionAllowed` | Real execution is explicitly enabled. |
| `capabilityAvailable` | Backend capability check has passed. |
| `recordingInUse` | Recording is currently in use. |
| `actionSupported` | Backend supports the action in principle. |

---

## Safety Result

| Field | Meaning |
| --- | --- |
| `canExecute` | No blockers are present. |
| `dryRun` | Mirrors the dry-run input. |
| `readOnlyBlocked` | Backend read-only config blocks mutation. |
| `executionDisabled` | Real execution is disabled. |
| `backendUnavailable` | Backend cannot be reached or resolved. |
| `recordingInUse` | Recording is currently in use. |
| `missingCapability` | Backend lacks the required capability. |
| `unsupportedAction` | Action is unsupported. |
| `blockers` | Human-readable execution blockers. |
| `warnings` | Non-blocking warnings such as dry-run notices. |

---

## Evaluation Flow

Capability-aware safety evaluation follows this flow:

- `RecordingActionType`
- `RecordingActionCapabilityContract`
- required capability
- `RecordingActionCapabilitySet`
- capability check
- `RecordingActionSafetyContext`
- `RecordingActionSafetyService`
- `RecordingActionSafetyResult`

The execution service exposes this through:

- `RecordingActionExecutionService::evaluateSafety(action, context)`
- `RecordingActionExecutionService::evaluateSafety(action, context, capabilitySet)`

---

## Safety Rules

Real mutation must be blocked when any of these conditions is true:

- backend is unavailable
- backend is read-only
- backend does not expose the required action capability
- requested action is unsupported
- recording is in use
- request is not dry-run and real execution is disabled

Dry-run requests may be allowed without real execution permission.

Dry-run requests must not mutate backend state.

---

## Real VDR Implications

The VDR core model makes this safety layer necessary.

Important implications:

- delete is not equivalent to immediate filesystem deletion
- rename and move follow VDR recording-name semantics
- recordings may be in use by replay, timers, move/copy/cut jobs, or other backend operations
- real delete must remain the last action enabled in live testing

---

## Multi-Backend Implications

The safety and capability model is required for multi-backend support.

Example:

| Backend | Capabilities | Read-only |
| --- | --- | --- |
| Backend A | move, rename, delete | false |
| Backend B | move, rename, delete | true |

In this case the UI may show that Backend B supports delete in principle, but execution is blocked by read-only policy.

Capability answers: can this backend do it?

Safety answers: may this request execute now?

---

## Policy-Gated Execution

Recording action execution now uses the same backend policy chain as safety preview.

The shared chain is:

1. `BackendRegistry`
2. `RecordingActionBackendPolicyProvider`
3. `RecordingActionBackendPolicyMapper`
4. `RecordingActionBackendPolicy`
5. `RecordingActionExecutionService::evaluateSafety(request, policy)`
6. `RecordingActionExecutionService::execute(request, registry, policy)`

This means the following paths must remain behaviorally aligned:

| Path | Requirement |
| --- | --- |
| `RecordingActionExecutionController::safety()` | Returns policy-derived safety JSON. |
| `RecordingActionExecutionController::execute()` | Blocks execution when policy safety is not executable. |
| `RecordingActionExecutionController::executeBody()` | Parses the body and then uses the same policy-gated execute path. |

No controller entry point may bypass backend policy evaluation when a backend registry is available.

The legacy registry-only execution path remains available for tests and non-policy callers, but real backend mutation should use the policy-aware controller construction.

---

## Execute Body Contract

`executeBody()` delegates to `RecordingActionValidationRequestParser`.

The request body uses domain-level fields:

| JSON field | Meaning |
| --- | --- |
| `action` | Recording action name, for example `DELETE`. |
| `recordingId` | VDR-Suite recording identifier. |
| `backendId` | Backend selected for the action. |
| `dryRun` | Whether the request is a dry-run. |

`executeBody()` must not interpret `type` as the action field.

This keeps body parsing aligned with the validation request parser and avoids transport-specific naming drift.

---

## API Exposure

The model is prepared for later HTTP API exposure.

A future endpoint may return the serialized safety result before execution.

Possible future endpoint:

`POST /api/recordings/actions/safety`

This endpoint is not required for execution yet.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Architecture Index](index.md)
