# Phase 8.41 – Platform Action Architecture

## Status

Architecture review and decision document.

No implementation has been performed by this document.

## Repository Findings

The current action model is recording-specific.

Current files:

* `core/recordings/include/RecordingAction.h`
* `core/recordings/src/ActionService.cpp`
* `core/recordings/include/Job.h`
* `core/recordings/src/JobService.cpp`
* `core/recordings/include/RecordingWorkflowService.h`
* `core/recordings/src/RecordingWorkflowService.cpp`
* `core/recordings/src/RecordingActionUtils.cpp`
* `core/recordings/src/WorkerSimulator.cpp`

There is no separate platform-level `core/actions` implementation visible in the current repository state.

`RecordingActionType` is currently a closed recording-specific enum with values:

* Check
* Repair
* Shrink
* Cut
* Pes2Ts
* Rename
* Unknown

This is correct for the current recordings workflow.

It is not yet a platform action model.

## Current Architecture

Current simplified action flow:

```text
RecordingWorkflowService
        ↓
ActionService
        ↓
RecordingAction
        ↓
JobService
        ↓
Job
        ↓
JobRepository
        ↓
WorkerSimulator
```

Current job execution does not yet interpret job type or target content.

`WorkerSimulator` only changes job status from PENDING to RUNNING to DONE.

## Problem

Future VDR-Suite architecture needs actions beyond local recording workflows.

Future examples:

* view recording
* stream recording
* rename recording
* delete recording
* move recording
* create timer
* modify timer
* delete timer
* view EPG
* search EPG
* view channel
* stream Live TV
* remote-control command

Capabilities and permissions will both be expressed against operations.

Therefore a future platform-level action concept is likely needed.

## Decision

Do not replace `RecordingActionType` immediately.

Keep the existing recording-specific action model stable until a dedicated migration phase.

Introduce the idea of PlatformAction as a future architecture concept.

A PlatformAction represents a system operation independent of the current recording-only workflow.

Future conceptual model:

```text
Actor
        ↓
Permission
        ↓
PlatformAction
        ↓
ActionTarget
        ↓
ContentIdentity or Source
        ↓
Capability check
        ↓
Execution
```

## PlatformAction vs RecordingActionType

`RecordingActionType` is current implementation detail for recording workflow jobs.

`PlatformAction` is future architecture concept for all source-aware operations.

Do not rename `RecordingActionType` prematurely.

Do not move existing recording workflow files prematurely.

Do not create a generic action hierarchy until Source, ContentIdentity and ActionTarget have stable definitions.

## Capability Relationship

Capabilities should be interpreted as supported platform actions.

Examples:

```text
Source archive-main supports:
        ViewRecording
        StreamRecording

Source local-vdr supports:
        ViewRecording
        StreamRecording
        RenameRecording
        CreateTimer
        ModifyTimer
        ViewLiveTv
```

This avoids a second unrelated capability hierarchy.

Capability should answer:

```text
Can this Source support this PlatformAction?
```

## Permission Relationship

Permission should also reference PlatformAction.

Permission should answer:

```text
May this Actor perform this PlatformAction on this ActionTarget?
```

This keeps capability and permission aligned through the same action vocabulary.

## Job Relationship

Future jobs should eventually reference a platform action and an action target.

Future conceptual job:

```text
Job
        ↓
PlatformAction
        ↓
ActionTarget
```

Current job:

```text
Job
        ↓
recordingId
        ↓
jobType string
```

The current model should not be migrated casually.

A source-aware job migration must be a dedicated later phase.

## Worker Relationship

Current WorkerSimulator only updates job status.

This is useful because action target architecture can be designed before real execution is implemented.

Future real workers should resolve:

```text
Job
        ↓
PlatformAction
        ↓
ActionTarget
        ↓
Source
        ↓
Adapter or worker backend
```

## Source Relationship

Source is the bridge between content identity and execution.

Content-side path:

```text
Content
        ↓
ContentIdentity
        ↓
Source
```

Execution-side path:

```text
PlatformAction
        ↓
ActionTarget
        ↓
Source
        ↓
Capability
        ↓
Adapter / Worker
```

## Migration Boundary

Do not introduce PlatformAction in the same phase as SourceType or Source.

Do not modify existing recording workflow code until a separate migration step.

Do not change the jobs database table as part of this architecture decision.

Do not expose platform actions through REST yet.

Do not connect frontend behavior to platform actions yet.

## Recommended Future Phase Order

Architecture-safe order:

1. SourceType
2. Source
3. SourceRegistry
4. ContentIdentity
5. ActionTarget
6. PlatformAction architecture
7. PlatformAction value object or enum
8. Capability model alignment with PlatformAction
9. Permission model alignment with PlatformAction
10. Job model migration
11. Worker execution migration
12. REST and frontend exposure

## Non-Goals

This document does not implement:

* PlatformAction class
* PlatformAction enum
* generic action registry
* capability checks
* permission checks
* job schema migration
* worker execution logic
* REST API changes
* frontend behavior
* federation behavior

## Final Recommendation

VDR-Suite should keep the current recording-specific action model unchanged for now.

A future platform action model should be introduced only after Source, ContentIdentity and ActionTarget are stable.

Capabilities and permissions should eventually align around the platform action vocabulary.
