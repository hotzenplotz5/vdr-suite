# Partial Snapshot Refresh Architecture

## Purpose

This document reviews and defines the architecture direction for partial snapshot refresh support in VDR-Suite.

The goal is to avoid rebuilding the complete VDR snapshot when only one backend domain changed.

This is an architecture document only. It does not introduce runtime code yet.

---

## Current Runtime State

The current snapshot refresh flow is:

```text
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent
    ↓
SnapshotRefreshDecisionService
    ↓
SnapshotRefreshDecision
    ↓
VdrSnapshotBuilder::buildSnapshot()
    ↓
SnapshotCache::update(snapshot)
```

The current decision model supports:

```text
NoRefresh
FullRefresh
```

Any detected change currently results in a full snapshot rebuild.

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

## Current Limitation

The current `VdrSnapshotBuilder` has only one public build operation:

```text
buildSnapshot()
```

That method reads all domains and returns a complete snapshot.

The current `SnapshotCache` has only one update operation:

```text
update(snapshot)
```

That replaces the complete cached snapshot.

The current `SnapshotRefreshDecision` has only one refresh action:

```text
FullRefresh
```

Therefore the current architecture can detect domain-specific changes, but it cannot yet turn them into domain-specific snapshot updates.

---

## Target Direction

The target direction is to introduce an intermediate update plan between change events and snapshot updates.

Conceptual future flow:

```text
VdrChangeEvent[]
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
VdrSnapshotBuilder domain reads
    ↓
SnapshotCacheService domain update
    ↓
SnapshotCache
```

The plan should represent which snapshot domains need to be refreshed.

Example:

```text
TimersChanged
    ↓
SnapshotUpdatePlan { refreshTimers = true }
```

Multiple changes should be merged into one plan:

```text
ChannelsChanged
RecordingsChanged
    ↓
SnapshotUpdatePlan {
        refreshChannels = true,
        refreshRecordings = true
    }
```

---

## SnapshotUpdatePlan Concept

A future `SnapshotUpdatePlan` may include:

```text
refreshStatus
refreshChannels
refreshRecordings
refreshTimers
refreshEvents
requiresFullRefresh
```

`requiresFullRefresh` remains important for:

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

The future builder should support domain-specific reads while preserving the existing complete build operation.

Conceptual future operations:

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

The cache should continue to own one coherent `VdrSnapshot` per backend runtime.

Partial refresh should update selected domains inside the cached snapshot.

Conceptual future operations:

```text
update(snapshot)
updateStatus(status)
updateChannels(channels)
updateRecordings(recordings)
updateTimers(timers)
updateEvents(events)
```

The cache service should own these operations rather than letting higher layers mutate cache internals directly.

---

## PollingService Implications

`PollingService` should not decide domain update details itself.

It may continue to coordinate the polling cycle, but the mapping from `VdrChangeEvent` values to snapshot update work should move behind a planner or decision service.

This keeps `PollingService` from becoming tightly coupled to every snapshot domain.

---

## Internal Event Dispatch Relationship

The internal event dispatch architecture prepares the boundary for multiple event consumers.

Partial snapshot refresh should be treated as one internal consumer path:

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

This phase does not implement:

- partial refresh code
- cache mutation methods
- builder domain methods
- backend identity envelopes
- event dispatcher runtime
- SSE or WebSocket updates
- permission-aware event filtering
- multi-VDR snapshot stores

---

## Recommended Implementation Sequence

A safe future implementation order would be:

1. introduce a `SnapshotUpdatePlan` domain object
2. extend refresh decision logic into a planner that maps change events to update plans
3. add tests for event-to-plan mapping
4. add domain build methods to `VdrSnapshotBuilder`
5. add domain update methods to `SnapshotCacheService`
6. update `PollingService` to apply update plans
7. keep full refresh as fallback and first-poll behavior
8. only later connect this to an internal dispatcher if more consumers appear

---

## Architecture Decision

Partial snapshot refresh is architecturally feasible now.

The existing snapshot model and adapter boundary are already domain-separated.

The next implementation phase may safely introduce the plan model and planner tests without changing public API behavior.
