# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

---

## Current Branch

`phase-2-actions`

---

## Current Verified Head

`latest commit after phase 8.92`

Phase 8.92: introduce snapshot refresh decision model.

Latest milestone tag:

`v1.92-phase8-snapshot-refresh-decision`

Verified locally with:

```bash
make test-snapshot-refresh-decision-service
make test-polling-service
make test
```

---

## Last Completed Development State

Latest verified implementation state:

- Phase 8.69: PollingService interface
- Phase 8.70: PollingService implementation
- Phase 8.72: extract VDR source list into make include
- Phase 8.74: extract VDR test targets into make include
- Phase 8.75: extract HTTP source list into make include
- Phase 8.76: extract daemon source list into make include
- Phase 8.77: extract recording source lists into make include
- Phase 8.78: extract action and job source lists into make include
- Phase 8.79: initial root Makefile include conversion
- Phase 8.80: remove duplicate VDR test targets
- Phase 8.81: initialize `PollingService` in `DaemonRuntime`
- Phase 8.82: introduce `VdrChangeState` and `VdrChangeEvent`
- Phase 8.84: introduce `ChangeDetectionService`
- Phase 8.85: add change-state contract to `IVdrAdapter`
- Phase 8.86: add change-state support to `MockVdrAdapter` and `ExternalVdrAdapter`
- Phase 8.87: add and stub RESTfulAPI change-state support
- Phase 8.88: read RESTfulAPI `/change-state.json` endpoint
- Phase 8.89: add RESTfulAPI change-state adapter tests and include them in the global test suite
- Phase 8.90: integrate change-state polling service
- Phase 8.93: introduce snapshot cache model
- Phase 8.92: introduce snapshot refresh decision model

---

## Current Architecture Direction

VDR-Suite is moving from live RESTfulAPI access per API request toward a daemon-owned snapshot and change-detection architecture.

Current target chain:

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
VdrSnapshotBuilder
    ↓
Snapshot Cache
    ↓
API Responses / future live updates
```

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- prepare daemon-owned refresh cycles
- prepare efficient polling based on lightweight change-state checks
- prepare future SSE/WebSocket update delivery
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent

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
- `IVdrAdapter::getChangeState()`
- `VdrService::getChangeState()`
- `MockVdrAdapter::getChangeState()`
- `ExternalVdrAdapter::getChangeState()`
- `RestfulApiVdrAdapter::getChangeState()`

RESTfulAPI integration:

- `RestfulApiVdrAdapter` now requests `GET /change-state.json`
- the response is mapped into `VdrChangeState`
- dedicated adapter tests verify parsing, endpoint selection, HTTP error handling and invalid/incomplete JSON handling
- the dedicated change-state adapter test is part of the global `make test` suite
- supported fields:
  - `statusVersion`
  - `channelsVersion`
  - `recordingsVersion`
  - `timersVersion`
  - `eventsVersion`

Polling integration:

- `PollingService` now reads `VdrService::getChangeState()` before refresh decisions
- first poll always builds an initial snapshot
- unchanged change-state keeps the existing snapshot
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshDecisionService` converts change events into refresh decisions
- current refresh decisions support `NoRefresh` and `FullRefresh`
- full refresh decisions trigger a snapshot rebuild through `VdrSnapshotBuilder`
- `DaemonRuntime` now initializes `PollingService` with `VdrSnapshotBuilder` and `VdrService`

Verified targets include:

```bash
make daemon
make test-vdr-snapshot-builder
make test-snapshot-refresh-decision-service
make test-polling-service
make test-vdr-change-state
make test-change-detection-service
make test-snapshot-refresh-decision-service
make test-restful-api-vdr-adapter
make test-restful-api-change-state-adapter
make test
```

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

Accepted ADRs:

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

---

## Next Planned Phase

### Phase 8.93 – Snapshot cache model

Scope:

- introduce a daemon-owned snapshot cache model
- keep cache storage separate from refresh decisions
- prepare future BackendId-aware cache storage
- avoid introducing federation, SSE or WebSocket runtime objects prematurely

Required verification:

```bash
make test-snapshot-refresh-decision-service
make test-snapshot-refresh-decision-service
make test-polling-service
make test
```

Commit target:

```text
Phase 8.93: introduce snapshot cache model
```

---

## Upcoming Phases

### Phase 8.94 – Snapshot cache integration

Integrate snapshot cache access into polling and daemon runtime after the cache model is stable.

### Phase 8.95 – Snapshot-backed API responses

Introduce daemon-owned snapshot cache access as the basis for snapshot-backed API responses.

### Later Phases

- API responses from daemon snapshot cache
- SSE/WebSocket live update transport
- multi-VDR backend identity and routing
- backend-owned capability model for permission-aware frontends
- backend lifecycle handling for offline/reconnecting/failed/disabled states
- backend-neutral stream provider integration

---

## Split Documentation

Long-running historical sections were moved out of this status file.

See:

- `docs/development/completed-phases.md`
- `docs/development/milestones.md`
- `docs/architecture/snapshot-architecture.md`
- `docs/adr/adr-001-backend-identity-strategy.md`
- `docs/adr/adr-002-backend-federation-strategy.md`
- `docs/adr/adr-003-backend-capability-strategy.md`
- `docs/adr/adr-004-backend-lifecycle-strategy.md`
- `docs/adr/adr-005-stream-provider-strategy.md`

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
- Run tests before code commits when local build access is available.
