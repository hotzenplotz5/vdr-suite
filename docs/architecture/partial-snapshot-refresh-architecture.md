# Partial Snapshot Refresh Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document reviews and defines the architecture direction for partial snapshot refresh support in VDR-Suite.

The goal is to avoid rebuilding the complete VDR snapshot when only one backend domain changed.

The Phase 9 partial refresh runtime is now implemented for the current single-backend snapshot path. The runtime creates `SnapshotUpdatePlan` values during polling and executes them through domain-specific builder reads and cache service updates.

---

## Current Runtime State

The current snapshot refresh flow is:

```text
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent[]
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
Plan execution
        ├─ buildStatus()     -> updateStatus()
        ├─ buildRecordings() -> updateRecordings()
        ├─ buildTimers()     -> updateTimers()
        ├─ buildChannels()   -> updateChannels()
        └─ buildEvents()     -> updateEvents()
    ↓
SnapshotCacheService
    ↓
SnapshotCache
```

The runtime plan model supports domain refresh flags for:

```text
refreshStatus
refreshChannels
refreshRecordings
refreshTimers
refreshEvents
requiresFullSnapshot
```

Detected backend-domain changes now create a domain-aware `SnapshotUpdatePlan`, and the plan is executed domain-by-domain.

---

## Existing Snapshot Shape

The current snapshot model is already domain-separated:

```text
VdrSnapshot
    status
    recordings[]
    timers[]
    channels[]
    events[]
```

This is a strong foundation for partial refresh work.

Partial refresh does not require splitting the top-level snapshot model.

---

## Existing Service and Adapter Shape

The current VDR service and adapter boundary is also domain-separated:

```text
getStatus()
getChannels()
getEvents()
getTimers()
getRecordings()
getChangeState()
```

This means partial refresh can be introduced without changing the fundamental adapter contract.

The existing backend abstraction already supports domain-specific reads.

---

## Implemented Phase 9 Foundation

Implemented:

- `SnapshotUpdatePlan`
- `SnapshotRefreshPlanner`
- event-to-plan mapping for status, channels, recordings, timers and events
- `VdrSnapshotBuilder` domain methods:
  - `buildStatus()`
  - `buildRecordings()`
  - `buildTimers()`
  - `buildChannels()`
  - `buildEvents()`
- `SnapshotCacheService` domain update methods:
  - `updateSnapshot()`
  - `updateStatus()`
  - `updateRecordings()`
  - `updateTimers()`
  - `updateChannels()`
  - `updateEvents()`
  - `clear()`
- runtime creation of `SnapshotUpdatePlan` values inside `PollingService`
- `PollingService::lastUpdatePlan()` for test and diagnostic visibility
- domain-by-domain execution of generated update plans
- full snapshot behavior for first poll and future recovery paths

Pending:

- optional local VDR integration tests
- backend-context wrapping for future multi-VDR support

---

## Target Direction

The Phase 9 target direction has been reached for the current single-backend runtime path: generated update plans are executed instead of rebuilding the complete snapshot for every backend-domain change.

Implemented runtime flow:

```text
VdrChangeEvent[]
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
VdrSnapshotBuilder domain reads
    ↓
SnapshotCacheService domain updates
    ↓
SnapshotCache
```

Example:

```text
TimersChanged
    ↓
SnapshotUpdatePlan { refreshTimers = true }
    ↓
VdrSnapshotBuilder::buildTimers()
    ↓
SnapshotCacheService::updateTimers(timers)
```

Multiple changes are merged into one plan:

```text
ChannelsChanged
RecordingsChanged
    ↓
SnapshotUpdatePlan {
        refreshChannels = true,
        refreshRecordings = true
    }
    ↓
buildChannels() + updateChannels()
buildRecordings() + updateRecordings()
```

---

## SnapshotUpdatePlan Concept

`SnapshotUpdatePlan` includes domain refresh flags for:

```text
refreshStatus
refreshChannels
refreshRecordings
refreshTimers
refreshEvents
requiresFullSnapshot
```

`requiresFullSnapshot` remains important for:

- first snapshot creation
- cache recovery
- incompatible model changes
- backend reconnects
- future lifecycle transitions where partial state cannot be trusted

---

## Refresh Mapping

Initial mapping direction:

```text
StatusChanged      -> refreshStatus
ChannelsChanged    -> refreshChannels
RecordingsChanged  -> refreshRecordings
TimersChanged      -> refreshTimers
EventsChanged      -> refreshEvents
```

