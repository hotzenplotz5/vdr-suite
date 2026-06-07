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

`ae3dde8`

Latest GitHub-applied code state:

`3083351`

Latest completed implementation phase:

Phase 10.21.1: Status documentation split in progress.

Verified locally with:

```text
make test-runtime-diagnostics-controller
make test-api-router
make test-test-http-server
make test
make daemon
git status
```

Local result after Phase 10.21:

- `make test-runtime-diagnostics-controller` passed
- `make test-api-router` passed
- `make test-test-http-server` passed
- `make test` passed
- `make daemon` passed
- working tree was clean
- branch was synchronized with `origin/phase-2-actions`

Phase 10.21.1 documentation split state:

- GitHub-applied split documents exist.
- Local verification is still pending after the final `current-status.md` reduction commit.

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

---

## Detail Documents

Current status details are intentionally split into focused documents:

- [Current Architecture State](current-architecture-state.md)
- [Phase 9 Runtime Validation Result](phase-9-runtime-validation-result.md)
- [Runtime Diagnostics Status](runtime-diagnostics-status.md)
- [Build System State](build-system-state.md)
- [Current Technical Debt](current-technical-debt.md)
- [Completed Phases](completed-phases.md)
- [Milestones](milestones.md)
- [Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)
- [Snapshot Architecture](../architecture/snapshot-architecture.md)
- [VDR Backends](../architecture/vdr-backends.md)
- [VDR-Suite Core Platform Model](../architecture/vdr-suite-core-platform-model.md)
- [Architecture Decision Records](../adr/)

---

## Completed Recent Phases

Recent completed implementation phases:

- Phase 10.16: runtime diagnostics REST endpoint
- Phase 10.17: runtime diagnostics retention policy
- Phase 10.18: runtime diagnostics measurement summaries
- Phase 10.19: runtime diagnostics summary endpoint
- Phase 10.20: runtime diagnostics API hardening
- Phase 10.21: runtime configuration cleanup
- Phase 10.21.1: status documentation split in progress

Long-running phase history is tracked in:

- [Completed Phases](completed-phases.md)
- [Milestones](milestones.md)

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

### Phase 10.21.1: Status Documentation Split

Goal:

Complete the split of `docs/development/current-status.md` into a compact status index and dedicated detail documents.

Motivation:

The previous `current-status.md` had grown into a large mixed status, history, architecture, validation, build and technical-debt document. This made GitHub-based edits harder to review and increased the risk of oversized file updates.

Expected direction:

- keep `current-status.md` focused on current branch, current verified state, latest phase and links
- keep detailed architecture state in `current-architecture-state.md`
- keep Phase 9 runtime validation data in `phase-9-runtime-validation-result.md`
- keep runtime diagnostics state in `runtime-diagnostics-status.md`
- keep build details in `build-system-state.md`
- keep known debt in `current-technical-debt.md`
- verify the reduced status file locally before marking Phase 10.21.1 complete

---

## Upcoming Phases

### Phase 10.21.1: Status Documentation Split

Finish local verification and mark the documentation split complete.

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
