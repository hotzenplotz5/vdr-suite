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
Phase 13.7e - Snapshot Cache Generation Tracking
```

Current major phase status:

```text
Phase 13 live update transport preparation is complete through 13.7e.
```

Verified locally with:

```text
make test-snapshot-cache-service
make test
make test-docs
make test-architecture
```

Verification summary:

- `make test-snapshot-cache-service` passed after generation expectation fixes
- `make test` passed
- `make test-docs` passed
- `make test-architecture` passed
- daemon-owned snapshot change feed wiring is in place
- mutable snapshot change feed append support is implemented
- runtime feed update integration is implemented
- snapshot cache generation tracking is implemented

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
- Future live updates should build on snapshot change information instead of coupling clients to polling internals.
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
Phase 13.8 - Live Transport Foundation
```

The next step is to add a transport layer above the existing snapshot change feed.

Important boundaries:

- live transport consumes the snapshot change feed
- live transport must not own polling
- live transport must not own snapshot generation
- live transport must not introduce backend-specific logic
- RESTfulAPI-specific code remains inside the adapter layer

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
