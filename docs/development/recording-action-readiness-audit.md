# Recording Action Readiness Audit

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current Status](current-status.md)
- [Recording Actions Architecture](../architecture/recording-actions-architecture.md)
- [Roadmap](../planning/roadmap.md)

---

## Phase

Phase 60.15a - Recording Action Readiness Audit

---

## Purpose

This document freezes the readiness decision before enabling real Recording
actions in the VDR-Suite frontend.

The next product block is not metadata/poster work. The immediate goal is to
make Recording actions real:

- rename
- move
- delete

Delete must be modeled as VDR-compatible soft delete / trash behavior, not as
a direct hard filesystem deletion.

---

## Source Findings

### VDR delete model

VDR distinguishes normal Recording directories and deleted Recording
directories through the extensions:

- `.rec`
- `.del`

Deleted recordings are scanned as a separate deleted Recording collection.

VDR also has a background deleted-recording removal mechanism that removes
deleted recordings only after retention / cleanup conditions are met.

Decision:

- VDR-Suite must treat frontend delete as `move to trash`.
- VDR-Suite must not expose direct permanent delete as the primary delete action.
- Permanent deletion, if implemented later, must be a separate explicit admin
  action.

### VDR Live delete model

VDR Live does not delete recordings by performing direct frontend filesystem
operations.

The Live recordings page collects deletion requests and runs a
`RemoveRecordingTask` through the Live task manager.

Decision:

- VDR-Suite should keep the same architectural spirit:
  frontend intent -> API -> validation -> action service -> backend adapter.
- The frontend must never call Rectools, VDR directories or filesystem paths
  directly.

### VDR-Suite current action foundation

VDR-Suite already has active action routes:

- `POST /api/recordings/actions/validate`
- `POST /api/vdr/recordings/actions/validate`
- `POST /api/recordings/actions/execute`
- `POST /api/vdr/recordings/actions/execute`

The core execution service validates requests and skips backend execution while
`dryRun=true`. When `dryRun=false`, execution can reach the backend adapter.

The RESTfulAPI executor already has branches for:

- move
- rename
- delete

Decision:

- The next implementation step should not create a parallel action mechanism.
- The existing action pipeline must be used.

---

## Product Vocabulary

Use clear user-facing wording:

- `Umbenennen`
- `Verschieben`
- `In Papierkorb verschieben`

Avoid using plain `Löschen` as the primary visible action until the UI clearly
explains the VDR trash behavior.

---

## Safety Rules

Real Recording actions require:

- backend identity
- backend-owned recording identity
- capability check
- permission check
- validation before execution
- explicit user confirmation for mutation
- `dryRun=false` only after confirmation
- visible action result
- cache refresh or invalidation after success

Delete/trash additionally requires:

- warning text that the recording is moved to the VDR trash/deleted area
- no permanent delete from the normal detail card
- future separate trash view before restore/permanent-delete is exposed

---

## Recommended Execution Order

### Phase 60.15b - Rename Action Runtime Probe

Goal:

- prove validate and execute payloads for rename
- keep the frontend button disabled for real users until runtime behavior is
  verified

### Phase 60.15c - Rename Action UI

Goal:

- enable real rename with confirmation
- use `dryRun=false` only after confirmation

### Phase 60.15d - Move Action Runtime Probe

Goal:

- prove move payload, target folder handling and backend behavior

### Phase 60.15e - Move Action UI

Goal:

- enable real move with confirmation and target folder input

### Phase 60.15f - Trash Action Runtime Probe

Goal:

- prove delete maps to VDR-compatible trash/soft-delete behavior

### Phase 60.15g - Trash Action UI

Goal:

- expose `In Papierkorb verschieben`
- no permanent delete in normal Recording detail view

---

## Rename Runtime Probe Result

Phase 60.15b verified the first real Recording action pipeline with `dryRun=true`.

No real mutation was performed.

Runtime target:

```text
http://127.0.0.1:18080
```

Backend:

```text
default
```

Authenticated user:

```text
admin
```

Probe recording:

```json
{
  "backendId": "default",
  "id": "226",
  "title": "Action/48 Hrs",
  "path": "/Action/48 Hrs/2026-06-21.10.08.1-0.rec",
  "backendNativeId": "/srv/vdr/video/Action/48 Hrs/2026-06-21.10.08.1-0.rec"
}
```

First probe finding:

```json
{
  "action": "rename"
}
```

