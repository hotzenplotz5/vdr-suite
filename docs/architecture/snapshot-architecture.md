# Snapshot Architecture

## Motivation

The original live integration path queried RESTfulAPI directly through service calls.

The long-term direction is a daemon-owned snapshot architecture.

Goals:

- reduce repeated RESTfulAPI requests
- provide stable API responses from cached state
- prepare future multi-VDR architectures
- prepare future source, capability and permission concepts
- keep RESTfulAPI behind adapter boundaries

## Current Direction

```text
RESTfulAPI
    ↓
RestfulApiVdrAdapter
    ↓
IVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
VdrChangeState
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent[]
    ↓
SnapshotCacheService
    ↓
SnapshotCache
    ↓
REST API / future event dispatch
```

## Current Components

### VdrSnapshot

Aggregates:

- VdrStatus
- recordings
- timers
- channels
- events

### VdrSnapshotBuilder

Builds a complete snapshot using VdrService.

### SnapshotCache

Owns the latest daemon-visible VDR snapshot.

Responsibilities:

- track whether a snapshot exists
- expose the current snapshot
- replace the current snapshot after refresh
- clear the current snapshot

### SnapshotCacheService

Provides a service boundary around SnapshotCache.

The service exists so later phases can evolve cache access without exposing storage details directly to API controllers or backend-specific adapters.

### PollingService

Responsible for deciding when snapshots need to be refreshed.

Current responsibilities:

- poll VdrChangeState
- detect VdrChangeEvent values
- ask SnapshotRefreshDecisionService whether a snapshot refresh is needed
- rebuild snapshots through VdrSnapshotBuilder
- write refreshed snapshots into SnapshotCacheService
- expose the current cached snapshot

### VdrChangeState

Represents backend-visible change counters for VDR-related domains.

Current domains:

- status
- channels
- recordings
- timers
- events

### VdrChangeEvent

Represents detected change events that can later drive refresh decisions, SSE, WebSocket updates, notifications and multi-VDR synchronization.

Current event types:

- StatusChanged
- ChannelsChanged
- RecordingsChanged
- TimersChanged
- EventsChanged

### ChangeDetectionService

Compares a previous VdrChangeState with a current VdrChangeState and produces VdrChangeEvent entries for changed domains.

Example:

```text
previous.channelsVersion = 1
current.channelsVersion  = 2

→ ChannelsChanged
```

## Change-State Based Polling Strategy

The long-term polling strategy is not based on rebuilding complete snapshots on every polling cycle.

VDR-Suite requires RESTfulAPI change-state information for efficient polling.

Target architecture:

```text
RESTfulAPI
    ↓
/change-state.json
    ↓
VdrChangeState
    ↓
ChangeDetectionService
    ↓
SnapshotRefreshDecisionService
    ↓
VdrSnapshotBuilder
    ↓
SnapshotCacheService
    ↓
SnapshotCache
```

Goals:

- avoid unnecessary full snapshot rebuilds
- avoid repeated loading of unchanged channels, timers, recordings and events
- reduce HTTP traffic between VDR-Suite and RESTfulAPI
- prepare efficient multi-VDR polling
- prepare later event dispatch through SSE or WebSocket

## RESTfulAPI Patch Direction

A RESTfulAPI patch exposes stable change-state counters or equivalent version information for VDR domains.

Minimum useful domains:

- status
- channels
- recordings
- timers
- events

The endpoint allows VDR-Suite to decide whether a full or partial snapshot refresh is required before loading larger data sets.

The preferred direction is a lightweight state endpoint rather than repeatedly loading all heavy RESTfulAPI resources.

## Planned Runtime Integration

```text
DaemonRuntime
        ↓
SnapshotCache
        ↓
SnapshotCacheService
        ↓
PollingService
        ↓
VdrChangeState
        ↓
ChangeDetectionService
        ↓
VdrChangeEvent[]
        ↓
SnapshotRefreshDecisionService
        ↓
VdrSnapshotBuilder
        ↓
VdrService
        ↓
IVdrAdapter
```

## Completed Phases

### Phase 8.80

Remove duplicate VDR test targets.

### Phase 8.81

Integrate PollingService into DaemonRuntime.

### Phase 8.82

Introduce VdrChangeState domain object.

### Phase 8.83

Introduce VdrChangeEvent domain object.

### Phase 8.84

Introduce ChangeDetectionService.

### Phase 8.90

Integrate change-state polling service.

### Phase 8.91

Generate backend-neutral VdrChangeEvent values from change-state transitions.

### Phase 8.92

Introduce SnapshotRefreshDecision and SnapshotRefreshDecisionService.

### Phase 8.93

Introduce SnapshotCache and SnapshotCacheService.

## Planned Phases

### Phase 8.94

Integrate SnapshotCacheService into PollingService and DaemonRuntime.

### Future

Move REST endpoints to snapshot-backed responses.

## Architectural Rules

- RESTfulAPI remains behind adapters.
- Controllers should not call RESTfulAPI directly.
- Daemon owns refresh logic.
- Snapshot models remain backend-neutral.
- PollingService owns refresh decisions, not storage internals.
- SnapshotCache owns current snapshot storage.
- Multi-VDR assumptions must remain possible.
- Polling optimizations may depend on RESTfulAPI change-state support.
- Event dispatch must be driven by backend-neutral VdrChangeEvent values.
- No SSE runtime, WebSocket runtime, federation runtime, user management or permission runtime is introduced by the snapshot cache integration phase.
