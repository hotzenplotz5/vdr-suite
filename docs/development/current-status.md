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
Phase 14.3 - Backend-Aware Snapshot Read Routing Boundary
```

Current major phase status:

```text
Phase 14 multi-VDR backend identity preparation is complete through 14.3.
```

Verified locally with:

```text
make test-snapshot-access-service
make test
```

Verification summary:

- `make test-snapshot-access-service` passed after backend-aware snapshot access coverage
- `make test` passed after Phase 14.3 implementation
- `ISnapshotAccessService` exposes backend-aware snapshot read methods
- `SnapshotAccessService` implements backend-aware lookup against the current single cached snapshot
- matching backend identity returns the cached snapshot
- unknown backend identity returns no snapshot
- empty cache backend lookup returns no snapshot
- existing `hasSnapshot()` and `snapshot()` behavior remains unchanged

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot-based read architecture is completed for the current domain set.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Snapshot cache generation tracking is implemented in `SnapshotCacheService`.
- Snapshot change feed service, serializer and read-only REST controller are implemented.
- Snapshot change feed entries can now be appended to an existing runtime-owned feed.
- Daemon runtime owns the snapshot change feed and updates it after VDR polling.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Backend identity is now present in snapshot change feed entries, snapshot read metadata and cached snapshots.
- Snapshot access now has a backend-aware read boundary while preserving single-backend compatibility.
- Future multi-VDR read routing can build on backend identity without coupling clients to polling internals.
- Backend identity, federation, capability and lifecycle strategy are documented through ADRs.

---

## Current Implemented API Areas

Snapshot-backed VDR read APIs:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/snapshot
GET /api/vdr/capabilities
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
```

Snapshot change feed API:

```text
GET /api/vdr/changes
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

The full local regression target currently takes approximately:

```text
17 minutes
```

This makes cloud-based CI valuable as an immediate infrastructure improvement.

---

## Next Technical Focus

```text
Phase 14ci - GitHub Actions CI Foundation
```

The next step is to add a GitHub Actions workflow that runs the full regression test automatically on GitHub.

Important boundaries:

- GitHub CI complements local targeted tests
- `make test` remains the full regression target
- local development can use narrower targets while CI runs the long full test
- no production runtime behavior changes are part of the CI phase

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
