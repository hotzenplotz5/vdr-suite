# VDR-Suite – Current Project Status

## Introduction

New contributors should start with:

- `docs/introduction/vdr-suite-vision.md`
- `README.md`
- [Documentation Index](../index.md)

The vision document explains why VDR-Suite exists, the long-term goals of the project, the snapshot architecture philosophy and the future direction of the platform.

---

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

`bc6a7dd`

Latest completed phase:

Phase 10.5: Snapshot Builder Timing Logs.

Verified locally with:

- `make test-vdr-snapshot-builder`
- `make test`

Important architecture note:

Phase 10.5 extends the runtime observability boundary from high-level polling and VDR service loading logs into domain-specific snapshot build timing. `VdrSnapshotBuilder` now accepts an optional `IRuntimeLogger*`, remains silent by default and emits timing logs for status, recordings, timers, channels and events when a runtime logger is provided.

The implementation uses `IRuntimeLogger::write(const RuntimeLogEntry&)` consistently with the existing runtime logger interface.

The repository was clean and synchronized with `origin/phase-2-actions` after the Phase 10.5 implementation, duplicate-declaration fix and full local test run.

---

## Current Architecture State

VDR-Suite is moving from direct live RESTfulAPI access per API request toward a daemon-owned snapshot and change-detection architecture.

Current implemented and locally validated runtime chain:

```text
VDR
    ↓
RESTfulAPI
    ↓
BasicHttpClient
    ↓
RestfulApiVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
SnapshotCacheService
    ↓
SnapshotCache
```

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- keep refresh decisions inside daemon-owned services
- keep snapshot storage separate from polling logic
- prepare efficient polling based on lightweight change-state checks
- update only changed snapshot domains where possible
- preserve full snapshot rebuild behavior for first poll and recovery paths
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent
- avoid premature federation, SSE, WebSocket, user-management or cluster runtime implementation
- expose runtime progress through an optional logging abstraction instead of ad-hoc `std::cout` output
- expose snapshot build timing through the same runtime logging abstraction
- keep future third-party or multi-client API consumers possible through a platform-oriented API strategy

---

## Phase 9 Runtime Validation Result

Phase 9 was validated against a real local VDR/RESTfulAPI setup.

Initial snapshot:

```text
channels:   342
recordings: 973
timers:     0
events:     36272
```

After one real timer-domain change, the generated update plan was:

```text
status:     no
channels:   no
recordings: no
timers:     yes
events:     no
full:       no
```

Updated snapshot:

```text
channels:   342
recordings: 973
timers:     1
events:     36272
```

This proves:

- real VDR access works
- RESTfulAPI access works
- `BasicHttpClient` works against the real local endpoint
- `RestfulApiVdrAdapter` maps real backend data into VDR-Suite domain objects
- the snapshot runtime works against real backend data
- change-state polling detects a real backend-domain change
- `ChangeDetectionService` creates the expected change event
- `SnapshotRefreshPlanner` creates the expected domain-specific plan
- partial refresh updates only the affected timer domain
- no full snapshot refresh is triggered for a normal timer change

---

## Runtime Logging State

Phase 10 introduced the runtime logging foundation and the first service-level runtime logs.

Implemented runtime logging components:

- `RuntimeLogLevel`
- `RuntimeLogEntry`
- `IRuntimeLogger`
- `NullRuntimeLogger`
- `ConsoleRuntimeLogger`
- runtime log level formatting
- `mk/runtime-sources.mk`
- optional `PollingService` runtime logging
- optional `VdrService` runtime logging
- optional `VdrSnapshotBuilder` domain timing logs

Current runtime log output examples:

```text
[INFO] [PollingService] Poll started
[INFO] [PollingService] Initial snapshot build
[INFO] [PollingService] Executing partial refresh
[INFO] [PollingService] Poll finished
[INFO] [VdrService] Loading status
[INFO] [VdrService] Loading channels
[INFO] [VdrService] Loading recordings
[INFO] [VdrService] Loading timers
[INFO] [VdrService] Loading events
[INFO] [VdrService] Loading change state
[INFO] [VdrSnapshotBuilder] Build status finished (0 ms)
[INFO] [VdrSnapshotBuilder] Build recordings finished (0 ms)
[INFO] [VdrSnapshotBuilder] Build timers finished (0 ms)
[INFO] [VdrSnapshotBuilder] Build channels finished (0 ms)
[INFO] [VdrSnapshotBuilder] Build events finished (0 ms)
```

Current logging design:

