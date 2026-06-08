# VDR-Suite Project Overview

Navigation:

- index.md
- development/current-status.md
- development/completed-phases.md
- planning/roadmap.md
- architecture/index.md
- adr/index.md

---

## Purpose

This page gives a compact overview of the current VDR-Suite project state.

Use this page for orientation before reading detailed status, roadmap, architecture or ADR documents.

Detailed status belongs in development/current-status.md.

Implementation history belongs in development/completed-phases.md.

Future planning belongs in planning/roadmap.md.

---

## Current Position

Completed major phase:

```text
Phase 12 - Snapshot Change Feed Foundation
```

Latest completed implementation phase:

```text
Phase 12.3 - Snapshot Change Feed REST Controller
```

Next major phase:

```text
Phase 13 - Live Update Transport
```

Current cleanup:

```text
Documentation and roadmap consolidation
```

---

## Product Direction

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, snapshot architecture, REST APIs and future frontend integrations.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Architecture Direction

VDR-Suite is designed to remain:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for future multi-VDR environments
- suitable for multiple future clients and integrations

---

## Current Architecture Highlights

- Snapshot-based read architecture is completed for the current domain set.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Snapshot change feed service, serializer and read-only REST controller are implemented.
- Future live updates should build on snapshot change information instead of coupling clients to polling internals.
- Backend identity, federation, capability and lifecycle strategy are documented through ADRs.

---

## Implemented Areas

The project currently contains foundations for:

- SQLite persistence
- recording, metadata, job and action services
- dashboard services and JSON serialization
- REST controllers and API routing
- HTTP client/server abstractions
- VDR domain objects and adapter boundaries
- RESTfulAPI integration
- daemon-owned VDR snapshots
- snapshot cache and snapshot access services
- change-state polling and partial snapshot refresh planning
- runtime logging, timing and diagnostics foundations
- snapshot read APIs
- snapshot change feed foundation

---

## Progress Overview

Planning estimates:

```text
Backend Foundation        95 percent
Snapshot Runtime          100 percent
Snapshot Read APIs        90 percent
Snapshot Change Feed      100 percent
Live Update Transport     0 percent
Multi-VDR Architecture    15 percent
Capability Model          0 percent
Authentication            0 percent
Frontend Contracts        25 percent
Windows Frontend          0 percent
Web Frontend              0 percent
Android Frontend          0 percent
iOS Frontend              0 percent
```

These values are orientation estimates, not release promises.

---

## Next Phase: Phase 13 - Live Update Transport

Goal:

Expose snapshot change feed information through a live transport mechanism.

Candidate transports:

- Server Sent Events
- WebSocket

Architecture rule:

Live transport consumes the snapshot change feed.

Live transport must not own:

- snapshot generation
- change detection
- feed generation

---

## Future Major Phases

Planned direction:

- Phase 14 - Multi-VDR Backend Routing
- Phase 15 - Frontend API Hardening
- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD integration
- stream provider integration
- media library expansion

---

## Documentation Entry Points

- Documentation Index: index.md
- Current Status: development/current-status.md
- Completed Phases: development/completed-phases.md
- Roadmap: planning/roadmap.md
- Planning Index: planning/index.md
- Development Index: development/index.md
- Architecture Index: architecture/index.md
- ADR Index: adr/index.md

---

## Documentation Rule

Use this page for project orientation.

Use current-status.md for the current verified technical state.

Use completed-phases.md for implementation history.

Use roadmap.md for future direction.
