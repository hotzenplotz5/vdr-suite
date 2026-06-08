# VDR-Suite Roadmap

Return:

- ../index.md
- ../project-overview.md
- index.md

---

## Purpose

This roadmap describes the forward direction of VDR-Suite.

Detailed implementation history belongs to:

- ../development/completed-phases.md
- ../development/milestones.md

Current project status belongs to:

- ../development/current-status.md

---

## Roadmap Principles

VDR-Suite remains:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for future multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Current Position

```text
Completed:
Phase 12 - Snapshot Change Feed Foundation

Current cleanup:
Documentation and roadmap consolidation

Next major phase:
Phase 13 - Live Update Transport
```

Phase 12 completed the transport-independent change feed foundation.

Phase 13 should add a live-update transport without moving change detection or feed generation into the transport layer.

---

## Next: Phase 13 - Live Update Transport

Goal:

Expose snapshot change feed information through a live transport mechanism.

Candidate transports:

- Server Sent Events (SSE)
- WebSocket

Expected result:

- live change delivery
- frontend-friendly update channel
- preserved backend neutrality
- preserved snapshot architecture
- no direct frontend coupling to polling internals

Architecture rule:

Live transport consumes the snapshot change feed.

Live transport must not become the owner of:

- snapshot generation
- change detection
- feed generation

---

## Future: Phase 14 - Multi-VDR Backend Routing

Goal:

Prepare read-side architecture for multiple VDR instances.

Planned direction:

- backend identity aware routing
- multiple VDR instances
- backend federation preparation
- permission-aware architecture foundation

---

## Future: Phase 15 - Frontend API Hardening

Goal:

Stabilize API contracts for future frontends.

Planned direction:

- filtering
- pagination
- stable response contracts
- capability-aware responses
- frontend-independent API behavior

---

## Future Product Layers

Possible later layers:

- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD integration
- stream provider integration
- media library expansion

---

## Roadmap Rule

This file should describe direction, not detailed implementation history.

Completed implementation detail belongs in completed phases.
