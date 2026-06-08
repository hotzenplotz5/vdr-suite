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
phase-2-actions
```

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 12.3 - Snapshot Change Feed REST Controller
```

Current major phase status:

```text
Phase 12 complete
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
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Snapshot change feed service, serializer and read-only REST controller are implemented.
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
```

---

## Next Planned Phase

```text
Phase 13 - Live Update Transport
```

Goal:

Expose snapshot change feed information through a live transport mechanism while preserving the current transport-independent change feed model.

Candidate transports:

- Server Sent Events
- WebSocket

Constraints:

- live transport consumes the snapshot change feed
- live transport must not own snapshot generation
- live transport must not own change detection
- live transport must not own feed generation
- no frontend coupling to polling internals
- keep multi-VDR and backend identity requirements visible

---

## Documentation Hubs

- [Project Overview](../project-overview.md)
- [Planning Documentation](../planning/index.md)
- [Development Documentation](index.md)
- [Architecture Documentation](../architecture/index.md)
- [Architecture Decision Records](../adr/index.md)

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

---

### Phase 13.5 / 13.6: Capability Resolver Foundation

Implemented:

- VdrCapabilitySet
- ICapabilityResolver
- CapabilityResolver
- GET /api/vdr/capabilities

The capabilities endpoint now uses the capability resolver instead of directly exposing a raw capability set.

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)