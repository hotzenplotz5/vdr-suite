# ADR-0068 checkpoint: first router extraction (2026-10-21)

Baseline: `main` at `62b52c00ee713b268b5ccd13df19409bef28a036`; R1/R2/R3 productive hardware acceptance is closed and is not repeated.

## Exact extracted unit

- `ApiRouter::handlePublicRecordingAction` moved byte-for-byte from `api/rest/src/ApiRouter.cpp` to `api/rest/src/ApiRouterPublicRecordingActions.cpp`. It is still one **private method on the same `ApiRouter` instance**, with the same single write mutex and existing resolver callbacks, controllers, provider preflight and backend executor.
- `ApiRouter::handlePost` still owns the same two Public-v1 recording dispatch paths and all original legacy `/api/recordings/actions/*` and `/api/vdr/recordings/actions/*` dispatch paths. No new router, security gate, repository or mutation service was created.
- Both `REST_ROUTER_SRC` (router tests) and `DAEMON_SRC` (daemon binary) compile the extracted source directly, without including a CPP file.

## Recording mutation route / behavior inventory

| Method | Request path | Handler / controller | Wire contract & gates |
| --- | --- | --- | --- |
| POST | `/api/v1/recordings/actions/validate` | `ApiRouter::handlePost` → `handlePublicRecordingAction(execute=false)` | Strict public `rec_` input, backend-scoped resolver, logical `fld1_` MOVE destination; validation response + optional provider ETag; public Problem Details; security admission remains in `SecurityHttpGate` |
| POST | `/api/v1/recordings/actions/execute` | `ApiRouter::handlePost` → `handlePublicRecordingAction(execute=true)` | Dry-run or validated write; strong If-Match, re-resolve and authoritative provider preflight/readback; provider errors fail closed, no blind retry; public no-store JSON/Problem Details |
| POST | `/api/recordings/actions/validate`, `/api/vdr/recordings/actions/validate` | Original `RecordingActionValidationController::validateBody` | Legacy request/response unchanged |
| POST | `/api/recordings/actions/execute`, `/api/vdr/recordings/actions/execute` | Original `RecordingActionExecutionController::executeBody` | Legacy execution unchanged |
| POST | `/api/recordings/actions/preview`, `/api/vdr/recordings/actions/preview` | Original `RecordingActionPreviewController::previewBody` | Read-only legacy preview unchanged |

Production authentication, CSRF and grants are still owned by `core/security/include/SecurityHttpGate.h` and the existing HTTP server; no security logic is relocated. Request/correlation IDs and status/error construction stay in the exact relocated method. The validation and execution tests under `api/rest/tests/test_api_router.cpp` protect Public-v1 URL query parsing, no-store headers and failure shapes alongside the existing dry-run, MOVE/DELETE, spoofing, revision, identity and legacy regression cases. See `api/recording-actions.md` and `docs/adr/ADR-0068-public-api-router-modularization-checkpoint.md` for the broader contract.

## Gate and remaining checkpoint

This is **increment 1**, not an assertion that the full ADR-0068 checkpoint is complete. Before extracting `PublicApiRuntime::tryHandleGet` / `tryHandlePost` / `tryHandleDelete`, inventory and freeze their domain branches, discovery and error fallthrough, capability availability and device/grant mappings; retain all direct standalone test-build source lists. The full router/Runtime split still requires separate coherent Public-v1 domain extractions and exact-head CI review. No production host action is part of this increment.
