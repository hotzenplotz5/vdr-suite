# Phase 8.40 – Action Target Architecture

## Status

Architecture decision for future source-aware jobs and actions.

No implementation has been performed by this document.

## Repository Findings

The current action and job model is recording-id based.

Current `RecordingAction` stores:

* recordingId
* RecordingActionType
* status
* message

Current `Job` stores:

* id
* recordingId
* jobType
* status
* priority
* message
* timestamps

Current `ActionService`, `JobService` and `RecordingWorkflowService` all accept `int recordingId`.

Current `jobs` database table stores `recording_id` as a foreign key to `recordings(id)`.

This is correct for the current single-source architecture.

It is not sufficient for future multi-source destructive operations.

## Problem

In a multi-source architecture, `recordingId` alone is not globally safe.

Examples:

* Source house-a has recording id 42.
* Source house-b also has recording id 42.
* Archive source may also have item id 42.

A destructive action such as rename, move, repair, shrink or delete must never target an ambiguous local id.

## Decision

VDR-Suite should introduce ActionTarget as a future architecture concept.

ActionTarget represents the target of an operation.

For source-aware content operations, ActionTarget should eventually reference ContentIdentity.

Future conceptual model:

```text
Action
        ↓
ActionTarget
        ↓
ContentIdentity
        ↓
Source
        ↓
Adapter / Worker
```

## Why ActionTarget Is Needed

ContentIdentity identifies an object globally across Sources.

ActionTarget identifies what an operation is supposed to affect.

This distinction matters because not every future action may target the same kind of object.

Examples:

* recording action targets a recording identity
* timer action targets a timer identity
* channel action targets a channel identity
* remote-control action may target a source, not content
* library action may target a library view

Therefore ContentIdentity is necessary, but ActionTarget is the correct action-layer abstraction.

## Current Risk

The current architecture uses `int recordingId` across:

* RecordingAction
* Job
* ActionService
* JobService
* RecordingWorkflowService
* JobRepository
* jobs table

This must not be casually extended with a simple `sourceId` field everywhere without a dedicated migration step.

That would duplicate identity logic and couple jobs directly to source internals.

## Recommended Future Direction

Future job/action targeting should move toward:

```text
Job
        ↓
ActionTarget
        ↓
ContentIdentity
```

instead of:

```text
Job
        ↓
recordingId only
```

or:

```text
Job
        ↓
sourceId + recordingId duplicated directly
```

## Capability Relationship

Capabilities should be interpreted as operation capabilities.

A Source does not merely have abstract capabilities.

A Source supports or denies specific operations.

Examples:

* ViewRecording
* StreamRecording
* RenameRecording
* DeleteRecording
* CreateTimer
* ModifyTimer
* DeleteTimer
* ViewLiveTv

This aligns capabilities with the action model and avoids a separate unrelated capability hierarchy.

## Permission Relationship

Permission should be evaluated against:

* Actor
* Action
* ActionTarget
* Source

Future rule:

```text
allowed = source exists AND source supports action AND actor may perform action on target
```

This document does not implement permission checks.

## Migration Boundary

Do not change `RecordingAction`, `Job`, `JobRepository` or the jobs database schema in the first SourceType or Source implementation phase.

Source-aware job migration is a separate architecture and implementation phase.

Do not introduce destructive remote or multi-source operations before ActionTarget and ContentIdentity are implemented and tested.

## Recommended Phase Order

Architecture-safe order:

1. SourceType
2. Source
3. SourceRegistry
4. ContentIdentity value object
5. ActionTarget architecture
6. ActionTarget value object
7. Job model migration
8. Workflow service migration
9. Job repository and schema migration
10. Capability integration
11. Permission integration
12. Federation-safe operations

## Non-Goals

This document does not implement:

* ActionTarget class
* ContentIdentity class
* Job schema migration
* RecordingAction migration
* Workflow service migration
* permission checks
* capability checks
* federation
* REST API changes
* frontend behavior

## Final Recommendation

ActionTarget is required before source-aware destructive operations.

The existing `int recordingId` job model should remain unchanged until a dedicated source-aware job migration phase.

The immediate next code phase should still stay small and introduce SourceType and Source only.
