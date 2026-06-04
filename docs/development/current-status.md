# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

`phase-2-actions`

---

## Current Verified Head

`f8ceef4`

Latest milestone tag:

`v1.94.1-phase8-daemon-runtime-wiring`

Latest completed phase:

Phase 8.94: integrate snapshot cache into polling and daemon runtime.

Verified locally with:

- `make daemon`
- `make test-polling-service`
- `make test-snapshot-cache`
- `make test-snapshot-cache-service`

---

## Current Architecture State

VDR-Suite is moving from direct live RESTfulAPI access per API request toward a daemon-owned snapshot and change-detection architecture.

Current implemented chain:

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
DaemonRuntime
    ↓
future snapshot-backed API responses

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- keep refresh decisions inside daemon-owned services
- keep snapshot storage separate from polling logic
- prepare efficient polling based on lightweight change-state checks
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent
- avoid premature federation, SSE, WebSocket, user-management or cluster runtime implementation

---

## Implemented Snapshot and Change-State Components

Implemented:

- `VdrSnapshot`
- `VdrSnapshotBuilder`
- `PollingService`
- `VdrChangeState`
- `VdrChangeEvent`
- `ChangeDetectionService`
- `SnapshotRefreshDecision`
- `SnapshotRefreshDecisionService`
- `SnapshotCache`
- `SnapshotCacheService`
- `IVdrAdapter::getChangeState()`
- `VdrService::getChangeState()`
- `MockVdrAdapter::getChangeState()`
- `ExternalVdrAdapter::getChangeState()`
- `RestfulApiVdrAdapter::getChangeState()`

RESTfulAPI integration:

- `RestfulApiVdrAdapter` requests `GET /change-state.json`
- the response is mapped into `VdrChangeState`
- supported fields:
  - `statusVersion`
  - `channelsVersion`
  - `recordingsVersion`
  - `timersVersion`
  - `eventsVersion`
- adapter tests verify parsing, endpoint selection, HTTP error handling and invalid/incomplete JSON handling

Polling integration:

- `PollingService` reads `VdrService::getChangeState()` before refresh decisions
- first poll builds an initial snapshot
- unchanged change-state keeps the existing cached snapshot
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshDecisionService` converts change events into refresh decisions
- current refresh decisions support `NoRefresh` and `FullRefresh`
- full refresh decisions trigger a snapshot rebuild through `VdrSnapshotBuilder`
- rebuilt snapshots are written through `SnapshotCacheService`
- `DaemonRuntime` owns `SnapshotCache`, `SnapshotCacheService`, `VdrSnapshotBuilder` and `PollingService`

---

## Completed Recent Phases

- Phase 8.90: change-state polling integration
- Phase 8.91: change event generation
- Phase 8.92: snapshot refresh decision model
- Phase 8.93: snapshot cache model
- Phase 8.94: snapshot cache integration into polling runtime
- Phase 8.94.1: daemon runtime wiring correction

---

## RESTfulAPI Patch Dependency

VDR-Suite can consume `/change-state.json` through `RestfulApiVdrAdapter`.

The endpoint is provided by the patched RESTfulAPI fork/branch and has also been submitted upstream as a pull request.

Expected RESTfulAPI response shape:

```json
{
  "statusVersion": 0,
  "channelsVersion": 2,
  "recordingsVersion": 0,
  "timersVersion": 0,
  "eventsVersion": 0
}
```

The adapter tolerates HTTP errors or malformed/missing fields by returning a default empty `VdrChangeState`.

---

## Architecture Decisions

Accepted ADRs include:

- ADR-001 Backend Identity Strategy
- ADR-002 Backend Federation Strategy
- ADR-003 Backend Capability Strategy
- ADR-004 Backend Lifecycle Strategy
- ADR-005 Stream Provider Strategy

Architectural impact:

- future backends must not be identified by hostnames or IP addresses alone
- future multi-VDR support requires stable backend identity and backend federation
- frontends should query backend capabilities instead of checking backend implementation types
- backend lifecycle states must be considered by future polling, snapshot and event-delivery logic
- stream handling must remain backend-neutral and must not permanently assume DVB/VDR as the only source

---

## Current Known Technical Debt

Current change-state parsing inside `RestfulApiVdrAdapter` uses a small local integer-field parser.

This is acceptable for the current minimal endpoint shape, but may later be replaced by a dedicated mapper if the endpoint grows or more RESTfulAPI JSON parsing is consolidated.

The unversioned local directory `vdr-suite-integration-lab/` exists in the working tree and is intentionally not part of the repository state.

Documentation debt remains:

- `README.md` is outdated and must be rebuilt for the current Phase 8.94 architecture.
- `docs/architecture/snapshot-architecture.md` should be updated to mark Phase 8.94 as completed.

---

## Next Planned Phase

### Phase 8.95 – Snapshot access architecture

Goal:

Define how API-facing layers should access daemon-owned snapshots without coupling controllers directly to cache internals or backend-specific adapters.

Scope:

- define snapshot access boundaries
- define read-only snapshot access service direction
- prepare snapshot-backed REST API responses
- preserve backend neutrality
- preserve future multi-VDR compatibility
- avoid premature federation runtime
- avoid premature SSE/WebSocket runtime
- avoid user management and permission runtime

Expected result:

A documented architecture for snapshot-backed API access before modifying REST controllers.

---

## Upcoming Phases

### Phase 8.96 – Snapshot-backed API responses

Introduce daemon-owned snapshot access as the basis for VDR API responses.

### Later Phases

- partial snapshot refresh decisions
- event dispatch preparation
- SSE/WebSocket live update transport
- multi-VDR backend identity and routing
- backend-owned capability model for permission-aware frontends
- backend lifecycle handling for offline/reconnecting/failed/disabled states
- backend-neutral stream provider integration

---

## Split Documentation

Long-running historical sections are kept out of this status file.

See:

- `docs/development/completed-phases.md`
- `docs/development/milestones.md`
- `docs/architecture/snapshot-architecture.md`
- `docs/architecture/vdr-backends.md`
- `docs/architecture/vdr-suite-core-platform-model.md`
- `docs/adr/`

---

## Project Rules

- Architecture first.
- Read existing code before code changes.
- No placeholders.
- No dummy implementations.
- No permanent single-VDR assumption.
- Prefer complete files for local changes.
- Use nano-compatible workflows for local instructions.
- No `cat <<EOF` blocks in local instructions.
- Keep builds working after each small change.
- Run targeted tests before code commits when local build access is available.
