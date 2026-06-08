# VDR-Suite Roadmap

This roadmap describes the current development direction of VDR-Suite.

Detailed implementation status lives in:

- [Current Project Status](../development/current-status.md)
- [Phase 11 Snapshot Read APIs](../development/phase-11-snapshot-read-apis.md)
- [Planning Milestones](milestones.md)
- [Development Milestones](../development/milestones.md)
- [Documentation Index](../index.md)

---

## Roadmap Principles

VDR-Suite remains:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for future multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Completed Foundation

### Phase 0 – Project and Architecture Foundation

Status: Completed

Result:

- repository structure established
- documentation structure introduced
- initial architecture documented
- ADR process introduced
- development rules documented

### Phase 1 – Persistence and Core Services

Status: Completed

Result:

- SQLite wrapper
- database schema
- recording repository and services
- metadata repository and services
- job and action foundations
- core service tests

### Phase 2 – Actions and Worker Foundation

Status: Completed

Result:

- action model
- job workflow foundation
- worker simulation
- Rectools adapter boundary

### Phase 3 to Phase 7 – Dashboard and REST API Foundation

Status: Completed

Result:

- dashboard services
- dashboard facade
- dashboard JSON serialization
- dashboard CLI
- REST controllers
- API router
- initial dashboard, jobs, recordings and metadata endpoints

---

## Completed VDR Backend and Snapshot Architecture

### Phase 8 – VDR Backend Architecture

Status: Completed foundation, still evolving through later phases

Result:

- external VDR adapter foundation
- `VdrConfig`
- `IVdrAdapter`
- `VdrAdapterFactory`
- mock backend
- RESTfulAPI integration architecture
- HTTP abstraction layer
- VDR domain objects
- RESTfulAPI mappers
- backend identity and federation ADRs
- platform API ADR

### Phase 9 – Snapshot Runtime Validation

Status: Completed

Result:

- VDR snapshot model
- snapshot builder
- polling integration
- change-state polling integration
- snapshot refresh decision model
- snapshot refresh planner
- partial refresh validation
- local RESTfulAPI runtime validation

### Phase 10 – Runtime Diagnostics and Runtime Wiring

Status: Completed

Result:

- structured runtime measurement boundary
- runtime diagnostics service
- HTTP client measurement collection
- snapshot builder measurement collection
- polling service measurement collection
- runtime diagnostics JSON serialization
- runtime diagnostics REST endpoints
- runtime diagnostics summary endpoint
- bounded diagnostics retention
- runtime configuration cleanup
- status documentation split

### Phase 11 – Snapshot Read APIs

Status: Completed for current domain set

Result:

- `VdrSnapshotReadService`
- `VdrSnapshotReadJsonSerializer`
- snapshot-backed controller methods
- snapshot read API routing
- HTTP server coverage
- real JSON serialization for:
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

---

## Current Roadmap Position

Current completed phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Current transition:

```text
Phase 11 complete
↓
Documentation cleanup
↓
Phase 12.0 Snapshot Change Feed Architecture
```

---

## Active Documentation Cleanup

Status: Active

Goal:

Make repository documentation navigable from the README and from `docs/index.md`.

Tasks:

- keep `README.md` as the stable project entry point
- make `docs/index.md` the complete documentation hub
- keep this roadmap focused on direction
- keep milestones focused on completion criteria
- keep `current-status.md` focused on the current verified state
- ensure architecture documents and ADRs are reachable from the index
- avoid duplicating long phase histories in README

---

## Next Planned Architecture Phase

### Phase 12.0 – Snapshot Change Feed Architecture

Status: Planned

Goal:

Define a read-oriented change feed on top of the existing snapshot and change-detection domain.

Motivation:

Phase 11 made snapshot data readable by future clients. Phase 12 should make it possible for clients to know what changed without reloading every domain endpoint repeatedly.

Architectural prerequisites:

- ADR-0014 Recording Identity Strategy defines the stable recording identity requirements that future snapshot change feeds must preserve.
- Change detection must remain compatible with future backend federation and multi-VDR routing.

Expected direction:

- review `VdrChangeState`
- review `VdrChangeEvent`
- review `ChangeDetectionService`
- define an internal snapshot change feed model
- keep feed generation separate from HTTP transport
- keep multi-VDR and backend identity requirements visible
- align recording change detection with ADR-0014
- avoid SSE/WebSocket transport in the first step

Expected result:

A stable internal architecture for snapshot change information that can later be exposed through REST, SSE or WebSocket transport.

---

## Later Roadmap

### Phase 12.x – Snapshot Change Feed Implementation

Planned direction:

- implement change feed service
- add tests for feed generation
- add REST endpoint for read-only change feed access
- keep transport-independent design

### Phase 13.x – Live Update Transport

Planned direction:

- evaluate SSE and WebSocket transport
- expose snapshot change events to frontends
- keep backend-neutral event delivery
- avoid coupling event generation to a single frontend

### Phase 14.x – Multi-VDR Backend Routing

Planned direction:

- introduce backend identity into read paths where required
- prepare snapshot access for multiple VDR instances
- preserve permission-aware future architecture
- align with backend federation ADRs

### Phase 15.x – Frontend-Oriented API Hardening

Planned direction:

- filtering
- pagination
- stable error responses
- capability-aware API responses
- frontend contract documentation

### Later Product Phases

Possible later product layers:

- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD integration
- stream provider integration
- media library expansion
- search and metadata enrichment

---

## Non-Goals For The Immediate Next Phase

The next phase should not immediately implement:

- SSE transport
- WebSocket transport
- frontend UI
- write APIs
- permissions
- multi-backend federation runtime

Those remain important, but they need the internal change feed foundation first.