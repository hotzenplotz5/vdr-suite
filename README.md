# VDR-Suite

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, REST API foundations and future user interfaces.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Start Here

New to VDR-Suite?

Read:

- [VDR-Suite vision](docs/introduction/vdr-suite-vision.md)
- [Current project status](docs/development/current-status.md)
- [Core platform model](docs/architecture/vdr-suite-core-platform-model.md)

The vision document explains the long-term goals, architecture philosophy and future direction of the project.

---

## Current Status

Branch:

`phase-2-actions`

Latest runtime tag:

`v1.94.1-phase8-daemon-runtime-wiring`

Current architecture state:

- Phase 8.93 completed: snapshot cache model
- Phase 8.94 completed: snapshot cache integration
- Phase 8.94.1 completed: daemon runtime wiring correction

Implemented runtime direction:

- daemon runtime foundation
- VDR adapter abstraction
- RESTfulAPI integration
- change-state polling
- change event generation
- snapshot refresh decisions
- snapshot cache model
- snapshot cache runtime wiring

---

## Architecture Goals

VDR-Suite is designed to remain:

- VDR-centric
- backend-neutral
- service-oriented
- daemon-driven
- snapshot-oriented
- prepared for future multi-VDR environments

The architecture intentionally prepares, but does not yet implement:

- backend federation runtime
- cluster runtime
- user management
- permission runtime
- SSE runtime
- WebSocket runtime

---

## Current Runtime Flow

Current implemented flow:

RESTfulAPI
  -> /change-state.json
  -> RestfulApiVdrAdapter
  -> VdrChangeState
  -> VdrService
  -> PollingService
  -> ChangeDetectionService
  -> VdrChangeEvent
  -> SnapshotRefreshDecisionService
  -> VdrSnapshotBuilder
  -> SnapshotCacheService
  -> SnapshotCache
  -> DaemonRuntime
  -> future snapshot-backed APIs

Purpose:

- keep RESTfulAPI behind adapter boundaries
- avoid repeated live RESTfulAPI requests per API request
- keep polling and refresh decisions inside daemon-owned services
- keep snapshot storage separate from refresh logic
- prepare efficient future multi-VDR polling
- keep API controllers backend-independent

---

## Snapshot Architecture

Implemented components:

- `VdrSnapshot`
- `VdrSnapshotBuilder`
- `SnapshotCache`
- `SnapshotCacheService`

Responsibilities:

`VdrSnapshotBuilder`

- builds complete backend-neutral snapshots through `VdrService`

`SnapshotCache`

- owns the current daemon-visible VDR snapshot

`SnapshotCacheService`

- provides the service boundary around snapshot storage

`PollingService`

- reads change-state data
- detects changes
- asks for refresh decisions
- triggers snapshot rebuilds
- writes refreshed snapshots through `SnapshotCacheService`

`DaemonRuntime`

- owns runtime lifecycle
- owns `SnapshotCache`
- owns `SnapshotCacheService`
- initializes polling and snapshot runtime wiring

---

## Change Detection Architecture

Implemented components:

- `VdrChangeState`
- `VdrChangeEvent`
- `ChangeDetectionService`
- `SnapshotRefreshDecision`
- `SnapshotRefreshDecisionService`

Current model:

/change-state.json
  -> VdrChangeState
  -> ChangeDetectionService
  -> VdrChangeEvent
  -> SnapshotRefreshDecisionService
  -> snapshot refresh or no refresh

Goals:

- avoid unnecessary full snapshot rebuilds
- reduce RESTfulAPI traffic
- prepare efficient future multi-VDR polling
- prepare future event delivery without introducing SSE or WebSocket runtime prematurely

---

## Backend Architecture

Implemented abstractions:

- `IVdrAdapter`
- `VdrService`
- `VdrAdapterFactory`

Implemented backends:

- `MockVdrAdapter`
- `ExternalVdrAdapter`
- `RestfulApiVdrAdapter`

