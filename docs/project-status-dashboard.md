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
EPG REST API Boundary     implemented
EPG Search API            implemented
Recording Query API       implemented
Recording Actions         implemented + diagnostics
Content Classification    ADR accepted/proposed foundation
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
Profile / Policy          planned after classification
```

### Client Platforms

```text
Web Frontend              planned
Windows Frontend          planned
Android Frontend          planned
iOS Frontend              planned
TV Frontend               planned
Hisense / VIDAA Strategy  future evaluation
```

### Media Extensions

```text
Content Classification    ADR-0028 documented
Genre Foundation          implemented foundation
Genre Resolution          implemented foundation
Genre JSON Contract       implemented
Content Rating / FSK      planned
Image Validation          planned
Preview Streams           planned
Media Streaming           planned
```

---

## Current Position

Current Major Phase:

```text
Phase 46.3 - Genre JSON Contract
```

Current Focus:

```text
Phase 46.4 - Canonical Genre Registry
```

Latest Completed Milestone:

```text
Phase 46.3 - Genre JSON Contract
```

Architecture Work In Progress:

```text
ADR-0028 - Content Classification Architecture
Phase 46.1 - Genre Domain Foundation
Future: profiles, content rating, policy and TV frontend strategy
```

---

## Recently Completed Milestones

```text
Phase 45.1 - EPG Search Request Foundation
Phase 45.2 - EPG Search Matcher Foundation
Phase 45.3 - EPG Search Result Foundation
Phase 45.4 - EPG Search JSON Contract
Phase 45.5 - EPG Search Service Foundation
Phase 45.6 - EPG Search Controller Foundation
Phase 45.7 - EPG Search REST Validation
Phase 45.8 - EPG Search Documentation
Phase 46.0 - Content Classification Architecture ADR
ADR-0028 - Content Classification Architecture
```

---

## Roadmap Progress

Completed Major Foundation:

```text
Multi-backend runtime foundation
Selective EPG query foundation
Recording query foundation
Recording action runtime diagnostics
EPG search API foundation
Content classification ADR
```

Planned Major Direction:

```text
Phase 46.x - Content Classification and Genre Foundation
Phase 47.x - SearchTimer Foundation
Phase 48.x - Multi-Backend Unified Search
Phase 49.x - User Profiles, Policy and Content Rating
Phase 50.x - Streaming API Foundation
Phase 51.x - TV Frontend Strategy
Phase 52.x - Rectools Integration Layer
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
| EPG REST API Boundary | Selective EPG reads are exposed through backend-neutral REST routes. |
| EPG Search API | `/api/epg/search` is implemented and documented over selective EPG windows. |
| Content Classification | ADR-0028 defines source-aware classification for genre, rating, keywords, collections, user tags and folder hints. |
| Recording Query API | Recording query reads support title, path, start-time, duration, sorting and paging. |
| Capability System | Capability set, resolver state and capability report foundations are implemented. |
| Authentication | Planned future concern, not implemented yet. |
| Authorization | Planned future concern, not implemented yet. |
| Client Platforms | Future frontend work, not implemented yet. |
| Media Extensions | Content classification, images, preview streams and media stream validation remain future work. |

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
