# Recording Action Validation API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

The Recording Action Validation API validates recording action requests before any mutation is executed.

It is intentionally non-mutating and is the stable preflight contract for future recording action execution.

---

## Endpoints

~~text
POST /api/recordings/actions/validate
POST /api/vdr/recordings/actions/validate
~~

Both endpoints currently share the same validation behavior.

---

## Request Body

The request body is parsed into the existing `RecordingActionRequest` DTO.

The shared DTO contains:

- `backendId`
- `recordingId`
- `type`
- `dryRun`
- `parameters`

Example move validation request:

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

Example delete validation request:

~~json
{
  "backendId": "default",
  "recordingId": "recording-002",
  "action": "DELETE",
  "dryRun": true
}
~~

---

## Supported Fields

| Field | Required | Description |
| --- | --- | --- |
| `backendId` | Yes | Target backend identity. Required for multi-backend routing and future permission evaluation. |
| `recordingId` | Yes | Backend-owned recording identity. |
| `action` | Yes | Requested action. Supported values currently include `MOVE`, `RENAME`, `DELETE` and `METADATA_REFRESH`. |
| `dryRun` | No | Defaults to `true`. Validation is non-mutating regardless of this value. |
| `targetPath` | For `MOVE` | Target path for move validation. |
| `newName` | For `RENAME` | New recording name for rename validation. |
| `recordingPath` | Optional | Backend recording path, useful for future RESTfulAPI action mapping. |

---

## Response Body

Example successful move validation response:

~~json
{
  "valid": true,
  "dryRun": true,
  "wouldCreateJob": false,
  "backendId": "default",
  "recordingId": "recording-001",
  "requiredCapabilities": [
    "recordings.action.move"
  ],
  "requiredPermissions": [
    "recordings.action.move"
  ],
  "warnings": [
    "dry-run only"
  ],
  "errors": []
}
~~

Example invalid validation response:

~~json
{
  "valid": false,
  "dryRun": true,
  "wouldCreateJob": false,
  "backendId": "default",
  "recordingId": "",
  "requiredCapabilities": [
    "recordings.action.delete"
  ],
  "requiredPermissions": [
    "recordings.action.delete"
  ],
  "warnings": [
    "dry-run only"
  ],
  "errors": [
    "recordingId is required"
  ]
}
~~

---

## Semantics

Validation is a preflight operation.

It does not:

- move recordings
- rename recordings
- delete recordings
- create jobs
- call a VDR backend
- call RESTfulAPI

It does:

- parse the action request
- validate required fields
- validate action-specific parameters
- expose required capabilities
- expose required permissions
- return warnings and errors
- keep multi-backend identity explicit

---

## RESTfulAPI Compatibility Strategy

The validation contract is intentionally REST-first and backend-neutral.

If future execution requires information or operations that RESTfulAPI does not yet expose, RESTfulAPI should be extended first whenever possible.

VDR-Suite should avoid backend-specific shortcuts and keep action execution capability-driven.

---

## LIVE Superset Strategy

The VDR LIVE plugin remains the functional reference for user-visible recording operations.

VDR-Suite extends that model with:

- dry-run validation
- capability preview
- permission preview
- explicit backend identity
- frontend-independent REST contracts
- future multi-backend action routing

This makes validation a LIVE-superset capability rather than a direct clone of LIVE behavior.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Architecture Index](index.md)
