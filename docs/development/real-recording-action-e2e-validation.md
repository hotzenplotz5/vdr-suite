# Real Recording Action End-to-End Validation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)

---

## Purpose

This document records the real VDR and RESTfulAPI validation results for destructive and mutating recording actions executed through the VDR-Suite daemon.

The validation was performed against a running VDR-Suite daemon, a real RESTfulAPI backend and real VDR recordings.

This document is a validation note. Current phase markers remain in the status and phase tracking documents.

---

## Validated Runtime Chain

The tested action path was:

```text
VDR-Suite HTTP API
  -> RecordingActionExecutionController
  -> RecordingActionExecutionService
  -> RESTfulAPI recording action backend executor
  -> RESTfulAPI plugin
  -> VDR recording domain
```

The real daemon was reachable on:

```text
127.0.0.1:18080
```

The real RESTfulAPI backend was reachable on:

```text
127.0.0.1:8002
```

---

## Native Recording Identity Finding

RESTfulAPI recording actions require the native absolute recording path as their action identity.

The correct native identifier is exposed by VDR-Suite as:

```text
backendNativeId
```

Example:

```text
/srv/vdr/video/Oskar/Spongebob_Pirat/2026-04-24.18.33.1-0.rec
```

The following values are not sufficient as RESTfulAPI recording action identity:

```text
recordingId
recordingPath
relative_file_name
```

For VDR/RESTfulAPI, `backendNativeId` maps to RESTfulAPI `file_name` / VDR `recording->FileName()`.

---

## Delete Validation

Delete was first validated directly against RESTfulAPI with the absolute `file` value and then through the VDR-Suite execution endpoint.

VDR-Suite endpoint:

```text
POST /api/vdr/recordings/actions/execute
```

Action payload shape:

```json
{
  "backendId": "default",
  "recordingId": "531",
  "action": "DELETE",
  "dryRun": false,
  "backendNativeId": "/srv/vdr/video/Oskar/Flipper/2026-05-13.10.10.1-0.rec"
}
```

Observed result:

```json
{
  "success": true,
  "type": "DELETE",
  "backendId": "default",
  "recordingId": "531",
  "message": "restfulapi backend executor request accepted",
  "warnings": [],
  "errors": []
}
```

A follow-up recording query no longer returned the deleted recording.

---

## Rename Validation

Rename was validated through the VDR-Suite execution endpoint with `backendNativeId` as the RESTfulAPI source identity.

Initial action:

```text
Oskar/Spongebob Pirat
  -> Oskar/Spongebob Pirat RENAME TEST
```

Follow-up query confirmed:

```text
title: Oskar/Spongebob Pirat RENAME TEST
backendNativeId: /srv/vdr/video/Oskar/Spongebob_Pirat_RENAME_TEST/2026-04-24.18.33.1-0.rec
```

The recording was then renamed back successfully:

```text
Oskar/Spongebob Pirat RENAME TEST
  -> Oskar/Spongebob Pirat
```

Follow-up query confirmed the restored title and native path.

---

## Move Validation

Move was validated through the VDR-Suite execution endpoint with `targetPath` and `backendNativeId`.

Initial observation showed that RESTfulAPI treats `target` as a complete target recording name, not merely a destination folder.

The request builder was adjusted so folder moves preserve the source recording leaf name.

Validated move:

```text
Oskar/Die unendliche Geschichte
  -> SmokeTest/Die unendliche Geschichte
```

Follow-up query confirmed:

```text
title: SmokeTest/Die unendliche Geschichte
backendNativeId: /srv/vdr/video/SmokeTest/Die_unendliche_Geschichte/2026-04-18.20.08.1-0.rec
```

Validated move back:

```text
SmokeTest/Die unendliche Geschichte
  -> Oskar/Die unendliche Geschichte
```

Follow-up query confirmed:

```text
title: Oskar/Die unendliche Geschichte
backendNativeId: /srv/vdr/video/Oskar/Die_unendliche_Geschichte/2026-04-18.20.08.1-0.rec
```

---

## Frontend Contract Findings

Recording actions can change backend-owned recording identifiers.

Observed behavior:

```text
Rename changed recordingId 533 -> 975.
Move returned the recording under a new list position.
```

Frontend clients must therefore reload the recording list after every mutating recording action:

```text
CREATE-like action
UPDATE-like action
RENAME
MOVE
DELETE
```

Clients must not cache `recordingId` as a durable identifier after a mutating action.

---

## Validated Recording Action State

The following recording actions were validated against a real VDR through VDR-Suite and RESTfulAPI:

```text
Recording Delete
Recording Rename
Recording Move
```

Together with the earlier real timer action validation, the current real action coverage includes:

```text
Timer Create
Timer Update
Timer Delete
Recording Rename
Recording Delete
Recording Move
```

---

## Architectural Result

The validation confirms the long-term backend-neutral identity direction:

```text
backendId + backendNativeId
```

For VDR/RESTfulAPI:

```text
backendNativeId = absolute VDR recording file name
```

For future backends this can map to backend-native stable identifiers such as TVHeadend recording UUIDs or other DVR-native item identifiers.

VDR remains the source of truth for recording state. VDR-Suite exposes and routes the backend-native identity without inventing a separate mutable recording identity.
---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to Development Index](index.md)
