# VDR-Suite – Completed Phases

This file keeps completed implementation phase history out of `docs/development/current-status.md`.

Current verified state is tracked in:

- [Current Project Status](current-status.md)

Implementation milestone and tag history is tracked in:

- [Development Milestones](milestones.md)

Forward-looking planning is tracked in:

- [Planning Milestones](../planning/milestones.md)
- [Roadmap](../planning/roadmap.md)

---

## Phase 0 – Project and Documentation Foundation

Status: Completed

Result:

- repository structure established
- documentation structure introduced
- ADR process introduced
- initial project vision and architecture documented

---

## Phase 1 – Persistence and Core Backend Foundation

Status: Completed

Result:

- SQLite wrapper
- database schema foundation
- recording repository and service foundations
- metadata repository and service foundations
- job and action foundations
- core tests

---

## Phase 2 – Actions, Queue and Worker Foundation

Status: Completed

Result:

- recording action model
- job workflow foundation
- worker simulation
- Rectools adapter boundary
- queue-style processing foundation

---

## Phase 3 – Job Dashboard Service

Status: Completed

Result:

- job dashboard summary model
- job dashboard service
- tests for job dashboard aggregation

---

## Phase 4 – Recording Dashboard Service

Status: Completed

Result:

- recording dashboard summary model
- recording dashboard service
- tests for recording dashboard aggregation

---

## Phase 5 – Dashboard Facade

Status: Completed

Result:

- dashboard facade combining job and recording summaries
- service boundary for dashboard consumers

---

## Phase 6 – Dashboard JSON and CLI

Status: Completed

Result:

- dashboard JSON serializer
- dashboard CLI application
- initial command-line dashboard output

---

## Phase 7 – REST API Foundation

Status: Completed

Result:

- REST controller foundation
- API router
- dashboard route
- jobs route
- recordings route
- metadata route

---

## Phase 8 – VDR Backend Architecture Foundation

Status: Completed

Result:

- daemon foundation
- `VdrConfig`
- external VDR adapter foundation
- `IVdrAdapter`
- `VdrAdapterFactory`
- mock backend
- RESTfulAPI integration boundary
- HTTP abstraction layer
- VDR domain objects
- RESTfulAPI mappers
- backend identity, federation, capability, lifecycle and stream provider ADRs
- library-first and platform-oriented architecture direction

Selected later Phase 8 work:

- change-state polling integration
- backend-neutral `VdrChangeEvent`
- snapshot refresh decision model
- snapshot cache model
- snapshot access architecture preparation

---

## Phase 9 – Snapshot Runtime Validation

Status: Completed

Result:

- `VdrSnapshot`
- `VdrSnapshotBuilder`
- `PollingService`
- `SnapshotCache`
- `SnapshotCacheService`
- `SnapshotAccessService`
- snapshot-backed overview support
- change detection foundation
- snapshot refresh decision model
- snapshot refresh planner
- partial snapshot refresh validation
- local runtime validation against RESTfulAPI-oriented flows

---

## Phase 10 – Runtime Diagnostics and Runtime Wiring

Status: Completed

Result:

- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- runtime diagnostics JSON serialization
- runtime diagnostics REST endpoint
- runtime diagnostics summary endpoint
- bounded diagnostics retention
- runtime configuration cleanup
- HTTP client runtime measurements
- snapshot builder runtime measurements
- polling service runtime measurements
- status documentation split into focused documents

---

## Phase 11 – Snapshot Read APIs

Status: Completed for current domain set

Result:

- `VdrSnapshotReadService`
- `VdrSnapshotReadJsonSerializer`
- snapshot-backed VDR controller read methods
- snapshot read API router paths
- HTTP server coverage
- JSON serialization for:
  - status
  - channels
  - timers
  - events
  - recordings

Implemented read endpoints:

```text
GET /api/vdr/status
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
```

Latest completed sub-phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Verified with:

```text
make test-api-router
make test
```

---

## Current Transition

```text
Phase 11 complete
Documentation cleanup active
Phase 12.0 Snapshot Change Feed Architecture planned
```

Phase 12 should start with architecture review before adding live-update transport.
