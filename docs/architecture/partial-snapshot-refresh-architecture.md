# Partial Snapshot Refresh Architecture

## Purpose

This document reviews and defines the architecture direction for partial snapshot refresh support in VDR-Suite.

The goal is to avoid rebuilding the complete VDR snapshot when only one backend domain changed.

The Phase 9 foundation is now partially implemented. The runtime already creates `SnapshotUpdatePlan` values during polling, but still executes a full snapshot refresh when the plan contains refresh work.

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
Full snapshot refresh execution
    ↓
VdrSnapshotBuilder::buildSnapshot()
    ↓
SnapshotCacheService::updateSnapshot(snapshot)
    ↓
SnapshotCache
```

The current runtime plan model supports domain refresh flags for:

```text
refreshStatus
refreshChannels
refreshRecordings
refreshTimers
refreshEvents
requiresFullSnapshot
```

Any detected change currently creates a domain-aware `SnapshotUpdatePlan`, but execution still falls back to a full snapshot rebuild.

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

Pending:

- executing generated update plans domain-by-domain
- keeping full refresh behavior for first poll and future recovery paths
- backend-context wrapping for future multi-VDR support

---

## Target Direction

The target direction is to execute generated update plans instead of rebuilding the complete snapshot for every backend-domain change.

Conceptual target flow:

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

The plan represents which snapshot domains need to be refreshed.

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

The builder now supports domain-specific reads while preserving the existing complete build operation.

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

Partial refresh should update selected domains inside the cached snapshot.

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

`PollingService` now creates update plans during the productive polling path:

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
```

When the plan contains refresh work, the runtime currently executes:

```text
SnapshotUpdatePlan
    ↓
VdrSnapshotBuilder::buildSnapshot()
    ↓
SnapshotCacheService::updateSnapshot(snapshot)
```

This preserves the previous full-refresh behavior while proving that the new plan path is active in runtime code.

The next implementation phase should execute the plan domain-by-domain.

---

## PollingService Implications

`PollingService` should not decide domain update details itself.

It may coordinate the polling cycle, but the mapping from `VdrChangeEvent` values to snapshot update work belongs behind `SnapshotRefreshPlanner` and `SnapshotUpdatePlan`.

This keeps `PollingService` from becoming tightly coupled to every snapshot domain.

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

Partial refresh can be introduced for the current single-backend runtime first.

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

The current Phase 9.4 runtime state does not yet implement:

- domain-by-domain plan execution
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

Next safe implementation step:

7. execute generated `SnapshotUpdatePlan` values through builder domain methods and cache service domain updates
8. keep full refresh as first-poll and recovery behavior
9. only later connect this to an internal dispatcher if more consumers appear

---

## Architecture Decision

Partial snapshot refresh is architecturally feasible and is now partially integrated into the runtime path.

The existing snapshot model, adapter boundary, builder methods, cache service methods and polling-plan integration are already domain-separated.

The next implementation phase may safely execute generated update plans without changing public API behavior.
