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
Live Transport            implemented
Selective Event Queries   implemented
Heavy Domain Policy       implemented
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
Phase 26.1 Complete
```

Current Focus:

```text
Phase 26.2 - EPG JSON Escaping
```

Latest Completed Milestone:

```text
Phase 26.1 - EPG Snapshot Decoupling Validation
```

Architecture Work In Progress:

```text
Phase 26.2 - EPG JSON Escaping
ADR-0021 - Selective Backend Query Strategy
```

---

## Recently Completed Milestones

```text
Phase 17.0 - Multi-backend snapshot read foundation
Phase 17.1 - Multi-backend snapshot summary serialization
Phase 17.2 - Multi-backend snapshots REST endpoint
Phase 17.3 - Multi-backend REST endpoint tests
Phase 18.0 - Real RESTfulAPI integration validation
Phase 18.1 - Real snapshot builder validation
Phase 18.2 - Real change-state validation
Phase 18.3 - Real polling initial snapshot validation
Phase 18.4 - Real polling stability validation
Phase 19.0 - Snapshot change feed service validation
Phase 19.1 - Polling to change feed runtime validation
Phase 19.2 - Multi-backend change feed aggregation
Phase 19.3 - Snapshot change feed REST validation
Phase 21.0 - Real VDR Runtime Polling Findings
Phase 21.2 - Selective Event Query Contract
ADR-0021 - Selective Backend Query Strategy
```

---

## Roadmap Progress

Completed Major Phases:

```text
Phase 0 - Phase 26.1
```

Planned Major Phases:

```text
Phase 25.x - Phase 34.x
```

Overall Roadmap Progress:

```text
Phase 26.0 completed; mid-term roadmap now continues with EPG decoupling validation and Phase 26.x - Phase 34.x
```

Important:

This progress description summarizes documented roadmap direction by major phase. It is not a code coverage metric and not a production-readiness guarantee.

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
| Live Transport | Live transport foundation is implemented. Future transport extensions remain possible above the snapshot change feed. |
| Selective Event Queries | Backend-neutral query contracts allow selective EPG access through adapter boundaries. |
| Heavy Domain Policy | Events / EPG are classified as a heavy domain and protected from automatic full refresh behavior. |
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
