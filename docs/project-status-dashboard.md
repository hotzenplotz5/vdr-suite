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
Multi-Backend Routing     implemented
Multi-Backend Polling     implemented
Multi-Backend Read API    implemented
Live Transport            planned
```

### Federation and Security

```text
Multi-VDR                 foundation implemented
Backend Registry          runtime + API implemented
Multi-Snapshot Cache      implemented
Backend-Aware Snapshots   implemented
Runtime Contexts          implemented
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
Phase 18.4 Complete
```

Current Focus:

```text
Phase 19.0 - Snapshot Change Feed Validation
```

Latest Completed Milestone:

```text
Phase 18.4 - Real Polling Stability Validation
```

---

## Recently Completed Milestones

```text
Phase 16.0 - Backend-aware snapshot cache service updates
Phase 16.1 - Backend-aware polling service
Phase 16.2 - Backend polling coordinator
Phase 16.3 - Backend runtime context foundation
Phase 16.4 - Daemon runtime context migration
Phase 16.5 - Coordinator runtime integration
Phase 16.6 - Runtime context collection
Phase 16.7 - Runtime context factory
Phase 16.8 - Runtime context creation from registry
Phase 16.9 - Backend-aware snapshot change feed
Phase 17.0 - Multi-backend snapshot read foundation
Phase 17.1 - Multi-backend snapshot summary serialization
Phase 17.2 - Multi-backend snapshots REST endpoint
Phase 17.3 - Multi-backend REST endpoint tests
```

---

## Roadmap Progress

Completed Major Phases:

```text
Phase 0 - Phase 18.4
```

Planned Major Phases:

```text
Phase 18.0+
```

Overall Roadmap Progress:

```text
17 / 18 = 94.4%
```

Important:

This percentage describes documented roadmap progress by major phase. It is not a code coverage metric and not a production-readiness guarantee.

---

## Status Meaning

| Area | Meaning |
| --- | --- |
| Backend Foundation | Core database, services, jobs, dashboard, REST and daemon foundations are implemented. |
| Snapshot Runtime | Daemon-owned snapshot generation, cache, access boundaries and generation tracking are implemented. |
| Read API | Snapshot-backed read APIs for current VDR domains and multi-backend snapshot summaries are implemented. |
| Change Feed | Transport-independent snapshot change feed and backend-aware feed entries are implemented. |
| Backend Registry | BackendNode, BackendRegistry, service layer, JSON serializer, controller and REST routes are implemented. |
| Multi-Backend Routing | Backend-aware snapshot routes, dynamic backend route parsing and multi-backend snapshots REST exposure are implemented and covered by router tests. |
| Multi-Snapshot Cache | SnapshotCache can store and resolve snapshots per backend while keeping the legacy single-snapshot interface compatible. |
| Backend-Aware Snapshots | Snapshot access, snapshot read service, VDR controller, snapshot builder and change feed support backend identity. |
| Multi-Backend Polling | PollingService, BackendPollingCoordinator and DaemonRuntime backend context wiring are implemented for registry-driven runtime preparation. |
| Multi-VDR | Backend registry, runtime contexts, backend-aware snapshots, polling coordination and multi-backend read summaries are implemented as foundations. |
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
