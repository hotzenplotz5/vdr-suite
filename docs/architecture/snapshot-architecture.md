# Snapshot Architecture

## Motivation

The original live integration path queried RESTfulAPI directly through service calls.

The current direction is a daemon-owned snapshot architecture.

Goals:

- reduce repeated RESTfulAPI requests
- provide stable API responses from cached state
- prepare future multi-VDR architectures
- prepare future source, capability and permission concepts
- keep RESTfulAPI behind adapter boundaries
- keep refresh decisions separate from snapshot storage

## Current Implemented Direction

RESTfulAPI -> change-state endpoint -> RestfulApiVdrAdapter -> VdrChangeState -> VdrService -> PollingService -> ChangeDetectionService -> VdrChangeEvent -> SnapshotRefreshDecisionService -> VdrSnapshotBuilder -> SnapshotCacheService -> SnapshotCache -> DaemonRuntime -> future snapshot-backed REST API responses

## Current Components

### VdrSnapshot

Aggregates VDR status, recordings, timers, channels and events.

### VdrSnapshotBuilder

Builds a complete backend-neutral snapshot using VdrService.

### SnapshotCache

Owns the latest daemon-visible VDR snapshot.

### SnapshotCacheService

Provides a service boundary around SnapshotCache.

### PollingService

Responsible for deciding when snapshots need to be refreshed.

Current responsibilities:

- poll VdrChangeState through VdrService
- detect VdrChangeEvent values
- ask SnapshotRefreshDecisionService whether a snapshot refresh is needed
- rebuild snapshots through VdrSnapshotBuilder
- write refreshed snapshots into SnapshotCacheService

### DaemonRuntime

Owns runtime wiring and lifecycle.

Current snapshot runtime ownership:

- VdrService
- VdrSnapshotBuilder
- SnapshotCache
- SnapshotCacheService
- PollingService

## Change-State Based Polling Strategy

RESTfulAPI -> change-state endpoint -> VdrChangeState -> ChangeDetectionService -> SnapshotRefreshDecisionService -> VdrSnapshotBuilder -> SnapshotCacheService -> SnapshotCache

Goals:

- avoid unnecessary full snapshot rebuilds
- reduce HTTP traffic
- prepare efficient multi-VDR polling
- prepare later event dispatch

## RESTfulAPI Patch Direction

A RESTfulAPI patch exposes stable change-state counters or equivalent version information for VDR domains.

Domains:

- status
- channels
- recordings
- timers
- events

## Completed Phases

### Phase 8.90

Integrate change-state polling service.

### Phase 8.91

Generate backend-neutral VdrChangeEvent values from change-state transitions.

### Phase 8.92

Introduce SnapshotRefreshDecision and SnapshotRefreshDecisionService.

### Phase 8.93

Introduce SnapshotCache and SnapshotCacheService.

### Phase 8.94

Integrate SnapshotCacheService into PollingService and DaemonRuntime.

### Phase 8.94.1

Correct daemon runtime wiring and finalize runtime integration.

## Planned Phases

### Phase 8.95

Define snapshot access architecture for future snapshot-backed REST API responses.

### Phase 8.96

Move selected REST endpoints to snapshot-backed responses.

## Architectural Rules

- RESTfulAPI remains behind adapters.
- Controllers should not call RESTfulAPI directly.
- Daemon owns refresh logic.
- Snapshot models remain backend-neutral.
- PollingService owns refresh decisions.
- SnapshotCache owns current snapshot storage.
- SnapshotCacheService is the service boundary for current snapshot storage.
- Multi-VDR assumptions must remain possible.
- Event dispatch must be driven by backend-neutral VdrChangeEvent values.
