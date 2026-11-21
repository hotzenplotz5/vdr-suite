# Recording Action Execution API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

The Recording Action Execution API executes validated recording actions through the VDR-Suite recording action execution stack.

It is the mutating counterpart to the non-mutating Recording Action Validation API.

Execution remains backend-neutral at the public API boundary.

Backend-specific transport details are handled by backend executor adapters.

---

## Endpoints

~~text
POST /api/recordings/actions/execute
POST /api/vdr/recordings/actions/execute
~~

Both endpoints currently share the same execution behavior.

---

## Request Body

The request body is parsed into the existing `RecordingActionRequest` DTO.

Public HTTP request bodies use the field name `action`.

The internal C++ domain object may still use the field name `type`.

Example dry-run move execution request:

~~json
{
  "backendId": "default",
  "recordingId": "recording-001",
  "action": "MOVE",
  "dryRun": true,
  "targetPath": "/srv/vdr/video/archive",
  "recordingPath": "Movies/Tatort/2026-06-16.20.15.1-0.rec"
}
~~

Example dry-run delete execution request:

~~json
{
  "backendId": "default",
  "recordingId": "recording-002",
  "action": "DELETE",
  "dryRun": true,
  "recordingPath": "Movies/Tatort/2026-06-16.20.15.1-0.rec"
}
~~

---

## Supported Fields

| Field | Required | Description |
| --- | --- | --- |
| `backendId` | Yes | Target backend identity. |
| `recordingId` | Yes | Backend-owned recording identity. |
| `action` | Yes | Requested action. Supported values currently include `MOVE`, `RENAME`, `DELETE` and `METADATA_REFRESH`. |
| `dryRun` | No | Defaults to `true`. Real mutation must only be enabled intentionally. |
| `targetPath` | For `MOVE` | Target path for move execution. |
| `newName` | For `RENAME` | New recording name for rename execution. |
| `recordingPath` | Backend dependent | Backend recording path. Required by RESTfulAPI transport mapping for real execution. |

---

## Response Body

The response body is serialized from `RecordingActionExecutionResult`.

Example dry-run move response:

~~json
{
  "success": false,
  "type": "MOVE",
  "backendId": "default",
  "recordingId": "recording-001",
  "message": "dry-run backend execution skipped",
  "warnings": [
    "dry-run only"
  ],
  "errors": []
}
~~

Example successful delete response:

~~json
{
  "success": true,
  "type": "DELETE",
  "backendId": "default",
  "recordingId": "recording-002",
  "message": "backend execution completed",
  "warnings": [],
  "errors": []
}
~~

---

## Execution Stack

Execution follows this internal chain:

~~text
HTTP POST body
  -> RecordingActionValidationRequestParser
  -> RecordingActionExecutionController
  -> RecordingActionExecutionService
  -> RecordingActionValidationService
  -> RecordingActionJobPayloadFactory
  -> RecordingActionBackendExecutorAdapterRegistry
  -> RecordingActionBackendExecutorAdapterDispatchService
  -> IRecordingActionBackendExecutorAdapter
  -> RecordingActionExecutionResult
  -> RecordingActionExecutionResultJsonSerializer
~~

---

## Semantics

Execution always validates before dispatch.

Invalid requests are not dispatched to a backend adapter.

Dry-run requests are execution requests but must not mutate backend state.

Backend adapters must preserve the VDR-Suite public action contract and translate to backend-specific transports internally.

---

## RESTfulAPI Transport Mapping

The RESTfulAPI mapping is defined by ADR-0024.

For RESTfulAPI backends:

| VDR-Suite action | RESTfulAPI endpoint | Method | Transport body |
| --- | --- | --- | --- |
| `MOVE` | `/recordings/move` | `POST` | `source`, `target`, `copy_only=false` |
| `RENAME` | `/recordings/move` | `POST` | `source`, `target`, `copy_only=false` |
| `DELETE` | `/recordings/delete` | `POST` preferred | `file` |

RESTfulAPI-specific fields must not leak into the public VDR-Suite API.

---

## Safety Requirements

Real execution against a live VDR must follow a safety sequence:

1. validation
2. dry-run execution
3. backend permission checks
4. dedicated test recording
5. real mutation only after explicit operator confirmation

Delete is the most dangerous action and must be tested last.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Architecture Index](index.md)
