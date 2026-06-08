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
Phase 12.3: Snapshot Change Feed REST controller
```

Verified locally with:

```text
make test-snapshot-change-feed-controller
make test
```

Verification summary:

- `make test-snapshot-change-feed-controller` passed
- `make test` passed
- `test_snapshot_change_feed_controller` passed
- `test_api_router` passed with snapshot change feed routing coverage
- snapshot change feed service and serializer tests passed
- snapshot cache, access, refresh, polling and RESTfulAPI mapper tests passed
- runtime diagnostics tests passed

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot-based read architecture is completed for the current domain set.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot change feed service, serializer and read-only REST controller are implemented.
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
- [Snapshot Change Feed Architecture](../architecture/snapshot-change-feed-architecture.md)
- [VDR Backends](../architecture/vdr-backends.md)
- [VDR-Suite Core Platform Model](../architecture/vdr-suite-core-platform-model.md)

---

## Next Planned Phase

### Phase 12.4: Snapshot Change Feed Integration Refinement

Goal:

Consolidate snapshot change feed integration and prepare follow-up architecture for future transport layers while keeping the feed model transport-independent.

Constraints:

- no SSE architecture yet
- no WebSocket architecture yet
- no frontend coupling
- keep multi-VDR and backend identity requirements visible

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
