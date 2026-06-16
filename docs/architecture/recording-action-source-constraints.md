# Recording Action Source Constraints

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document records the source-derived constraints for recording actions.

It is based on the current VDR core, LIVE plugin, RESTfulAPI plugin, and VDR-Suite action model.

The goal is to keep VDR-Suite aligned with real backend behavior before enabling real mutation.

---

## Source Systems

The current action model is informed by these source systems:

- VDR core recording model
- LIVE recording edit flow
- RESTfulAPI recording endpoints
- VDR-Suite safety and capability contracts

---

## VDR Core Constraints

VDR recordings expose explicit usage states.

A recording may be in use by:

- timer recording
- replay
- cut
- move
- copy
- source operation
- destination operation
- pending operation
- canceled operation awaiting cleanup

VDR exposes this through the recording usage flags and `IsInUse()`.

This means VDR-Suite must treat recording usage as a real blocker before destructive or moving actions.

---

## VDR Core Delete Semantics

VDR delete does not mean immediate filesystem removal.

The normal delete operation marks a recording as deleted and moves it from the active recordings list to the deleted recordings list.

Actual disk removal is a separate operation.

VDR-Suite must therefore model delete as a VDR delete operation, not as direct filesystem deletion.

---

## VDR Core Rename and Move Semantics

VDR supports changing the recording name through VDR recording naming semantics.

The recording name is not just a display title.

It is related to the recording directory path below the video directory.

VDR-Suite must therefore keep these concepts separate:

- display title
- VDR recording name
- folder path
- physical recording directory

---

## RESTfulAPI Constraints

RESTfulAPI currently exposes recording operations through endpoints including:

- `/recordings/move`
- `/recordings/delete`
- `/recordings/cut`
- `/recordings/marks`

The move endpoint expects:

- `source`
- `target`
- `copy_only`

The delete-by-name endpoint expects:

- `file`

These values are transport-level fields.

They are not VDR-Suite domain field names.

---

## RESTfulAPI Move Failure Cases

RESTfulAPI move may fail when:

- source is missing
- target is missing
- source path does not exist
- source recording cannot be found
- source path has no valid directory separator
- directory move/copy fails
- moved recording cannot be found after the operation

VDR-Suite must map these failures into stable execution result errors.

---

## RESTfulAPI Delete Failure Cases

RESTfulAPI delete may fail when:

- file is missing
- recording cannot be found
- VDR delete returns false

VDR-Suite must map these failures into stable execution result errors.

---

## LIVE Constraints

LIVE performs permission checks before allowing recording edit operations.

LIVE edit flow supports:

- rename
- move
- copy
- metadata update
- delete resume information
- delete marks information

LIVE uses the recording manager as a higher-level abstraction for recording updates.

VDR-Suite should treat LIVE as the functional reference for user-facing behavior, not as a transport API to copy directly.

---

## VDR-Suite Implications

VDR-Suite must keep four separate layers:

| Layer | Responsibility |
| --- | --- |
| Request validation | Is the request syntactically and semantically valid? |
| Capability check | Does the backend support the action in principle? |
| Safety check | May this request execute right now? |
| Execution | Perform the backend mutation or dry-run simulation. |

No real backend mutation should bypass these layers.

---

## Required Safety Inputs

The source review confirms that these inputs are required:

- backend exists
- backend is writable
- action capability exists
- recording exists
- recording is not in use
- target is present for move
- target does not conflict with an existing recording
- execution is explicitly enabled
- dry-run mode is honored

---

## Next Implementation Direction

The next implementation step should not enable real delete yet.

The safe order is:

1. expose source-derived blocker names in the VDR-Suite model
2. add stable safety reason identifiers
3. extend safety preview JSON
4. map RESTfulAPI failures into stable result errors
5. only then run controlled real-backend smoke tests

Delete must remain the last real mutation enabled.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Architecture Index](index.md)
