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

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)