Current primary live integration path:

- `vdr-plugin-restfulapi`

Mapped VDR domain objects:

- `VdrStatus`
- `VdrChannel`
- `VdrRecording`
- `VdrTimer`
- `VdrEvent`
- `VdrChangeState`

RESTfulAPI endpoints currently used or prepared:

- `/info.json` -> `VdrStatus`
- `/recordings.json` -> `VdrRecording`
- `/timers.json` -> `VdrTimer`
- `/channels.json` -> `VdrChannel`
- `/events.json` -> `VdrEvent`
- `/change-state.json` -> `VdrChangeState`

---

## ADR Foundation

Accepted ADRs include:

- ADR-001 Backend Identity Strategy
- ADR-002 Backend Federation Strategy
- ADR-003 Backend Capability Strategy
- ADR-004 Backend Lifecycle Strategy
- ADR-005 Stream Provider Strategy

These ADRs prepare future:

- stable backend identity
- multi-VDR routing
- backend federation concepts
- backend capability negotiation
- backend lifecycle handling
- backend-neutral stream provider integration

without forcing those runtime systems into the current implementation too early.

---

## Implemented Areas

Database:

- SQLite wrapper
- database tests
- repository foundations

Recording domain:

- recordings
- metadata
- jobs
- actions
- dashboard services

REST API:

- API router
- dashboard controller
- jobs controller
- recordings controller
- metadata controller
- VDR controller

HTTP:

- HTTP request and response abstractions
- mock HTTP client
- basic HTTP client
- test HTTP server
- simple HTTP listener

VDR:

- domain objects
- adapter abstraction
- mock backend
- RESTfulAPI backend
- RESTfulAPI mappers
- VDR overview service
- VDR overview JSON serializer
- snapshot builder
- polling service
- change detection
- snapshot refresh decision model
- snapshot cache model

Daemon:

- runtime configuration
- database lifecycle
- REST runtime wiring
- VDR runtime wiring
- snapshot cache ownership

---

## Repository Structure

- `core/sqlite` – SQLite wrapper and database tests
- `core/recordings` – recordings, metadata, jobs, actions and dashboard services
- `core/http` – HTTP abstractions, clients, listener and test server
- `core/vdr` – VDR domain objects, adapters, mappers, overview, polling and snapshot services
- `core/daemon` – daemon runtime and lifecycle
- `api/rest` – REST controllers and API router
- `apps/dashboard` – dashboard CLI
- `apps/daemon` – daemon entry point
- `mk` – modular Makefile include files
- `docs` – project documentation, architecture notes and ADRs

---

## Build

Common targets:

- `make daemon`
- `make test`
- `make test-polling-service`
- `make test-snapshot-cache`
- `make test-snapshot-cache-service`
- `make test-vdr-snapshot-builder`
- `make test-restful-api-vdr-adapter`
- `make test-restful-api-change-state-adapter`

Use targeted tests during local architecture phases.

Use `make test` for release, tag, larger refactoring or build-system changes.

---

## Documentation

Important documentation:

- `docs/introduction/vdr-suite-vision.md`
- `docs/development/current-status.md`
- `docs/development/completed-phases.md`
- `docs/development/milestones.md`
- `docs/architecture/snapshot-architecture.md`
- `docs/architecture/vdr-backends.md`
- `docs/architecture/vdr-suite-core-platform-model.md`
- `docs/adr/`

---

## Near-Term Direction

Next planned phase:

### Phase 8.95 – Snapshot access architecture

Goal:

Define how API-facing layers should access daemon-owned snapshots without coupling REST controllers directly to cache internals or backend-specific adapters.

Scope:

- define snapshot access boundaries
- define read-only snapshot access service direction
- prepare snapshot-backed REST API responses
- preserve backend neutrality
- preserve future multi-VDR compatibility
- avoid premature federation runtime
- avoid premature SSE or WebSocket runtime
- avoid user-management and permission runtime

---

## License

See `LICENSE`.