The lowercase action name was rejected as `UNKNOWN` because the current parser
expects uppercase action names.

Correct action value:

```json
{
  "action": "RENAME"
}
```

Successful dry-run payload:

```json
{
  "backendId": "default",
  "recordingId": "226",
  "action": "RENAME",
  "dryRun": true,
  "newName": "__VDR_SUITE_RENAME_RUNTIME_PROBE__",
  "recordingPath": "/Action/48 Hrs/2026-06-21.10.08.1-0.rec",
  "backendNativeId": "/srv/vdr/video/Action/48 Hrs/2026-06-21.10.08.1-0.rec"
}
```

Validation result:

```json
{
  "valid": true,
  "dryRun": true,
  "wouldCreateJob": false,
  "backendId": "default",
  "recordingId": "226",
  "requiredCapabilities": [
    "recording.action.rename"
  ],
  "requiredPermissions": [
    "recording.permission.rename"
  ],
  "warnings": [
    "dry-run only"
  ],
  "errors": []
}
```

Execution result:

```json
{
  "success": false,
  "type": "RENAME",
  "backendId": "default",
  "recordingId": "226",
  "backendNativeId": "/srv/vdr/video/Action/48 Hrs/2026-06-21.10.08.1-0.rec",
  "recordingPath": "/Action/48 Hrs/2026-06-21.10.08.1-0.rec",
  "snapshotRefreshed": false,
  "upstreamHttpStatus": 0,
  "upstreamEndpoint": "",
  "upstreamResponseBody": "",
  "message": "dry-run backend execution skipped",
  "warnings": [
    "dry-run only"
  ],
  "errors": []
}
```

Interpretation:

- `RENAME` is parsed correctly.
- validation accepts the request.
- required rename capability is reported.
- required rename permission is reported.
- execution reaches the dry-run boundary.
- no backend mutation happens while `dryRun=true`.

The expected result is:

```text
success=false
message=dry-run backend execution skipped
```

This is not a failure. It proves the safety boundary is active.

Decision:

- The next implementation step may prepare real rename execution.
- The frontend must send uppercase `RENAME`.
- `dryRun=false` may only be sent after explicit confirmation.
- Delete/trash and move remain out of scope until rename has been proven safe.

---

## Non-Goals

The following are explicitly not part of the first sharp-action phase:

- direct hard delete
- trash browser
- restore
- empty trash
- Rectools direct frontend integration
- bypassing the existing action service
- metadata/poster work

---

## Acceptance Criteria

Phase 60.15a is complete when:

- the VDR/VDR-Live/VDR-Suite action model is documented
- delete is explicitly defined as trash/soft-delete for VDR-Suite UX
- the next implementation order is defined
- no runtime mutation has been performed

Phase 60.15b is complete when:

- the rename validation payload is proven with `dryRun=true`
- the rename execution route reaches the dry-run boundary
- uppercase `RENAME` is documented as the currently required action value
- no runtime mutation has been performed

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)

## Phase 60.15d: Delete validation and dry-run runtime probe

Status: completed as readiness probe only.

Runtime environment:

- VDR-Suite daemon: `http://127.0.0.1:18080`
- Backend: `default`
- Probe folder: `heute_journal`
- Probe mode: validation plus dry-run execution only
- No sharp delete was executed.

Observed source recording:

- Folder request returned exactly one recording in `heute_journal`.
- Recording id: `7983`
- Recording path: `/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec`
- Backend native id: `/srv/vdr/video/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec`

Validation result:

- HTTP status: `200`
- `valid`: `true`
- `dryRun`: `true`
- Required capability: `recording.action.delete`
- Required permission: `recording.permission.delete`
- Warning: `dry-run only`
- Errors: none

Dry-run execution result:

- HTTP status: `200`
- `success`: `false`
- `type`: `DELETE`
- `snapshotRefreshed`: `false`
- Message: `dry-run backend execution skipped`
- Warning: `dry-run only`
- Errors: none

Conclusion:

- Delete is recognized by the validation layer.
- Delete dry-run execution is safety-gated and does not call the backend executor.
- The Web UI must keep Delete in validate/dry-run-only mode until a dedicated sharp-delete probe exists.
- Before enabling sharp Delete, the backend request source path must be reviewed against RESTfulAPI mount-loop paths. Runtime probes have shown that RESTfulAPI can expose the same recording through `Recordings_on_yavdr(nfs)` mount aliases, so sharp Delete must not blindly trust the displayed backend-native path.
