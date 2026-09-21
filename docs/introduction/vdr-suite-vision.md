# VDR-Suite Vision

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)

---

## What is VDR-Suite?

VDR-Suite is a modern service-oriented platform built around VDR.

Its goal is to provide recordings, timers, channels, EPG data, metadata, background processing and future VDR-related services through a unified architecture that can be consumed by web applications, desktop clients, mobile applications and VDR-integrated user interfaces.

VDR remains the primary backend and source of truth.

VDR-Suite does not replace VDR.

Instead, it extends VDR with a modern backend architecture designed for long-term maintainability, scalability and integration.

---

## Why VDR-Suite Exists

Traditional VDR installations expose functionality through a mixture of plugins, SVDRP commands, REST interfaces and OSD integrations.

While these approaches work well, they often require clients to communicate directly with backend components.

This creates tight coupling between frontends and backend implementations.

VDR-Suite introduces a stable service layer between clients and VDR.

Benefits include:

- backend abstraction
- centralized data access
- reduced API load
- consistent data models
- easier frontend development
- future multi-backend support
- improved maintainability

---

## Long-Term Vision

The long-term goal is to create a platform architecture that allows multiple frontend types to consume the same backend services.

```text
                 VDR-Suite

          ┌───────────────────┐
          │   Web Frontend    │
          ├───────────────────┤
          │ Windows Frontend  │
          ├───────────────────┤
          │ Android Frontend  │
          ├───────────────────┤
          │  iOS Frontend     │
          ├───────────────────┤
          │   VDR OSD Plugin  │
          └─────────┬─────────┘
                    │

          ┌─────────▼─────────┐
          │     REST API      │
          └─────────┬─────────┘
                    │

          ┌─────────▼─────────┐
          │ Snapshot Runtime  │
          └─────────┬─────────┘
                    │

          ┌─────────▼─────────┐
          │ Change Detection  │
          └─────────┬─────────┘
                    │

          ┌─────────▼─────────┐
          │   VDR Backend     │
          └───────────────────┘
```

All frontends access the same service layer.

The backend remains responsible for collecting and maintaining data.

---

## How VDR-Suite Works

VDR-Suite is built around a snapshot-based architecture.

Instead of requesting live data from VDR for every frontend request, the daemon monitors backend changes and maintains a cached snapshot.

Current architecture:

```text
VDR

↓
change-state.json

↓
VdrChangeState

↓
ChangeDetectionService

↓
VdrChangeEvent

↓
SnapshotRefreshDecision

↓
VdrSnapshotBuilder

↓
VdrSnapshot

↓
SnapshotCache

↓
SnapshotAccessService

↓
VdrOverviewService

↓
VdrController

↓
Browser / App / Frontend
```

### VDR

The actual VDR remains the source of truth.

This is where recordings, timers, channels, EPG data and runtime status originate.

### change-state.json

VDR-Suite does not need to request all backend data for every check.

It first asks whether something has changed.

For example, a changed timer version means that timer data should be considered outdated.

### VdrChangeState

`VdrChangeState` is the internal representation of backend version counters.

It describes the observed state of backend data groups such as recordings, timers, channels and events.

### ChangeDetectionService

`ChangeDetectionService` compares the previous change-state with the current one.

It determines which data groups have changed.

### VdrChangeEvent

A `VdrChangeEvent` describes what changed.

Examples include:

- `StatusChanged`
- `ChannelsChanged`
- `RecordingsChanged`
- `TimersChanged`
- `EventsChanged`

### SnapshotRefreshDecision

`SnapshotRefreshDecisionService` decides whether a snapshot refresh is needed.

The current model supports no refresh or full refresh.

Future versions may support more granular partial refresh decisions.

### VdrSnapshotBuilder

`VdrSnapshotBuilder` collects the required backend data through `VdrService` and builds a complete backend-neutral snapshot.

### VdrSnapshot

`VdrSnapshot` is a moment-in-time representation of the backend state.

It contains status, recordings, timers, channels and events.

### SnapshotCache

`SnapshotCache` stores the current daemon-visible snapshot.

Frontend-facing APIs can use this cached state without repeatedly querying the live backend.

### SnapshotAccessService

`SnapshotAccessService` provides a read boundary around cached snapshot access.

API-facing services do not need to know how the snapshot is stored or refreshed.

### VdrOverviewService

`VdrOverviewService` converts raw snapshot data into an overview model suitable for API responses.

### VdrController

`VdrController` exposes the VDR overview through the REST API.

It remains independent from backend-specific adapter details and snapshot cache internals.

---

## Why Snapshots?

Traditional approaches often request live data for every API request.

```text
Browser
  ↓
REST API
  ↓
RESTfulAPI
  ↓
VDR
```

This causes repeated backend access.

VDR-Suite instead keeps a cached representation of the backend state.

```text
Browser
  ↓
REST API
  ↓
Snapshot
```

The backend is queried when changes are detected.

Benefits:

- lower backend load
- faster API responses
- better scalability
- cleaner separation of responsibilities
- easier frontend development
- better preparation for future multi-VDR support

---

## Future Multi-VDR Architecture

VDR-Suite is currently focused on a single backend.

However, the architecture is intentionally designed to allow future expansion.

Conceptually:

```text
                VDR-Suite

                      │

      ┌───────────────┼───────────────┐
      │               │               │

   VDR #1         VDR #2         VDR #3
```

Multi-VDR support is not implemented today.

The architecture merely avoids assumptions that would prevent it in the future.

Future backend-aware runtime concepts may include:

- stable backend identity
- backend lifecycle state
- backend capabilities
- backend-specific snapshots
- backend routing
- frontend-visible capability discovery

These concepts are prepared architecturally but should not be implemented prematurely.

---

## What VDR-Suite Is Not

VDR-Suite intentionally avoids becoming a general-purpose media platform.

It is not:

- a Plex clone
- a replacement for VDR
- a TVHeadend replacement
- a generic media server

Instead, it is:

- VDR-centric
- service-oriented
- backend-neutral
- extensible
- snapshot-based
- designed for long-term evolution

---

## Architecture Principles

The architecture follows several core principles.

### Backend Neutrality

Business logic should not depend on specific backend implementations.

### VDR-Centric Design

VDR remains the primary source of truth.

### Service Orientation

Functionality is exposed through well-defined services.

### Snapshot-Based Data Access

Frontend requests operate on cached snapshots instead of direct backend queries.

### Future Multi-VDR Readiness

The architecture avoids assumptions that would block future multi-backend scenarios.

### Capability-Based Evolution

Future clients should discover backend capabilities instead of backend implementation types.

### Lifecycle Awareness

Backend availability and state transitions are considered architectural concerns.

### Stream Provider Neutrality

Future stream handling should not permanently assume a single backend technology.

---

## Project Goal

The ultimate goal of VDR-Suite is to provide a modern, maintainable and extensible platform around VDR while preserving the strengths, stability and flexibility that have made VDR successful for many years.

VDR-Suite aims to become a modern and extensible platform around VDR while remaining fully VDR-centric.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)

---

## Further Reading

- [Documentation Index](../index.md)
- [Current project status](../development/current-status.md)
- [Snapshot architecture](../architecture/snapshot-architecture.md)
- [Snapshot access architecture](../architecture/snapshot-access-architecture.md)
- [VDR backend architecture](../architecture/vdr-backends.md)
- [Core platform model](../architecture/vdr-suite-core-platform-model.md)
- [Architecture decision records](../adr/)
