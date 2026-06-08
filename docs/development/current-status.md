# VDR-Suite – Current Project Status

This document tracks the current verified state of the VDR-Suite project.

Detailed implementation history, planning, architecture notes and ADRs are maintained in dedicated documents.

---

## Start Here

New contributors should start with:

- [README](../../README.md)
- [Documentation Index](../index.md)
- [VDR-Suite Vision](../introduction/vdr-suite-vision.md)
- [Planning Index](../planning/index.md)
- [Development Index](index.md)
- [Architecture Index](../architecture/index.md)
- [ADR Index](../adr/index.md)

---

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST APIs, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

`phase-2-actions`

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Verified locally with:

```text
make test-api-router
make test
```

Verification summary:

- `make test-api-router` passed
- `make test` passed
- `test_vdr_controller` passed with snapshot-backed domain JSON
- `test_api_router` passed with status, channels, timers, events and recordings snapshot read coverage
- `test_test_http_server` passed
- snapshot cache, access, refresh, polling and RESTfulAPI mapper tests passed
- runtime diagnostics tests passed

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot-based read architecture is completed for the current domain set.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Future live updates should build on snapshot change information instead of coupling clients to polling internals.
- Backend identity, federation, capability and lifecycle strategy are documented through ADRs.

---

## Detail Documents

Documentation hubs:

- [Documentation Index](../index.md)
- [Planning Index](../planning/index.md)
- [Development Index](index.md)
- [Architecture Index](../architecture/index.md)
- [ADR Index](../adr/index.md)

Development:

- [Completed Phases](completed-phases.md)
- [Development Milestones](milestones.md)
- [Current Architecture State](current-architecture-state.md)
- [Build System State](build-system-state.md)
- [Current Technical Debt](current-technical-debt.md)
- [Phase 9 Runtime Validation Result](phase-9-runtime-validation-result.md)
- [Runtime Diagnostics Status](runtime-diagnostics-status.md)
- [Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)
- [Phase 11 Snapshot Read APIs](phase-11-snapshot-read-apis.md)

Planning:

- [Roadmap](../planning/roadmap.md)
- [Planning Milestones](../planning/milestones.md)

Architecture:

- [Snapshot Architecture](../architecture/snapshot-architecture.md)
- [VDR Backends](../architecture/vdr-backends.md)
- [VDR-Suite Core Platform Model](../architecture/vdr-suite-core-platform-model.md)

---

## Next Planned Phase

### Phase 12.0: Snapshot Change Feed Architecture

Goal:

Introduce a read-oriented change feed concept on top of the existing snapshot and change-detection domain so future frontends can refresh selectively instead of polling every snapshot read endpoint.

Expected direction:

- review the existing `VdrChangeState`, `VdrChangeEvent` and `ChangeDetectionService` contracts
- define the internal shape of a snapshot change feed
- keep feed generation separate from HTTP transport
- avoid introducing SSE or WebSocket transport in the first step
- keep multi-VDR and backend identity requirements in mind

Later planning is tracked in:

- [Roadmap](../planning/roadmap.md)
- [Planning Milestones](../planning/milestones.md)

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
