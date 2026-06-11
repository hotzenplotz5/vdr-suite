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
Phase 18.4 - Real Polling Stability Validation
```

Current major phase status:

```text
Phase 17 multi-backend read-side REST visibility is complete through 17.3.
```

Verified locally with:

```text
make test-api-router
make test-fast
make test-docs
make test-architecture
make test-phase
make daemon
```

Verification summary:

- multi-backend snapshot read foundation passed targeted tests
- multi-backend snapshot summary JSON serialization passed targeted tests
- `GET /api/vdr/snapshots` route was added through controller and router wiring
- `GET /api/vdr/snapshots` is covered by the API router regression test
- default `/api/vdr/snapshot` behavior remains compatible
- `make test-fast` passed during the Phase 17 read-side work
- `make daemon` passed after REST route test coverage
- GitHub Actions remains the standard full regression path for normal non-VDR-specific changes

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
- `ApiRouter` exposes backend registry routes, backend-aware snapshot routes and the multi-backend snapshots route.
- `SnapshotCache` can store snapshots per backend while preserving the legacy single-snapshot interface.
- `SnapshotAccessService` resolves snapshots through the multi-backend cache and can return all cached backend snapshots.
- `VdrSnapshotReadService` supports backend-aware reads and multi-backend snapshot list reads.
- `VdrSnapshotReadJsonSerializer` can serialize multi-backend snapshot summary lists.
- `VdrController` exposes default VDR reads, backend-specific reads and multi-backend snapshot summary reads.
- `ApiRouter` regression coverage verifies the multi-backend snapshots REST route.
- `VdrSnapshotBuilder` can assign a stable backend ID to generated snapshots.
- `PollingService` and `BackendPollingCoordinator` support backend-aware polling coordination.
- `DaemonRuntime` creates backend runtime contexts from the backend registry.
- Future multi-VDR runtime configuration can build on the registry-driven context creation model.
- Backend identity, federation, capability and lifecycle strategy are documented through ADRs.

---

## Current Implemented API Areas

Snapshot-backed default VDR read APIs:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/snapshot
GET /api/vdr/snapshots
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

Recommended local pre-commit verification for architecture work:

```text
make test-fast
make test-docs
make test-architecture
make test-phase
make daemon
```

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
Phase 19.0 - Snapshot Change Feed Validation
```

The next step is to validate the RESTfulAPI adapter path against an actual VDR while keeping real VDR tests opt-in and outside the fast mock-based test set.

Important boundaries:

- real VDR tests must not be required by `make test-fast`
- GitHub Actions must remain independent from a running VDR
- RESTfulAPI validation should use explicit opt-in environment variables
- BasicHttpClient, RestfulApiVdrAdapter and VdrService should be validated before broader runtime integration
- snapshot runtime validation against real VDR data should build on the opt-in integration test boundary

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
