# Internal Event Dispatch Architecture

## Purpose

This document defines the internal event dispatch direction for VDR-Suite after the completed snapshot and change-state architecture phases.

The goal is to prevent `PollingService` from becoming the permanent coordination point for every downstream reaction to `VdrChangeEvent` values.

The architecture is intentionally internal-only. It does not introduce SSE, WebSocket, frontend push updates, user notifications, federation runtime or multi-node messaging yet.

---

## Current Implemented Runtime Path

The current implemented path is:

```text
RESTfulAPI
    ↓
/change-state.json
    ↓
RestfulApiVdrAdapter
    ↓
VdrChangeState
    ↓
VdrService
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent
    ↓
SnapshotRefreshDecisionService
    ↓
VdrSnapshotBuilder
    ↓
SnapshotCacheService
    ↓
SnapshotCache
    ↓
SnapshotAccessService
    ↓
API-facing services and controllers
```

This path is correct for the current snapshot runtime.

However, `SnapshotRefreshDecisionService` is only one possible consumer of `VdrChangeEvent` values.

Future consumers may include:

- partial snapshot refresh planning
- backend lifecycle handling
- capability-aware backend routing
- frontend live update preparation
- SSE or WebSocket transport adapters
- audit, metrics or logging services
- multi-VDR event routing

---

## Architectural Problem

`VdrChangeEvent` already represents backend-neutral domain change information.

If future features are connected directly to `PollingService`, the service would gradually accumulate too many responsibilities:

- polling change-state
- detecting domain changes
- deciding snapshot refreshes
- triggering snapshot rebuilds
- publishing frontend updates
- handling backend lifecycle changes
- routing events by backend identity
- applying future permission or capability filters

That would make `PollingService` a hidden event bus and would make future multi-VDR support harder.

---

## Target Direction

The target direction is an internal event dispatch boundary between change detection and downstream consumers.

Conceptual future flow:

```text
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent
    ↓
InternalEventDispatcher
    ├── SnapshotRefreshDecisionService
    ├── future PartialSnapshotRefreshPlanner
    ├── future BackendLifecycleEventConsumer
    ├── future LiveUpdateEventConsumer
    ├── future AuditEventConsumer
    └── future MultiVdrEventRouter
```

The dispatcher is not a transport protocol.

It is a local in-process coordination boundary for daemon-owned domain events.

---

## Responsibilities

### PollingService

PollingService remains responsible for polling backend change-state and initiating change detection.

It should not become the permanent owner of all downstream event reactions.

### ChangeDetectionService

ChangeDetectionService remains responsible for comparing previous and current `VdrChangeState` values and producing backend-neutral `VdrChangeEvent` values.

### VdrChangeEvent

`VdrChangeEvent` remains the internal backend-neutral event model for domain changes.

The current event types are:

- status changed
- channels changed
- recordings changed
- timers changed
- events changed

Future extensions may add backend identity, source identity, timestamps or correlation data when multi-VDR support requires it.

### InternalEventDispatcher

A future `InternalEventDispatcher` should distribute internal domain events to registered daemon-owned consumers.

It should not expose public API transport details.

It should not know about HTTP, SSE, WebSocket, UI sessions or browser clients.

### SnapshotRefreshDecisionService

`SnapshotRefreshDecisionService` remains a consumer of change events.

It decides whether snapshot refresh work is needed.

It should not become the dispatcher.

### Future Live Transport Services

Future SSE or WebSocket services may consume prepared live update events.

They should not consume raw polling state directly.

They should not be called directly from `PollingService`.

---

## Non-Goals

This architecture phase does not implement:

- SSE
- WebSocket
- frontend subscriptions
- persistent event queues
- cross-process messaging
- distributed federation
- user-specific event filtering
- permission-aware live event delivery
- partial snapshot refresh execution

These remain future phases.

---

## Multi-VDR Implications

The current `VdrChangeEvent` model has no backend identity yet.

That is acceptable for the current single-runtime path.

Before true multi-VDR dispatch is implemented, internal events should be extended or wrapped with backend context.

Possible future context:

```text
BackendEventEnvelope
    backendId
    sourceId
    lifecycleState
    events[]
```

This should be introduced only when the backend identity and federation architecture require it in code.

---

## Snapshot Refresh Implications

The current refresh model supports:

- no refresh
- full refresh

The event dispatch boundary prepares the architecture for later partial refresh decisions without forcing them now.

Future decisions may distinguish:

- status-only refresh
- recordings-only refresh
- timers-only refresh
- EPG/events-only refresh
- full refresh

This should remain behind `SnapshotRefreshDecisionService` or a successor planner.

---

## Architectural Rules

- `VdrChangeEvent` is the internal event source for backend-neutral domain changes.
- `PollingService` must not become a general-purpose event bus.
- Snapshot refresh logic remains a consumer of change events, not the owner of the event model.
- Public live transports must not be coupled directly to polling internals.
- Future multi-VDR event dispatch must include backend identity before events cross backend boundaries.
- Event dispatch is daemon-owned and internal until a dedicated live transport architecture is introduced.

---

## Phase 8.99 Result

Phase 8.99 establishes the architectural direction only.

No runtime implementation is required in this phase.

The next implementation step may introduce a minimal internal dispatcher only when there is at least a second real consumer or when refactoring `PollingService` becomes necessary for partial refresh work.
