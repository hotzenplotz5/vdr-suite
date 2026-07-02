# VDR-Suite Project Status Dashboard

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)
- [Current Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Lazy Recording Loading](planning/lazy-recording-loading.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)
- [Completed Phases](development/completed-phases.md)

---

<!-- PROJECT_PROGRESS_START -->
## Project State Snapshot

This is a verified implementation-state snapshot, not a product-completion percentage.

### Verified foundations

- Core runtime and daemon foundation
- VDR backend adapter and RESTfulAPI integration foundation
- Backend registry and multi-backend runtime foundation
- Snapshot cache, snapshot access and change-feed foundation
- REST routing and JSON response boundaries
- Recording query foundation
- Recording action validation and guarded execution foundation
- Selective EPG query and EPG search foundation
- Content classification and person metadata foundations
- Recording person and character search foundations
- SearchTimer backend, validation, planning and workflow foundations
- SearchTimer safety gates, readback verification and production mutation policy foundations
- Live parity discovery foundation
- Real VDR acceptance manifest and runner foundation
- Daemon lifecycle hardening for duplicate bind failures and SIGTERM shutdown
- Recording operations audit and safety policy foundation

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.\n- Phase 58.39 verifies bounded live EPG input for channel cards via the now-next EPG route.

### Guarded or deliberately incomplete areas

- SearchTimer production mutation remains gated and closed by default.
- Recording operations real-backend write probes remain explicitly gated.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- Suite-owned metadata database and external scraper/provider strategy are planned but not yet implemented as the final metadata product.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy are still planned.
- Web, Windows, Android, iOS and TV frontends remain planned product layers; the current web frontend is a Phase 58 foundation, not the final client product.

### Current active focus

```text
Phase 58 - Frontend and Live Parity
```

### Later strategic milestones

- Multi-site backend federation and permissions
- Frontend and live-parity foundation
- Suite metadata database and external provider integration
- EPG cache database and SSE/change-state triggered background synchronization\n- Safe production-grade recording operations

Progress source: planning/project-progress.md
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
Lazy Recording Loading          planned
Content Classification          ADR + foundation implemented
Person Metadata                 implemented foundation
Recording Person Search         implemented foundation
Recording Character Search      implemented foundation
EPG Person Search               result model foundation
SearchTimer Route               implemented
SearchTimer Daemon Provider     implemented
SearchTimer Backend Contract    documented
SearchTimer Payload Validation  documented
SearchTimer Domain Model        expanded
Native Fuzzy Capability         validated end-to-end
SearchTimer User Workflow       completed foundation + verified execution
SearchTimer Preview EPG Cache   ADR-0034 documented
Real VDR Acceptance             20/20 safe/dry-run probes verified
Daemon Lifecycle Hardening       duplicate bind + SIGTERM verified
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
Profiles / Policy               planned future concern
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
Person Metadata                 implemented foundation
Recording Person Metadata       implemented foundation
Recording Character Search      implemented foundation
Content Rating / FSK            planned
Image Validation                planned
Preview Streams                 planned
Media Streaming                 planned
```

---

## Current Position

Current Major Phase:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Current Documentation Consolidation:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Next Major Implementation Milestone:

```text
Phase 58 - Frontend and Live Parity
```

Required Planned Follow-Up:

```text
Frontend and Live Parity
```

Latest Completed Milestones:

```text
Person Metadata Foundation
Recording Person Search Foundation
Recording Character Search Foundation
EPGSearch Native Fuzzy Capability Validation
SearchTimer Runtime Mutation Policy
SearchTimer Warm EPG Cache Architecture
Real VDR Acceptance Foundation
Daemon Runtime Lifecycle Hardening
Documentation Handoff Verification
Recording Operations Audit and Safety Policy
```

Current Milestone In Progress:

```text
Frontend and Live Parity
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
EPGSearch Native Fuzzy Capability Validation
SearchTimer Warm EPG Cache Architecture
```

Representative completed phases:

```text
Phase 45.x  - EPG Search Foundation
Phase 46.x  - Metadata and person foundations
Phase 47.x  - SearchTimer backend foundation
Phase 49.x  - EPGSearch native fuzzy capability validation
Phase 50.x  - SearchTimer user workflow foundation
Phase 53.x  - SearchTimer title-only workflow preservation
Phase 54.1  - SearchTimer preview comparison-option fix
Phase 54.2  - SearchTimer warm EPG cache architecture
Phase 55.5  - Real VDR acceptance and daemon lifecycle hardening
Phase 55.5o - Phase map and roadmap simplification
Phase 55.6  - Recording operations audit and safety policy
Phase 57.x - Multi-site backend administration and permissions
Phase 58.38 - SearchTimer frontend cockpit and mobile UI polish
Phase 58.39 - Bounded live EPG for channel cards
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
Native fuzzy backend capability validation
SearchTimer warm EPG cache architecture
```

Current Foundation:

```text
Frontend and Live Parity
```

Planned Major Direction:

```text
Phase 58 - Frontend and Live Parity
Phase 57 - Multi-Site Backend Administration and Permissions
Phase 58 - Frontend and Live Parity
Phase 59 - Suite Metadata Database and External Providers
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
| Heavy Domain Policy | Events / EPG and recordings are classified as heavy domains and protected from automatic full refresh behavior. |
| EPG REST API Boundary | Selective EPG reads are exposed through backend-neutral REST routes. |
| EPG Search API | The EPG search API is implemented and documented over selective EPG windows. |
| SearchTimer | Backend route, daemon provider, domain model expansion, real payload validation, native fuzzy capability validation and user workflow foundation are implemented; production mutation remains gated. |
| SearchTimer Preview EPG Cache | ADR-0034 requires warm backend-scoped EPG input for interactive preview and forbids full EPG dumps per preview request. |
| Lazy Recording Loading | ADR-0035 requires startup to avoid synchronous full recording loads and requires backend-scoped on-demand recording refresh with explicit loading state. |
| Live Plugin Parity | Source audit, gap matrix, read-only discovery domain foundation and completion are documented and implemented. |
| SearchTimer Automation | Completed title-only repair and workflow audit while automation remains preview-only and mutation-gated. |
| Recording Query API | Recording query reads support title, path, start-time, duration, sorting and paging; future runtime must respect recording cache status. |
| Recording Actions | Recording action validation and execution foundations are implemented with diagnostics; future readback must refresh only the affected backend. |
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
- [Lazy Recording Loading](planning/lazy-recording-loading.md)
- [Completed Phases](development/completed-phases.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)
