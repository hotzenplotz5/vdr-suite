# VDR-Suite Project Status Dashboard

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)
- [Current Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)
- [Completed Phases](development/completed-phases.md)

---

<!-- PROJECT_PROGRESS_START -->
## Project Progress

Overall project progress:

    ████████░░ 77%

Milestone progress:

    Core Runtime Foundation   ██████████ 100%  completed
    Multi-Backend Foundation  ██████████ 100%  completed
    Query Foundation          ██████████ 100%  completed
    Action Foundation         ██████████ 100%  completed
    Metadata Foundation       ██████████ 100%  completed
    Documentation Foundation  ██████████ 100%  completed
    SearchTimer Foundation    ██████████  95%  in progress
    Automation Foundation     ░░░░░░░░░░   0%  planned
    Federation Foundation     ░░░░░░░░░░   0%  planned
    Frontend Foundation       ░░░░░░░░░░   0%  planned

Current milestone:

    Phase 47.0 - SearchTimer Foundation

Progress source: [Project Progress](planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Release State

### Core Platform

```text
Backend Foundation              complete
Snapshot Runtime                complete
Read API                        complete
Change Feed                     complete
Backend Registry                implemented
Multi-Backend Routing           implemented
Multi-Backend Polling           implemented
Multi-Backend Read API          implemented
Live Transport                  implemented
Selective Event Queries         implemented
Heavy Domain Policy             implemented
EPG REST API Boundary           implemented
EPG Search API                  implemented
Recording Query API             implemented
Recording Actions               implemented + diagnostics
Content Classification          ADR + foundation implemented
Person Metadata                 implemented foundation
Recording Person Search         implemented foundation
Recording Character Search      implemented foundation
EPG Person Search               started
SearchTimer Route               implemented
SearchTimer Daemon Provider     implemented
SearchTimer Backend Contract    documented
SearchTimer Payload Validation  documented
SearchTimer Domain Model        expanded
```

### Federation and Security

```text
Multi-VDR                       foundation implemented
Backend Registry                runtime + API implemented
Multi-Snapshot Cache            implemented
Backend-Aware Snapshots         implemented
Runtime Contexts                implemented
Capability System               foundation implemented
Authentication                  planned
Authorization                   planned
Profiles / Policy               planned for Phase 49
```

### Client Platforms

```text
Web Frontend                    planned
Windows Frontend                planned
Android Frontend                planned
iOS Frontend                    planned
TV Frontend                     planned
Hisense / VIDAA Strategy        future evaluation
```

### Media Extensions

```text
Content Classification          ADR-0028 documented
Genre Foundation                implemented foundation
Genre Resolution                implemented foundation
Genre JSON Contract             implemented
Canonical Genre Registry        implemented
Genre Localization              implemented foundation
Localized Genre JSON            implemented
Genre Architecture Docs         complete
Person Metadata                 implemented foundation
Recording Person Metadata       implemented foundation
Recording Character Search      implemented foundation
EPG Person Search               started
Content Rating / FSK            planned
Image Validation                planned
Preview Streams                 planned
Media Streaming                 planned
```

---

## Current Position

Current Major Phase:

```text
Phase 48.3 - EPGSearch result model audit
```

Current Documentation Consolidation:

```text
Phase 46.39 - Project Status Dashboard Refresh
```

Next Major Implementation Milestone:

```text
Phase 48.4 - EPGSearch service interface
```

Latest Completed Milestones:

```text
Person Metadata Foundation
Recording Person Search Foundation
Recording Character Search Foundation
```

Current Milestone In Progress:

```text
EPG Person Search Foundation
```

---

## Recently Completed Milestones

```text
EPG Search Foundation
Recording Query Foundation
Recording Action Foundation
Content Classification Foundation
Person Metadata Foundation
Recording Person Search Foundation
Recording Character Search Foundation
```

Representative completed phases:

```text
Phase 45.x  - EPG Search Foundation
Phase 46.0  - Content Classification Architecture ADR
Phase 46.16 - Person REST Boundary
Phase 46.18 - Person Query Model
Phase 46.24 - Person Query Documentation
Phase 46.26 - Recording Additional Media Person Import
Phase 46.32 - Snapshot-backed Recording Person Search Wiring
Phase 46.34 - Real VDR Person Metadata Validation
Phase 46.35 - Recording Character Search
Phase 46.36 - Recording Character Search API Documentation
Phase 46.37 - EPG Person Search Result Model
```

---

## Roadmap Progress

Completed Major Foundations:

```text
Core platform foundation
VDR backend foundation
Multi-backend runtime foundation
Snapshot runtime foundation
Change feed foundation
Live transport foundation
Capability foundation
Selective EPG query foundation
EPG search API foundation
Recording query foundation
Recording action foundation
Content classification foundation
Person metadata foundation
Recording person search foundation
Recording character search foundation
```

Current Foundation:

```text
EPG person search foundation
```

Planned Major Direction:

```text
Phase 47.x - SearchTimer Foundation
Phase 48.x - Unified Search Foundation
Phase 49.x - Profiles, Permissions and Policy
Phase 50.x - Backend Management Foundation
Phase 51.x - Live Plugin Parity Foundation
Phase 52.x - SearchTimer Automation
Phase 53.x - Recommendation Foundation
Phase 54.x - Cross Backend Search and Federation
Phase 55.x - Content Knowledge Graph
```

Important:

This progress description summarizes documented roadmap direction by major milestone. It is not a code coverage metric and not a production-readiness guarantee.

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
| EPG Search API | The EPG search API is implemented and documented over selective EPG windows. |
| Recording Query API | Recording query reads support title, path, start-time, duration, sorting and paging. |
| Recording Actions | Recording action validation and execution foundations are implemented with diagnostics. |
| Content Classification | ADR-0028 defines source-aware classification for genre, rating, keywords, collections, user tags and folder hints. |
| Person Metadata | Person domain, roles, sources, query, matching, service, JSON and REST boundaries are implemented. |
| Recording Person Search | Recording-attached person metadata is searchable with recording and backend context. |
| Recording Character Search | TVScraper actor role data is exposed as characterName and searchable. |
| EPG Person Search | The result model foundation is implemented; service and API wiring remain planned. |
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
- [Completed Phases](development/completed-phases.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)
