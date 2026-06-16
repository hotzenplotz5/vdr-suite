# ADR-0024: Recording Action Transport Mapping

## Status

Accepted

## Context

VDR-Suite exposes recording actions through its own domain API.

The public VDR-Suite action model must remain stable even when different backend transports expose different action names, request bodies, error models, or safety behavior.

The LIVE plugin is the functional reference for user-visible recording action behavior.

LIVE handles rename, move, and copy through a recording update operation. It updates recording metadata and then changes the VDR recording name/path through VDR recording APIs. LIVE delete handling is more advanced than a simple file delete because it handles active local and remote timers before deleting recordings.

The current RESTfulAPI plugin transport exposes a different low-level HTTP contract:

- move/copy through `POST /recordings/move`
- move/copy body fields: `source`, `target`, `copy_only`
- delete through `POST /recordings/delete` or `DELETE /recordings/delete`
- delete body field: `file`

VDR-Suite already has an internal action model:

- `RecordingActionRequest`
- `RecordingActionValidationService`
- `RecordingActionJobPayload`
- `RecordingActionExecutionService`
- `RecordingActionBackendExecutorAdapterRegistry`
- `IRecordingActionBackendExecutorAdapter`
- `RecordingActionExecutionResult`

## Decision

VDR-Suite keeps its own stable recording action API and does not expose RESTfulAPI transport details as the public domain contract.

The action API uses domain terms:

- `backendId`
- `recordingId`
- `recordingPath`
- `action`
- `targetPath`
- `newName`
- `dryRun`

Backend adapters translate this domain model to backend-specific transports.

For RESTfulAPI backends:

| VDR-Suite action | RESTfulAPI endpoint | Method | Transport body |
| --- | --- | --- | --- |
| `MOVE` | `/recordings/move` | `POST` | `source`, `target`, `copy_only=false` |
| `RENAME` | `/recordings/move` | `POST` | `source`, `target`, `copy_only=false` |
| `DELETE` | `/recordings/delete` | `POST` preferred | `file` |
| `COPY` when introduced | `/recordings/move` | `POST` | `source`, `target`, `copy_only=true` |

The VDR-Suite REST API continues to use `action` rather than the internal field name `type` for request bodies.

The internal execution result may serialize the action as `type` until the public HTTP contract is explicitly stabilized in a later API documentation phase.

## Consequences

VDR-Suite can support multiple backend transports without changing the public API.

RESTfulAPI-specific fields such as `source`, `target`, `copy_only`, and `file` stay inside the RESTfulAPI backend adapter.

LIVE remains the functional reference for expected user behavior, especially around rename/move semantics and safe delete behavior.

Before real delete execution is enabled against a live VDR, VDR-Suite must provide an explicit safety path:

1. validation
2. dry-run execution
3. read-only backend permission checks
4. test recording requirement
5. real delete only after explicit operator confirmation

## Follow-up

Phase 36.x should add adapter-level contract tests for RESTfulAPI mapping.

A later implementation phase should verify the behavior against a real VDR with dry-run first.
