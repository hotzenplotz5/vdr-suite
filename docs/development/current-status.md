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
Phase 14.2 - Snapshot Cache Backend Identity Lookup
```

Current major phase status:

```text
Phase 14 multi-VDR backend identity preparation is complete through 14.2.
```

Verified locally with:

```text
make test-snapshot-cache-service
make test
make test-docs
make test-architecture
```

Verification summary:

- `make test-snapshot-cache-service` passed after backend identity lookup coverage
- `make test` passed
- `make test-docs` passed
- `make test-architecture` passed
- snapshot change feed entries are backend identity aware
- snapshot change feed JSON serializes backend identity
- VDR snapshots carry a backend identity with a default fallback
- snapshot read metadata can expose backend identity
- `SnapshotCacheService` exposes the currently cached backend identity
- cache clear restores the default backend identity boundary

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

## Next Technical Focus

```text
Phase 14.3 - Backend-Aware Snapshot Read Routing Boundary
```

The next step is to prepare the read-side routing boundary for selecting snapshot data by backend identity.

Important boundaries:

- backend routing consumes backend identity already present in snapshots and change feed metadata
- backend routing must not own polling
- backend routing must not own snapshot generation
- backend routing must not introduce backend-specific adapter logic
- permission-aware behavior remains architectural preparation unless explicitly implemented
- RESTfulAPI-specific code remains inside the adapter layer

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
