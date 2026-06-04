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

`5fab49d`

Phase 8.88: read RESTfulAPI change-state endpoint.

Verified locally with:

```bash
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
ChangeDetectionService
    ↓
PollingService
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
- `IVdrAdapter::getChangeState()`
- `MockVdrAdapter::getChangeState()`
- `ExternalVdrAdapter::getChangeState()`
- `RestfulApiVdrAdapter::getChangeState()`

RESTfulAPI integration:

- `RestfulApiVdrAdapter` now requests `GET /change-state.json`
- the response is mapped into `VdrChangeState`
- supported fields:
  - `statusVersion`
  - `channelsVersion`
  - `recordingsVersion`
  - `timersVersion`
  - `eventsVersion`

Verified targets include:

```bash
make daemon
make test-vdr-snapshot-builder
make test-polling-service
make test-vdr-change-state
make test-change-detection-service
make test-restful-api-vdr-adapter
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

## Current Known Technical Debt

Current change-state parsing inside `RestfulApiVdrAdapter` uses a small local integer-field parser.

This is acceptable for the current minimal endpoint shape, but may later be replaced by a dedicated mapper if the endpoint grows or more RESTfulAPI JSON parsing is consolidated.

The unversioned local directory `vdr-suite-integration-lab/` exists in the working tree and is intentionally not part of the repository state.

---

## Next Planned Phase

### Phase 8.89 – RESTfulAPI change-state adapter tests

Scope:

- add tests for successful `/change-state.json` parsing
- verify `statusVersion`, `channelsVersion`, `recordingsVersion`, `timersVersion`, and `eventsVersion`
- verify HTTP error handling returns an empty change state
- verify invalid or incomplete JSON is tolerated

Required verification:

```bash
make test-restful-api-vdr-adapter
make test
```

Commit target:

```text
Phase 8.89: add RESTfulAPI change-state adapter tests
```

---

## Upcoming Phases

### Phase 8.90 – PollingService change-state integration

Use `IVdrAdapter::getChangeState()` inside `PollingService` so snapshot refreshes can be driven by lightweight backend change information.

### Phase 8.91 – ChangeDetectionService integration

Compare previous and current `VdrChangeState` objects and classify which backend domains changed.

### Phase 8.92 – VdrChangeEvent generation

Generate `VdrChangeEvent` objects from detected backend changes for future live update delivery.

### Later Phases

- daemon-owned snapshot cache access
- API responses from daemon snapshot cache
- SSE/WebSocket live update transport
- multi-VDR backend identity and routing
- backend-owned capability model for permission-aware frontends

---

## Split Documentation

Long-running historical sections were moved out of this status file.

See:

- `docs/development/completed-phases.md`
- `docs/development/milestones.md`
- `docs/architecture/snapshot-architecture.md`

---


## Architecture Decisions

- ADR-001 Backend Identity Strategy
- ADR-002 Backend Federation Strategy
- ADR-003 Backend Capability Strategy
- ADR-004 Backend Lifecycle Strategy

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