- logging stays optional through `IRuntimeLogger* logger = nullptr`
- existing unit tests remain silent unless they explicitly pass a logger
- `NullRuntimeLogger` provides a no-op implementation for future dependency wiring
- `ConsoleRuntimeLogger` provides simple local runtime visibility
- runtime logging uses `IRuntimeLogger::write(const RuntimeLogEntry&)`
- snapshot builder timings use `std::chrono::steady_clock`
- runtime logging is intentionally not a metrics system yet
- HTTP diagnostics, event counters, channel counters, structured diagnostics objects and REST diagnostics endpoints are not implemented yet

---

## Build System State

The top-level `Makefile` delegates source groups to modular include files under `mk/`.

Implemented build modules:

- `mk/common.mk`
- `mk/recording-sources.mk`
- `mk/action-job-sources.mk`
- `mk/rest-sources.mk`
- `mk/daemon-sources.mk`
- `mk/http-sources.mk`
- `mk/runtime-sources.mk`
- `mk/vdr-sources.mk`
- `mk/vdr-tests.mk`

The public targets remain stable:

- `make test`
- `make clean`
- `make daemon`
- `make dashboard-cli`
- `make prepare-test-db`

Local VDR integration targets are intentionally optional and are not part of `make test`:

- `make test-local-restfulapi-integration`
- `make test-local-snapshot-runtime-integration`
- `make test-local-partial-refresh-validation`

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
- `SnapshotUpdatePlan`
- `SnapshotRefreshPlanner`
- `SnapshotCache`
- `SnapshotCacheService`
- `ISnapshotAccessService`
- `SnapshotAccessService`
- `VdrOverviewService` snapshot-backed overview path
- `VdrController` snapshot-backed overview test coverage
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

- `PollingService` reads `VdrService::getChangeState()` before refresh planning
- first poll builds an initial full snapshot
- unchanged change-state keeps the existing cached snapshot
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshPlanner` converts change events into `SnapshotUpdatePlan` values
- `PollingService::lastUpdatePlan()` exposes the most recent generated update plan for tests and diagnostics
- generated update plans are executed domain-by-domain through `VdrSnapshotBuilder` and `SnapshotCacheService`
- status changes execute `buildStatus()` and `updateStatus()`
- recording changes execute `buildRecordings()` and `updateRecordings()`
- timer changes execute `buildTimers()` and `updateTimers()`
- channel changes execute `buildChannels()` and `updateChannels()`
- event changes execute `buildEvents()` and `updateEvents()`
- full snapshot rebuild remains available for first poll and future recovery paths
- `PollingService` can emit optional runtime progress logs through `IRuntimeLogger`
- `VdrService` can emit optional coarse backend-loading progress logs through `IRuntimeLogger`
- `VdrSnapshotBuilder` can emit optional domain-specific timing logs through `IRuntimeLogger`
- `SnapshotAccessService` provides a read boundary for snapshot-backed API-facing services
- `VdrOverviewService` consumes `ISnapshotAccessService` for snapshot-backed overview generation in the daemon runtime
- `VdrController` remains independent from snapshot cache internals
- `DaemonRuntime` owns `SnapshotCache`, `SnapshotCacheService`, `SnapshotAccessService`, `VdrSnapshotBuilder` and `PollingService`
- `DaemonRuntime` wires API-facing overview responses through `SnapshotAccessService`

---

## Future Event Provider Observation

The current runtime uses RESTfulAPI plus change-state polling as the implemented source for `VdrChangeEvent` generation.

`dbus2vdr` is not implemented and is not an active Phase 10 work item.

However, `dbus2vdr` may become relevant later as an alternative or additional event provider. It could potentially emit backend changes into the same internal event path that is currently fed by change-state polling.

Directional model:

```text
Current:
change-state.json
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent

Possible later:
dbus2vdr
    ↓
DbusEventProvider
    ↓