This mapping should remain conservative.

Some domains may later imply dependent refreshes.

Examples:

- channel changes may require event refreshes if EPG data is channel-indexed
- backend reconnects may require a full refresh
- lifecycle recovery may invalidate cached state

These dependency rules should belong to the refresh planner, not to `PollingService`.

---

## Builder Implications

The builder supports domain-specific reads while preserving the existing complete build operation.

Implemented operations:

```text
buildSnapshot()
buildStatus()
buildChannels()
buildRecordings()
buildTimers()
buildEvents()
```

`buildSnapshot()` remains useful for:

- first poll
- full refresh
- tests
- recovery paths

Domain build operations support partial updates.

---

## Cache Implications

The cache continues to own one coherent `VdrSnapshot` per backend runtime.

Partial refresh updates selected domains inside the cached snapshot.

Implemented service operations:

```text
updateSnapshot(snapshot)
updateStatus(status)
updateChannels(channels)
updateRecordings(recordings)
updateTimers(timers)
updateEvents(events)
clear()
```

The cache service owns these operations rather than letting higher layers mutate cache internals directly.

`SnapshotCache` remains a simple storage container.

---

## Runtime Integration Status

`PollingService` creates and executes update plans during the productive polling path:

```text
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent[]
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
Domain-specific plan execution
```

When the plan contains refresh work, the runtime executes only the selected domains:

```text
SnapshotUpdatePlan
    ↓
VdrSnapshotBuilder::buildStatus()/buildChannels()/...
    ↓
SnapshotCacheService::updateStatus()/updateChannels()/...
```

This replaces the previous "any change means full snapshot rebuild" runtime behavior for normal domain changes.

Full snapshot rebuild remains preserved for first poll and future recovery paths.

---

## PollingService Implications

`PollingService` coordinates the polling cycle and executes the generated update plan.

The mapping from `VdrChangeEvent` values to snapshot update work belongs behind `SnapshotRefreshPlanner` and `SnapshotUpdatePlan`.

This keeps `PollingService` from owning event-to-domain mapping rules directly.

---

## Internal Event Dispatch Relationship

The internal event dispatch architecture prepares the boundary for multiple event consumers.

Partial snapshot refresh is one internal consumer path:

```text
VdrChangeEvent[]
    ↓
InternalEventDispatcher or direct planner boundary
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
```

A dispatcher does not need to be implemented before partial refresh if snapshot refresh remains the only real event consumer.

However, the partial refresh design should not block a future dispatcher.

---

## Multi-VDR Implications

The current snapshot cache owns a single snapshot.

Partial refresh is implemented for the current single-backend runtime first.

For future multi-VDR support, the same idea should apply per backend:

```text
Backend A -> SnapshotUpdatePlan -> Snapshot A
Backend B -> SnapshotUpdatePlan -> Snapshot B
Backend C -> SnapshotUpdatePlan -> Snapshot C
```

Before events cross backend boundaries, they must carry or be wrapped with backend identity.

This remains aligned with the backend identity, federation and lifecycle ADRs.

---

## Non-Goals

The current Phase 9.5 runtime state does not yet implement:

- local VDR integration test automation
- backend identity envelopes
- event dispatcher runtime
- SSE or WebSocket updates
- permission-aware event filtering
- multi-VDR snapshot stores

---

## Recommended Implementation Sequence

Implemented sequence so far:

1. introduce a `SnapshotUpdatePlan` domain object
2. introduce `SnapshotRefreshPlanner` to map change events to update plans
3. add tests for event-to-plan mapping
4. add domain build methods to `VdrSnapshotBuilder`
5. add domain update methods to `SnapshotCacheService`
6. update `PollingService` to create update plans at runtime
7. execute generated `SnapshotUpdatePlan` values through builder domain methods and cache service domain updates
8. keep full refresh as first-poll and recovery behavior

Next safe implementation step:

9. introduce optional local VDR/RESTfulAPI integration tests
10. only later connect this to an internal dispatcher if more consumers appear

---

## Architecture Decision

Partial snapshot refresh is now implemented for the current single-backend polling runtime.

The existing snapshot model, adapter boundary, builder methods, cache service methods, planner and polling execution path are domain-separated.

The next implementation phase may safely introduce optional local VDR integration tests without changing public API behavior.
---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
