# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite.

It should stay focused on the present state.

Implementation history belongs in [Completed Phases](completed-phases.md).

Future planning belongs in [Roadmap](../planning/roadmap.md).

---

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST APIs, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

```text
main
```

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 15.9 - Backend-aware Snapshot Builder
```

Current major phase status:

```text
Phase 15 multi-backend read-side and snapshot foundation is complete through 15.9.
```

Verified locally with:

```text
make test-vdr-snapshot-builder
make test-fast
make daemon
```

Verification summary:

- `make test-vdr-snapshot-builder` passed after backend-aware snapshot builder compatibility fixes
- `make test-fast` passed after backend registry and snapshot access foundation work
- `make daemon` passed after backend-aware snapshot builder integration
- GitHub Actions remains the standard full regression path for normal non-VDR-specific changes
- local full `make test` is not required for ordinary architecture, service, snapshot, registry and documentation changes

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot-based read architecture is completed for the current domain set.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Snapshot cache generation tracking is implemented in `SnapshotCacheService`.
- Snapshot change feed service, serializer and read-only REST controller are implemented.
- Snapshot change feed entries can be appended to an existing runtime-owned feed.
- Daemon runtime owns the snapshot change feed and updates it after VDR polling.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Backend identity is present in snapshot change feed entries, snapshot read metadata and cached snapshots.
- `BackendNode` and `BackendRegistry` provide the backend identity foundation.
- `BackendRegistryService`, `BackendRegistryJsonSerializer` and `BackendRegistryController` expose the registry through service and REST boundaries.
- `ApiRouter` exposes backend registry routes and backend-aware snapshot routes.
- `SnapshotCache` can store snapshots per backend while preserving the legacy single-snapshot interface.
- `SnapshotAccessService` resolves snapshots through the multi-backend cache.
- `VdrSnapshotReadService` and `VdrController` support backend-aware read paths.
- `VdrSnapshotBuilder` can assign a stable backend ID to generated snapshots.
- Future multi-VDR runtime polling can build on backend identity without coupling clients to polling internals.
- Backend identity, federation, capability and lifecycle strategy are documented through ADRs.

---

## Current Implemented API Areas

Snapshot-backed default VDR read APIs:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/snapshot
GET /api/vdr/capabilities
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
GET /api/vdr/changes
```

Backend registry and backend-aware read APIs:

```text
GET /api/backends
GET /api/backends/default
GET /api/backends/{backendId}/status
GET /api/backends/{backendId}/health
GET /api/backends/{backendId}/snapshot
```

Existing application APIs:

```text
GET /api/dashboard
GET /api/jobs
GET /api/recordings
GET /api/metadata
GET /api/runtime/diagnostics
GET /api/runtime/summary
```

---

## Current Test Runtime Observation

The full local regression target is intentionally no longer the default verification path for normal development work.

Project workflow:

```text
GitHub-first
CI-first
```

Targeted local tests remain useful for narrow changes and for real VDR validation.

Real VDR tests are reserved for:

- RESTfulAPI against an actual VDR
- SSE event streams
- polling against an actual VDR
- snapshot runtime against an actual VDR
- multi-VDR or multi-server scenarios

---

## Next Technical Focus

```text
Phase 16.0 - Multi-Backend Polling Foundation
```

The next step is to connect the backend registry and backend-aware snapshot builder to daemon-owned polling and snapshot cache updates.

Important boundaries:

- polling remains daemon-owned
- snapshot generation remains backend-neutral
- default backend behavior must remain compatible
- `/api/vdr/...` routes must continue to work
- `/api/backends/{backendId}/...` routes should become backed by runtime data
- no parallel polling should be introduced before the single-threaded multi-backend model is clear

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
