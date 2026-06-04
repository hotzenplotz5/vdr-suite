# ADR-006 Internal Event Dispatch Strategy

## Status

Accepted

## Context

VDR-Suite has introduced a daemon-owned snapshot and change-state runtime path.

The current implemented flow detects backend-neutral domain changes and turns them into `VdrChangeEvent` values.

Those events are currently consumed directly by `SnapshotRefreshDecisionService` to decide whether the snapshot should remain unchanged or be rebuilt.

This is correct for the current snapshot runtime, but it creates a future architectural risk.

`VdrChangeEvent` is not only a snapshot-refresh input. It is also the natural internal event model for future daemon-owned reactions such as partial snapshot refresh planning, backend lifecycle handling, live update preparation, audit logging and multi-VDR routing.

If every future reaction is connected directly to `PollingService`, `PollingService` would become a hidden event bus.

That would make the architecture harder to extend and would weaken the separation between polling, event detection, refresh planning and future live transport layers.

---

## Decision

VDR-Suite shall treat `VdrChangeEvent` as the backend-neutral internal event model for daemon-owned backend domain changes.

Snapshot refresh decision logic shall be treated as one consumer of those events, not as the owner of the event model.

The architecture shall prepare an internal event dispatch boundary between change detection and downstream consumers.

The future conceptual structure is:

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

The internal dispatcher is an in-process daemon architecture boundary.

It is not a public transport mechanism and does not imply SSE, WebSocket, federation, frontend subscriptions or cross-process messaging.

---

## Rules

- `VdrChangeEvent` remains backend-neutral.
- `PollingService` remains responsible for polling and initiating change detection.
- `PollingService` must not become a general-purpose event bus.
- `SnapshotRefreshDecisionService` remains a consumer of change events.
- Future live transports must not depend directly on polling internals.
- Future multi-VDR dispatch must include backend identity before events cross backend boundaries.
- Event dispatch remains daemon-owned and internal until a dedicated live transport architecture is introduced.

---

## Consequences

Benefits:

- keeps polling, change detection and downstream event reactions separate
- prepares partial snapshot refresh decisions
- prepares future SSE or WebSocket architecture without coupling it to polling
- prepares backend lifecycle event handling
- prepares multi-VDR event routing
- keeps snapshot refresh logic replaceable and extendable

Tradeoffs:

- introduces another architecture concept before code requires it
- may require a small future refactor of `PollingService`
- should not be implemented prematurely without a second real consumer

---

## Non-Goals

This ADR does not introduce:

- SSE
- WebSocket
- frontend subscriptions
- persistent queues
- distributed event buses
- cross-process messaging
- user-specific event filtering
- permission-aware live delivery
- partial snapshot refresh execution

---

## Implementation Guidance

No immediate runtime implementation is required for this ADR.

A minimal `InternalEventDispatcher` should only be introduced when at least one of the following becomes true:

- snapshot refresh decisions are no longer the only consumer of `VdrChangeEvent`
- partial snapshot refresh planning requires a cleaner separation
- backend lifecycle events need to be handled from the same event stream
- live update preparation needs an internal event source
- multi-VDR event routing requires backend context around events

Until then, the current direct `PollingService` to `SnapshotRefreshDecisionService` path may remain in code.

---

## Related Documents

- `docs/architecture/snapshot-architecture.md`
- `docs/architecture/snapshot-access-architecture.md`
- `docs/architecture/internal-event-dispatch-architecture.md`
- ADR-001 Backend Identity Strategy
- ADR-002 Backend Federation Strategy
- ADR-003 Backend Capability Strategy
- ADR-004 Backend Lifecycle Strategy
