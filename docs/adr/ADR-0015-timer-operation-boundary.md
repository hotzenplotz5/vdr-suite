# ADR-0015: Timer Operation Boundary

## Status

Accepted

## Context

VDR-Suite now exposes snapshot-backed read APIs for status, channels, timers, events and recordings.

Timer reads are already part of the snapshot read domain, but timer writes are a different architectural concern.

A VDR timer is not just a passive database row. It is an active recording schedule object that can influence:

```text
recording creation
recording start and stop behavior
channel usage
conflict handling
VPS behavior
priority and lifetime handling
plugin-driven search timer workflows
```

Future VDR-Suite frontends may need to create, update, delete or inspect timers across one or more VDR backends.

Backends may expose timer operations through different mechanisms:

```text
RESTfulAPI
SVDRP
future plugin bridges
mock adapters
future remote or federated backends
```

Timer identifiers, channel references, EPG event references and timer capabilities are backend-scoped. A timer number or backend-local timer id must not be treated as a global VDR-Suite identity.

Treating timers as ordinary CRUD records would create architecture risk because timer changes can immediately affect live recording behavior.

## Decision

VDR-Suite will treat timer write operations as validated domain actions, not as direct CRUD operations.

Timer create, update and delete operations must pass through:

- capability validation
- permission validation
- operation validation
- action boundaries
- backend adapter boundaries

Frontend code, REST controllers and generic service code must not execute backend timer commands directly.

Timer operations must be expressed as explicit domain actions such as:

```text
CreateTimerAction
UpdateTimerAction
DeleteTimerAction
```

The exact implementation names may evolve, but the boundary rule is mandatory.

## Backend Scope

All timer operations are backend-scoped.

A future timer reference must include enough backend context to avoid single-VDR assumptions.

Conceptually, a timer operation target is closer to:

```text
backendId + backendLocalTimerId
```

than to:

```text
timerId
```

Channel references, EPG event references and plugin-specific timer metadata must also remain backend-scoped unless explicitly mapped into backend-neutral domain objects.

## Capability Model

Timer functionality must be capability-driven.

Backends may support different timer features, for example:

```text
timer.read
timer.create
timer.update
timer.delete
timer.conflict_read
timer.vps
timer.priority
timer.lifetime
timer.search_timer_bridge
```

Higher layers must ask for capabilities instead of branching on backend implementation names.

The existence of a backend transport does not imply that every timer operation is safe or available.

## Validation Requirements

Before a timer write operation is executed, VDR-Suite must validate the operation against the current backend state where possible.

Validation may include:

- target backend existence
- backend availability
- timer capability support
- user permission
- channel availability
- time range validity
- running or imminent recording state
- conflict state where the backend can expose it
- plugin-specific constraints after they are mapped into backend-neutral fields

Validation must be performed before the backend adapter executes the operation.

## RESTfulAPI and SVDRP Implications

RESTfulAPI and SVDRP are backend transports, not timer domain layers.

RESTfulAPI may be the preferred structured integration path when it exposes suitable timer operations.

SVDRP may be appropriate for native VDR control operations where RESTfulAPI does not provide the required operation or where SVDRP better represents the VDR control model.

Both mechanisms must remain behind `IVdrAdapter` or a future equivalent timer-capable adapter boundary.

Backend command names, response formats and transport-specific error details must not leak into frontend or controller contracts.

## Plugin Timer Implications

Plugin-based timer workflows, such as future bridges to search timers or plugin-specific scheduling features, must not expose raw plugin internals as public VDR-Suite APIs.

Plugin timer data must be mapped into backend-neutral domain objects or clearly marked backend-specific capability data below the adapter boundary.

Any plugin-based timer write or control operation must still pass through capability, permission, validation and action boundaries.

## Snapshot and Change Feed Implications

Timer snapshots are read models.

Snapshot timer data may be used to display current state and to support validation, but a snapshot item alone is not an executable command.

Future snapshot change feeds may report timer changes such as:

```text
timer.added
timer.changed
timer.removed
timer.recording_state_changed
```

These events should describe observed state changes. They must not bypass action validation for future write operations.

## Consequences

Benefits:

- avoids treating active recording schedules as passive CRUD rows
- protects destructive or recording-affecting operations behind action boundaries
- supports multiple VDR backends
- supports backend-specific timer capabilities without leaking backend internals
- keeps RESTfulAPI, SVDRP and plugin bridges interchangeable behind adapter boundaries
- prepares VDR-Suite for permission-aware timer control

Trade-offs:

- timer write implementation becomes more explicit
- simple controller-level CRUD endpoints are intentionally avoided
- validation requires current backend state and capability information
- tests must cover backend capability and permission combinations

## Non-Goals

This ADR does not implement:

- timer write APIs
- timer persistence schema
- timer conflict detection
- search timer integration
- SVDRP timer adapter implementation
- RESTfulAPI timer write implementation
- permission model enforcement code
- frontend timer workflows

This ADR records the architecture rule only.

## Follow-Up Work

Future phases may introduce:

- backend-scoped timer references
- timer write action objects
- timer operation validators
- timer capability declarations
- timer permission checks
- timer conflict read models
- REST API endpoints for validated timer actions
- tests for multi-backend timer identity and capability behavior
