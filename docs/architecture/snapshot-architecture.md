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
Snapshot Cache / Event Dispatch / REST API
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

### PollingService

Responsible for refreshing snapshots.

Current responsibilities:

- poll()
- hold latest snapshot
- provide snapshot()

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

VDR-Suite will require an extended RESTfulAPI implementation that exposes change-state information.

Target architecture:

```text
RESTfulAPI
    ↓
Change-State Endpoint
    ↓
PollingService
    ↓
VdrChangeState
    ↓
ChangeDetectionService
    ↓
Snapshot Refresh Decision
    ↓
VdrSnapshotBuilder
```

Goals:

- avoid unnecessary full snapshot rebuilds
- avoid repeated loading of unchanged channels, timers, recordings and events
- reduce HTTP traffic between VDR-Suite and RESTfulAPI
- prepare efficient multi-VDR polling
- prepare later event dispatch through SSE or WebSocket

Potential future endpoint examples:

```text
/change-state.json
/state.json
/versions.json
```

The exact endpoint design is intentionally deferred until the RESTfulAPI patch strategy is defined.

Architectural decision:

A patched RESTfulAPI implementation is considered a supported requirement for future VDR-Suite polling optimizations.

## RESTfulAPI Patch Direction

A future RESTfulAPI patch should expose stable change-state counters or equivalent version information for VDR domains.

Minimum useful domains:

- status
- channels
- recordings
- timers
- events

The endpoint should allow VDR-Suite to decide whether a full or partial snapshot refresh is required before loading larger data sets.

The preferred direction is a lightweight state endpoint rather than repeatedly loading all heavy RESTfulAPI resources.

## Planned Runtime Integration

```text
DaemonRuntime
        ↓
PollingService
        ↓
VdrChangeState
        ↓
ChangeDetectionService
        ↓
VdrChangeEvent[]
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

## Planned Phases

### Phase 8.85

Integrate change detection with PollingService.

### Future

Move REST endpoints to snapshot-backed responses.

## Architectural Rules

- RESTfulAPI remains behind adapters.
- Controllers should not call RESTfulAPI directly.
- Daemon owns refresh logic.
- Snapshot models remain backend-neutral.
- Multi-VDR assumptions must remain possible.
- Polling optimizations may depend on RESTfulAPI change-state support.
- Event dispatch must be driven by backend-neutral VdrChangeEvent values.
