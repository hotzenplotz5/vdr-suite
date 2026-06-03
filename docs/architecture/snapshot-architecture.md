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
Snapshot Cache
    ↓
REST API
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
Snapshot Refresh Decision
    ↓
VdrSnapshotBuilder
```

Goals:

- avoid unnecessary full snapshot rebuilds
- avoid repeated loading of unchanged channels, timers, recordings and events
- reduce HTTP traffic between VDR-Suite and RESTfulAPI
- prepare efficient multi-VDR polling

Potential future endpoint examples:

```text
/change-state.json
/state.json
/versions.json
```

The exact endpoint design is intentionally deferred until snapshot cache integration is completed.

Architectural decision:

A patched RESTfulAPI implementation is considered a supported requirement for future VDR-Suite polling optimizations.

## Planned Runtime Integration

```text
DaemonRuntime
        ↓
PollingService
        ↓
VdrSnapshotBuilder
        ↓
VdrService
        ↓
IVdrAdapter
```

## Planned Phases

### Phase 8.80

Remove duplicate VDR test targets.

### Phase 8.81

Integrate PollingService into DaemonRuntime.

### Phase 8.82

Introduce snapshot refresh cycle.

### Phase 8.83

Introduce daemon-owned snapshot cache.

### Future

Move REST endpoints to snapshot-backed responses.

## Architectural Rules

- RESTfulAPI remains behind adapters.
- Controllers should not call RESTfulAPI directly.
- Daemon owns refresh logic.
- Snapshot models remain backend-neutral.
- Multi-VDR assumptions must remain possible.
- Polling optimizations may depend on RESTfulAPI change-state support.
