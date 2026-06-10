# VDR-Suite Project Status Dashboard

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)
- [Current Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)

---

## Current Release State

### Core Platform

```text
Backend Foundation        complete
Snapshot Runtime          complete
Read API                  complete
Change Feed               complete
Backend Registry          implemented
Multi-Backend Routing     foundation implemented
Live Transport            planned
```

### Federation and Security

```text
Multi-VDR                 foundation implemented
Backend Registry          runtime + API implemented
Multi-Snapshot Cache      implemented
Backend-Aware Snapshots   implemented
Multi-Backend Polling     planned
Capability System         foundation implemented
Authentication            planned
Authorization             planned
```

### Client Platforms

```text
Web Frontend              planned
Windows Frontend          planned
Android Frontend          planned
iOS Frontend              planned
```

### Media Extensions

```text
Image Validation          planned
Preview Streams           planned
Media Streaming           planned
```

---

## Current Position

Current Major Phase:

```text
Phase 15.9 Complete
```

Current Focus:

```text
Phase 16.0 - Multi-Backend Polling Foundation
```

Latest Completed Milestone:

```text
Phase 15.9 - Backend-aware Snapshot Builder
```

---

## Recently Completed Milestones

```text
Phase 14.8 - BackendRegistry runtime integration
Phase 14.9 - BackendRegistry service layer
Phase 15.0 - BackendRegistry JSON serializer
Phase 15.1 - BackendRegistry controller
Phase 15.2 - BackendRegistry API routing
Phase 15.3 - Backend-aware snapshot read service
Phase 15.4 - Backend-aware VDR controller methods
Phase 15.5 - Backend snapshot API routes
Phase 15.6 - Dynamic backend snapshot route parsing
Phase 15.7 - Multi-snapshot cache foundation
Phase 15.8 - Multi-backend snapshot cache access
Phase 15.9 - Backend-aware snapshot builder
```

---

## Roadmap Progress

Completed Major Phases:

```text
Phase 0 - Phase 15.9
```

Planned Major Phases:

```text
Phase 16.0+
```

Overall Roadmap Progress:

```text
16 / 17 = 94.1%
```

Important:

This percentage describes documented roadmap progress by major phase. It is not a code coverage metric and not a production-readiness guarantee.

---

## Status Meaning

| Area | Meaning |
| --- | --- |
| Backend Foundation | Core database, services, jobs, dashboard, REST and daemon foundations are implemented. |
| Snapshot Runtime | Daemon-owned snapshot generation, cache, access boundaries and generation tracking are implemented. |
| Read API | Snapshot-backed read APIs for current VDR domains are implemented. |
| Change Feed | Transport-independent snapshot change feed and runtime feed update integration are implemented. |
| Backend Registry | BackendNode, BackendRegistry, service layer, JSON serializer, controller and REST routes are implemented. |
| Multi-Backend Routing | Backend-aware snapshot routes and dynamic backend route parsing are implemented. |
| Multi-Snapshot Cache | SnapshotCache can store and resolve snapshots per backend while keeping the legacy single-snapshot interface compatible. |
| Backend-Aware Snapshots | Snapshot access, snapshot read service, VDR controller and snapshot builder support backend identity. |
| Multi-VDR | Backend registry, backend-aware snapshots and routing foundations are implemented. Multi-backend polling remains future work. |
| Live Transport | SSE/WebSocket transport remains planned above the snapshot change feed. |
| Capability System | Capability set and resolver foundations are implemented. |
| Authentication | Planned future concern, not implemented yet. |
| Authorization | Planned future concern, not implemented yet. |
| Client Platforms | Future frontend work, not implemented yet. |
| Media Extensions | Image, preview stream and media stream validation remain future work. |

---

## Primary Documentation Entry Points

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)
- [Current Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)
