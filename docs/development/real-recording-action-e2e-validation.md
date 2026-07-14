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
VDR-Suite web frontend
  -> VDR-Suite HTTP API
  -> RecordingActionExecutionController
  -> RecordingActionExecutionService
  -> RESTfulAPI recording action backend executor
  -> RESTfulAPI plugin
  -> VDR recording domain
  -> snapshot and persistent recording-cache reconcile
```

The real daemon was reachable on:

```text
127.0.0.1:18080
```

The real RESTfulAPI backend was reachable through the configured default backend.

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

For VDR/RESTfulAPI, `backendNativeId` maps to the absolute VDR recording filename used by the RESTfulAPI plugin and VDR recording domain.

---

## Native Recording Trash Validation

Recording delete now uses the native safe trash workflow instead of the legacy single-request delete path.

The frontend dry-run reaches the real non-mutating plugin preview endpoint:

```text
POST /recordings/trash/preview.json
```

After user confirmation, the sharp execution path runs:

```text
POST /recordings/trash/preview.json
POST /recordings/trash/validate.json
POST /recordings/trash.json
```

The workflow preserves native VDR behavior:

```text
.rec -> .del
```

VDR remains the source of truth. VDR-Suite does not introduce a separate trash store or restore model.

### VDR-Suite endpoint

```text
POST /api/vdr/recordings/actions/execute
```

### Action payload shape

```json
{
  "backendId": "default",
  "recordingId": "531",
  "action": "DELETE",
  "dryRun": false,
  "backendNativeId": "/srv/vdr/video/Oskar/Flipper/2026-05-13.10.10.1-0.rec"
}
```

### Safety properties

The native plugin contract provides:

- real preview before mutation,
- explicit blockers and warnings,
- optimistic concurrency revisions for recordings and timers,
- validation immediately before execution,
- protection against active recording and replay conflicts,
- idempotent `already-trashed` success,
- no automatic stopping of recording, replay or timers.

Previously verified plugin-level scenarios include:

```text
missing revision                  -> HTTP 400
stale revision                    -> HTTP 409
active local recording            -> HTTP 423
active replay                     -> HTTP 423
successful native trash           -> HTTP 200, status=trashed
immediate repeated execution      -> HTTP 200, status=already-trashed
```

### Real frontend and daemon result

The browser workflow was exercised repeatedly with expendable real recordings.

Observed request sequence:

```text
POST /recordings/trash/preview.json   -> 200
POST /recordings/trash/validate.json  -> 200
POST /recordings/trash.json           -> 200
```

The active recording count decreased on each successful run:

```text
998 -> 997 -> 996
```

The recording cache then reconciled to the new VDR state and the removed recording disappeared from the frontend without requiring a manual page refresh.

---

## Recording Cache Reconcile Validation

A successful recording mutation performs an immediate readback and bounded follow-up reconciliation.

The initial implementation scheduled eight full recording reloads. With a real catalog of roughly one thousand recordings, each `/recordings.json` response was approximately 4.5 MB and unnecessary retries increased backend load.

The reconcile budget is now bounded to:

```text
1 immediate readback
2 recording-action-reconcile attempts maximum
```

Real runtime logs confirmed exactly two follow-up reconcile runs after the immediate readback.

The final cache count after the latest validation was:

```text
recordings=996
```

All observed recording cache writes completed with:

```text
stored=true
```

---

## Shared SQLite Transaction Validation

The real trash workflow exposed overlapping EPG-cache and recording-cache persistence on the shared SQLite connection.

The original failure was:

```text
SQLite error: cannot start a transaction within a transaction
```

The database layer now serializes transaction ownership from `BEGIN` through `COMMIT` or `ROLLBACK` across the shared connection.

The fix is covered by a two-thread regression test and was verified under real concurrent load:

```text
Recording cache warmup finished: stored=true
EPG cache warmup finished: stored=true
```

No further nested-transaction error occurred while recording and EPG refreshes overlapped.

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

The recording was then renamed back successfully. Follow-up query confirmed the restored title and native path.

Rename has real execution evidence, but it does not yet use the new native `preview -> validate -> execute` gold-standard contract.

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

The recording was moved back successfully and the original title and native path were restored.

Move has real execution evidence, but it does not yet use the new native `preview -> validate -> execute` gold-standard contract. Move is the next recording workflow to be upgraded.

---

## Frontend Contract Findings

Recording actions can change backend-owned recording identifiers.

Observed behavior:

```text
Rename changed recordingId 533 -> 975.
Move returned the recording under a new list position.
Delete removes the recording from the active VDR catalog.
```

Frontend clients must therefore reload or reconcile the recording list after every mutating recording action:

```text
RENAME
MOVE
DELETE
```

Clients must not cache `recordingId` as a durable identifier after a mutating action.

The durable routing identity remains:

```text
backendId + backendNativeId
```

---

## Validated Recording Action State

The following recording actions were validated against a real VDR through VDR-Suite and RESTfulAPI:

```text
Recording Delete
Recording Rename
Recording Move
```

Delete now additionally has complete gold-standard coverage:

```text
real frontend preview
preview blockers
revision validation
native execution
idempotent result mapping
automatic cache readback
bounded reconcile retries
concurrent cache persistence safety
```

Together with earlier timer action validation, current real action coverage includes:

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

The validated recording mutation gold standard is:

```text
Preview
  -> Validate current revisions
  -> Execute native backend mutation
  -> Immediate readback
  -> Bounded reconcile
  -> Frontend state refresh
```

For future backends, `backendNativeId` can map to backend-native stable identifiers such as TVHeadend recording UUIDs or other DVR-native item identifiers.

VDR remains the source of truth for recording state. VDR-Suite exposes and routes the backend-native identity without inventing a separate mutable recording identity.

---

## Next Recording Workflow

The next implementation block is the Move gold-standard upgrade.

Before changing runtime behavior, the existing Move path must be audited for:

- current dry-run behavior,
- source and target identity rules,
- destination leaf-name preservation,
- stale-state and conflict handling,
- post-move identity changes,
- preview and validation contract gaps,
- bounded readback and frontend refresh behavior.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to Development Index](index.md)
