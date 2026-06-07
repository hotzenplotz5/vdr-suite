# VDR-Suite – Current Project Status

## Introduction

New contributors should start with:

- `docs/introduction/vdr-suite-vision.md`
- `README.md`
- [Documentation Index](../index.md)
- [Roadmap](../planning/roadmap.md)
- [Planning Milestones](../planning/milestones.md)
- [Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)
- [Phase 11 Snapshot Read APIs](phase-11-snapshot-read-apis.md)

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

`e777264`

Latest GitHub-applied documentation state:

`1ec7ea4`

Latest completed implementation phase:

Phase 11.6: Complete snapshot read domain JSON serialization.

Verified locally with:

```text
make test-api-router
make test
```

Local result after Phase 11.6 snapshot read serialization:

- `make test-api-router` passed
- `make test` passed
- `test_vdr_controller` passed with real snapshot-backed domain JSON
- `test_api_router` passed with status, channels, timers, events and recordings snapshot read coverage
- `test_test_http_server` passed
- snapshot cache, access, refresh, polling and RESTfulAPI mapper tests passed
- runtime diagnostics tests passed
- branch was synchronized with `origin/phase-2-actions`

Phase 11.6 snapshot read serialization status:

- GitHub-applied
- Locally verified
- full `make test` verified

---

## Important Architecture Notes

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
- Phase 10.19 exposed those summaries through the dedicated `GET /api/runtime/summary` endpoint while keeping `GET /api/runtime` backward compatible.
- Phase 10.20 hardened router-level and HTTP-server-level coverage for non-empty runtime diagnostics and summary responses.
- Phase 10.21 moved daemon runtime wiring values for VDR access and HTTP listener binding into `RuntimeConfig`.
- Phase 10.21.1 split long-running status, architecture, validation, diagnostics, build and technical-debt sections into dedicated documents.
- Phase 11.0 introduced `VdrSnapshotReadService` as the frontend-oriented snapshot read boundary.
- Phase 11.1 introduced `VdrSnapshotReadJsonSerializer` for read-only snapshot API serialization.
- Phase 11.2 wired frontend-oriented VDR snapshot read paths through `VdrController`, `ApiRouter` and `TestHttpServer` coverage.
- Phase 11.3 added snapshot-backed channel JSON serialization.
- Phase 11.4 added snapshot-backed timer JSON serialization.
- Phase 11.5 added snapshot-backed event JSON serialization.
- Phase 11.6 added snapshot-backed recording JSON serialization and completed the current snapshot read domain API set.

---

## Detail Documents

Current status details are intentionally split into focused documents:

- [Current Architecture State](current-architecture-state.md)
- [Phase 9 Runtime Validation Result](phase-9-runtime-validation-result.md)
- [Runtime Diagnostics Status](runtime-diagnostics-status.md)
- [Build System State](build-system-state.md)
- [Current Technical Debt](current-technical-debt.md)
- [Completed Phases](completed-phases.md)
- [Development Milestones](milestones.md)
- [Planning Roadmap](../planning/roadmap.md)
- [Planning Milestones](../planning/milestones.md)
- [Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)
- [Phase 11 Snapshot Read APIs](phase-11-snapshot-read-apis.md)
- [Snapshot Architecture](../architecture/snapshot-architecture.md)
- [VDR Backends](../architecture/vdr-backends.md)
- [VDR-Suite Core Platform Model](../architecture/vdr-suite-core-platform-model.md)
- [Architecture Decision Records](../adr/)

---

## Completed Recent Phases

Recent completed implementation phases:

- Phase 10.21: runtime configuration cleanup
- Phase 10.21.1: status documentation split
- Phase 11.0: snapshot read service foundation
- Phase 11.1: snapshot read JSON serializer foundation
- Phase 11.2: snapshot read REST API routing and HTTP coverage
- Phase 11.3: snapshot read channel JSON serialization
- Phase 11.4: snapshot read timer JSON serialization
- Phase 11.5: snapshot read event JSON serialization
- Phase 11.6: snapshot read recording JSON serialization and complete domain read API coverage

Long-running phase history is tracked in:

- [Completed Phases](completed-phases.md)
- [Development Milestones](milestones.md)
- [Planning Roadmap](../planning/roadmap.md)

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

## Next Planned Phase

### Phase 12.0: Snapshot Change Feed Architecture

Goal:

Introduce a read-oriented change feed concept on top of the existing snapshot and change-detection domain so future frontends can refresh selectively instead of polling every snapshot read endpoint.

Motivation:

Phase 11 completed stable snapshot-backed read endpoints for status, channels, timers, events and recordings. The next architectural step is to expose what changed between snapshots without coupling clients to polling internals or direct RESTfulAPI access.

Expected direction:

- review the existing `VdrChangeState`, `VdrChangeEvent` and `ChangeDetectionService` contracts
- define the internal shape of a snapshot change feed
- keep feed generation separate from HTTP transport
- avoid introducing SSE or WebSocket transport in the first step
- keep multi-VDR and backend identity requirements in mind

---

## Upcoming Phases

### Phase 12.0: Snapshot Change Feed Architecture

Define the architecture and first read model for snapshot change information.

### Later Phases

- snapshot change feed service implementation
- snapshot change feed REST endpoint
- event dispatch expansion beyond snapshot refresh
- optional future event providers such as dbus2vdr
- SSE/WebSocket live update transport
- multi-VDR backend identity and routing
- backend-owned capability model for permission-aware frontends
- backend lifecycle handling for offline/reconnecting/failed/disabled states
- backend-neutral stream provider integration

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