VdrChangeEvent
```

This is only an architectural observation. The current RESTfulAPI and `change-state.json` path remains the primary integration path.

---

## Completed Recent Phases

- Phase 8.90: change-state polling integration
- Phase 8.91: change event generation
- Phase 8.92: snapshot refresh decision model
- Phase 8.93: snapshot cache model
- Phase 8.94: snapshot cache integration into polling runtime
- Phase 8.94.1: daemon runtime wiring correction
- Phase 8.95: snapshot access service
- Phase 8.96: snapshot-backed VDR overview service support
- Phase 8.97: runtime overview wiring consistency
- Phase 8.98: Makefile source modularization
- Phase 8.99: internal event dispatch architecture
- Phase 9.0: snapshot update plan
- Phase 9.1: snapshot refresh planner
- Phase 9.2: snapshot builder domain methods
- Phase 9.2.1: documentation sync after builder domain methods
- Phase 9.3: snapshot cache domain updates
- Phase 9.4: runtime plan integration
- Phase 9.4.1: documentation sync after runtime plan integration
- Phase 9.5: execute snapshot update plans
- Phase 9.5.1: documentation sync after plan execution
- Phase 9.6: local RESTfulAPI integration
- Phase 9.6.1: documentation sync after local RESTfulAPI integration
- Phase 9.7: local snapshot runtime integration
- Phase 9.7.1: documentation sync after local snapshot runtime integration
- Phase 9.8: local partial refresh validation
- Phase 9.8.1: documentation sync after local partial refresh validation
- Phase 10.0: runtime source module and logging interfaces
- Phase 10.1: console runtime logger
- Phase 10.2: runtime log level formatting
- Phase 10.3: PollingService runtime logging
- Phase 10.4: VdrService runtime logging
- Phase 10.5: SnapshotBuilder timing logs
- ADR-007: Platform API Strategy

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
- ADR-006 Internal Event Dispatch Strategy
- ADR-007 Platform API Strategy

Architectural impact:

- future backends must not be identified by hostnames or IP addresses alone
- future multi-VDR support requires stable backend identity and backend federation
- frontends should query backend capabilities instead of checking backend implementation types
- backend lifecycle states must be considered by future polling, snapshot and event-delivery logic
- stream handling must remain backend-neutral and must not permanently assume DVB/VDR as the only source
- internal event dispatch keeps `VdrChangeEvent` independent from snapshot refresh, live update transport and future multi-VDR routing
- partial snapshot refresh planning is represented by `SnapshotUpdatePlan` and `SnapshotRefreshPlanner`
- `VdrSnapshotBuilder` exposes domain-specific build methods while preserving full snapshot rebuild support
- `SnapshotCacheService` owns controlled domain update methods
- `PollingService` now creates and executes `SnapshotUpdatePlan` values during the runtime polling path
- runtime observability is routed through `IRuntimeLogger` instead of direct service-level console output
- VDR-Suite API design should remain suitable for multiple clients and integrations instead of being coupled to one frontend

---

## Current Known Technical Debt

Current change-state parsing inside `RestfulApiVdrAdapter` uses a small local integer-field parser.

This is acceptable for the current minimal endpoint shape, but may later be replaced by a dedicated mapper if the endpoint grows or more RESTfulAPI JSON parsing is consolidated.

The root `Makefile` still contains many test targets and the `clean` target. Source groups have been modularized in Phase 8.98; moving additional target groups into dedicated `mk/*-tests.mk` or `mk/clean.mk` files can be considered later, but is no longer urgent because the root Makefile no longer owns the large source-definition block.

Phase 10.5 improves runtime visibility for polling, VDR service loading and snapshot domain build timing, but observability is still intentionally minimal. HTTP request diagnostics, event counters, channel counters, structured diagnostics objects and REST diagnostics endpoints are not implemented yet.

Documentation state:

- `README.md` has been rebuilt after Phase 8.94.
- `docs/architecture/snapshot-architecture.md` has been rebuilt after Phase 8.94.
- `docs/architecture/snapshot-access-architecture.md` documents the Phase 8.95 access boundary.
- `docs/architecture/internal-event-dispatch-architecture.md` documents the Phase 8.99 internal event dispatch direction.
- `docs/architecture/partial-snapshot-refresh-architecture.md` documents the Phase 9 partial snapshot refresh architecture.
- `docs/development/phase-9.6-local-restfulapi-integration.md` documents the local RESTfulAPI validation.
- `docs/development/phase-9.7-local-snapshot-runtime-integration.md` documents the local snapshot runtime validation.
- `docs/development/phase-9.8-local-partial-refresh-validation.md` documents the local partial refresh validation.
- `docs/adr/007-platform-api-strategy.md` documents the platform API direction.
- `docs/development/current-status.md` documents Phase 10.5, ADR-007 and the current runtime logging foundation.

---

## Next Planned Phase

### Phase 10.6 – HTTP Request Timing

Goal:

Measure HTTP request durations and expose coarse HTTP client diagnostics through the runtime logging boundary.

Motivation:

Phase 10.5 measures snapshot domain build durations. The next useful observability step is to distinguish slow snapshot domains caused by VDR data volume from slow snapshot domains caused by HTTP requests to RESTfulAPI.

Expected direction:

- keep logging optional and silent by default
- do not introduce a metrics database yet
- measure BasicHttpClient request durations
- include coarse request method/path and response status where appropriate
- avoid leaking sensitive request data into logs
- keep structured diagnostics objects and REST diagnostics endpoints for later phases

---

## Upcoming Phases

### Phase 10.6 – HTTP Request Timing

Measure BasicHttpClient request durations, response status and response size where appropriate.

### Phase 10.7 – Runtime Diagnostics Model

Introduce an internal diagnostics model after logging and timing signals are stable enough to aggregate.

### Later Phases

- diagnostics REST endpoint
- event dispatch expansion beyond snapshot refresh
- optional future event providers such as dbus2vdr
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
