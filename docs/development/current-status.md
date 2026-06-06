# VDR-Suite – Current Project Status

## Introduction

New contributors should start with:

- `docs/introduction/vdr-suite-vision.md`
- `README.md`
- [Documentation Index](../index.md)
- [Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)

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

## Current Verified State

Latest verified code state:

`b978a87`

Latest completed implementation phase:

Phase 10.18: Runtime diagnostics measurement summaries.

Verified locally with:

```text
make test-runtime-diagnostics-controller
make test-api-router
make test-test-http-server
make test
make daemon
git status
```

Local result:

- `make test-runtime-diagnostics-controller` passed
- `make test-api-router` passed
- `make test-test-http-server` passed
- `make test` passed
- `make daemon` passed
- working tree was clean
- branch was synchronized with `origin/phase-2-actions`

Important architecture notes:

- Phase 10.10 introduced `IRuntimeMeasurementSink` as the structured runtime measurement boundary.
- Phase 10.11 made `RuntimeDiagnosticsService` implement that boundary.
- Phase 10.12 connected `BasicHttpClient` to the measurement sink.
- Phase 10.13 connected `VdrSnapshotBuilder` to the measurement sink.
- Phase 10.14 connected `PollingService` to the measurement sink.
- Phase 10.14 also made `DaemonRuntime` own `RuntimeDiagnosticsService` and pass it into current measurement-producing components.
- Phase 10.15 added runtime diagnostics JSON serialization.
- Phase 10.16 added the read-only runtime diagnostics REST endpoint at `GET /api/runtime`.
- Phase 10.17 added bounded in-memory diagnostics measurement retention.
- Phase 10.18 added internal runtime measurement summaries grouped by component and operation.

Detailed status:

- [Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)

---

## Current Architecture State

VDR-Suite is moving from direct live RESTfulAPI access per API request toward a daemon-owned snapshot, change-detection and runtime-observability architecture.

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

Current implemented runtime diagnostics chains:

```text
BasicHttpClient
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics

VdrSnapshotBuilder
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics

PollingService
    ↓
RuntimeMeasurement
    ↓
IRuntimeMeasurementSink
    ↓
RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics

RuntimeDiagnosticsService
    ↓
RuntimeDiagnostics
    ↓
RuntimeDiagnosticsJsonSerializer
    ↓
RuntimeDiagnosticsController
    ↓
GET /api/runtime

RuntimeDiagnosticsService
    ↓
RuntimeMeasurementSummary
    ↓
count
minDurationMs
maxDurationMs
lastDurationMs
lastStatusCode
lastSizeBytes
```

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- keep refresh decisions inside daemon-owned services
- keep snapshot storage separate from polling logic
- preserve full snapshot rebuild behavior for first poll and recovery paths
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent
- expose runtime progress through optional logging instead of direct service-level console output
- expose HTTP request timing through structured runtime measurements
- expose snapshot-domain build timing through structured runtime measurements
- expose polling-cycle and refresh-path timing through structured runtime measurements
- keep logging and diagnostics separate
- avoid unbounded diagnostics measurement growth through service-owned retention
- provide internal grouped measurement summaries without changing the public `/api/runtime` JSON format
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
- the snapshot runtime works against real backend data
- change-state polling detects a real backend-domain change
- partial refresh updates only the affected timer domain
- no full snapshot refresh is triggered for a normal timer change

---

## Phase 10 Runtime Logging and Diagnostics State

Phase 10 introduced runtime logging first and then started structured runtime diagnostics.

Implemented runtime logging components:

- `RuntimeLogLevel`
- `RuntimeLogEntry`
- `IRuntimeLogger`
- `NullRuntimeLogger`
- `ConsoleRuntimeLogger`
- runtime log level formatting
- optional `PollingService` runtime logging
- optional `VdrService` runtime logging
- optional `VdrSnapshotBuilder` domain timing logs
- optional `BasicHttpClient` request timing logs
- `ConsoleRuntimeLogger` wiring inside `DaemonRuntime`

Implemented runtime diagnostics components:

- `RuntimeMeasurement`
- `RuntimeDiagnostics`
- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- `RuntimeMeasurementSummary`
- daemon-owned `RuntimeDiagnosticsService` wiring
- `test_runtime_diagnostics`
- `test_runtime_diagnostics_service`
- `BasicHttpClient` structured HTTP measurement recording
- `VdrSnapshotBuilder` structured snapshot-domain measurement recording
- `PollingService` structured poll-cycle and refresh-path measurement recording
- `RuntimeDiagnosticsJsonSerializer`
- `RuntimeDiagnosticsController`
- `GET /api/runtime`
- runtime diagnostics retention policy
- internal runtime measurement summaries
- `test_vdr_snapshot_builder` measurement-sink coverage
- `test_polling_service` measurement-sink coverage
- `test_runtime_diagnostics_json_serializer`
- `test_runtime_diagnostics_controller`
- `test_api_router` `/api/runtime` routing coverage
- `test_test_http_server` `/api/runtime` HTTP server coverage

Current diagnostics design:

- diagnostics do not parse human-readable log output
- runtime components create `RuntimeMeasurement` values directly
- `IRuntimeMeasurementSink` decouples producers from diagnostics storage
- `RuntimeDiagnosticsService` stores measurements in `RuntimeDiagnostics`
- `RuntimeDiagnosticsService` keeps retained measurements bounded in memory
- `RuntimeDiagnosticsService` can calculate internal measurement summaries grouped by component and operation
- `BasicHttpClient` records component, operation, duration, status code and body size for successful requests
- `VdrSnapshotBuilder` records component, operation, duration and domain item count for snapshot-domain build operations
- `PollingService` records component, operation, duration and selected counts for poll-cycle and refresh-path work
- `DaemonRuntime` owns one `RuntimeDiagnosticsService` and passes it into `BasicHttpClient`, `VdrSnapshotBuilder` and `PollingService`
- `RuntimeDiagnosticsJsonSerializer` serializes collected measurements as JSON
- `RuntimeDiagnosticsController` exposes the current diagnostics state through `GET /api/runtime`

Not implemented yet:

- public summary JSON or rolling averages
- runtime diagnostics persistence

---

## Build System State

The top-level `Makefile` delegates source groups to modular include files under `mk/`.

Implemented build modules include:

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

Runtime diagnostics test state:

- `make test-runtime-diagnostics` validates the internal `RuntimeDiagnostics` and `RuntimeMeasurement` model.
- `test_runtime_diagnostics_service` validates `RuntimeDiagnosticsService`, `IRuntimeMeasurementSink` usage, retention behavior and internal measurement summaries.
- `test_vdr_snapshot_builder` validates `VdrSnapshotBuilder` measurement-sink recording.
- `test_polling_service` validates `PollingService` measurement-sink recording while preserving existing polling behavior.
- `test_runtime_diagnostics_json_serializer` validates diagnostics JSON serialization.
- `test_runtime_diagnostics_controller` validates the read-only REST controller response.
- `test_api_router` validates `/api/runtime` routing.
- `test_test_http_server` validates `/api/runtime` through the HTTP server layer.
- `make test` includes the runtime diagnostics, serializer, controller, router, HTTP server, snapshot-builder and polling-service measurement tests.

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

Polling integration:

- `PollingService` reads `VdrService::getChangeState()` before refresh planning
- first poll builds an initial full snapshot
- unchanged change-state keeps the existing cached snapshot
- changed change-state is converted into `VdrChangeEvent` entries
- `SnapshotRefreshPlanner` converts change events into `SnapshotUpdatePlan` values
- generated update plans are executed domain-by-domain through `VdrSnapshotBuilder` and `SnapshotCacheService`
- `BasicHttpClient` can emit structured HTTP runtime measurements through `IRuntimeMeasurementSink`
- `VdrSnapshotBuilder` can emit structured snapshot-domain runtime measurements through `IRuntimeMeasurementSink`
- `PollingService` can emit structured polling and refresh-path runtime measurements through `IRuntimeMeasurementSink`
- `DaemonRuntime` wires `ConsoleRuntimeLogger` into runtime components
- `DaemonRuntime` owns `RuntimeDiagnosticsService` and wires it into current measurement producers
- `DaemonRuntime` owns `RuntimeDiagnosticsController` and exposes diagnostics through `GET /api/runtime`

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
- Phase 10.6: HTTP client timing logs
- Phase 10.7: runtime diagnostics model
- Phase 10.8: runtime logger daemon wiring
- Phase 10.9: runtime diagnostics service
- Phase 10.10: runtime measurement sink boundary
- Phase 10.11: diagnostics service implements measurement sink
- Phase 10.12: HTTP runtime measurement collection
- Phase 10.13: VdrSnapshotBuilder runtime measurements
- Phase 10.14: PollingService runtime measurements and daemon diagnostics wiring
- Phase 10.15: runtime diagnostics JSON serializer
- Phase 10.16: runtime diagnostics REST endpoint
- Phase 10.17: runtime diagnostics retention policy
- Phase 10.18: runtime diagnostics measurement summaries
- ADR-007: Platform API Strategy

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
- internal event dispatch keeps `VdrChangeEvent` independent from snapshot refresh, live update transport and future multi-VDR routing
- runtime diagnostics are routed through `IRuntimeMeasurementSink` instead of log parsing
- VDR-Suite API design should remain suitable for multiple clients and integrations instead of being coupled to one frontend

---

## Current Known Technical Debt

Current change-state parsing inside `RestfulApiVdrAdapter` uses a small local integer-field parser.

This is acceptable for the current minimal endpoint shape, but may later be replaced by a dedicated mapper if the endpoint grows or more RESTfulAPI JSON parsing is consolidated.

Phase 10.18 extends runtime diagnostics with bounded in-memory retention and internal measurement summaries. The read-only REST endpoint at `GET /api/runtime` still exposes the existing measurements JSON format. Persistence and broader diagnostics API hardening are not implemented yet.

The real daemon test showed that recordings are currently the dominant initial snapshot cost in the tested setup. Future optimization should investigate whether recordings can be refreshed or summarized more incrementally instead of repeatedly fetching large `/recordings.json` responses.

Documentation state:

- `docs/development/phase-10-runtime-diagnostics-measurement-collection.md` documents the Phase 10.10 to Phase 10.18 diagnostics measurement collection, retention, internal summaries, serialization and REST endpoint state.
- `docs/adr/007-platform-api-strategy.md` documents the platform API direction.
- `docs/development/current-status.md` documents Phase 10.18 and the current runtime diagnostics retention and internal summary foundation.

---

## Next Planned Phase

### Phase 10.19: Runtime Diagnostics API Summary Exposure Decision

Goal:

Decide whether internal runtime measurement summaries should remain service-internal or become part of a dedicated diagnostics summary API surface.

Motivation:

Phase 10.18 introduced grouped measurement summaries without changing the public `/api/runtime` JSON contract. Before exposing summaries to clients, the API shape should be explicit and should not turn diagnostics into a debug dump.

Expected direction:

- keep `/api/runtime` backward compatible unless an explicit API extension is chosen
- avoid mixing diagnostics summaries with debug-state output
- preserve logging, diagnostics and debugging as separate runtime concerns
- keep any public summary JSON explicit and test-covered

---

## Upcoming Phases

### Phase 10.19: Runtime Diagnostics API Summary Exposure Decision

Decide whether internal measurement summaries should remain service-internal or be exposed through an explicit diagnostics API contract.

### Phase 10.20: Runtime Diagnostics API Hardening or Integration Validation

Validate and harden the runtime diagnostics API surface before expanding diagnostics beyond the current read-only endpoint.

### Later Phases

- recordings snapshot optimization or incremental recording refresh strategy
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
- `docs/development/phase-10-runtime-diagnostics-measurement-collection.md`
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
- Before every push, run `git fetch` and inspect `git log --oneline --decorate HEAD..origin/phase-2-actions`.
