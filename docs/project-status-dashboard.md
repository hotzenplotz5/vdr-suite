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
- Backend-scoped persistent EPG database foundation
- Content classification and person metadata foundations
- Recording person and character search foundations
- SearchTimer backend, validation, planning and workflow foundations
- SearchTimer safety gates, readback verification and production mutation policy foundations
- Live parity discovery foundation
- Real VDR acceptance manifest and runner foundation
- Daemon lifecycle hardening for duplicate bind failures and SIGTERM shutdown
- Recording operations audit and safety policy foundation
- Channel move API and stable frontend channel sorter foundation

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.
- Phase 58.39 verifies bounded live EPG input for channel cards via the now-next EPG route.
- Phase 58.90a verifies guarded channel move API coverage.
- Phase 58.90b verifies stable channel sorting on desktop and touch devices.

### Guarded or deliberately incomplete areas

- SearchTimer production mutation remains gated and closed by default.
- Recording operations real-backend write probes remain explicitly gated.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- The persistent EPG database foundation is present, but EPG synchronization service, daemon scheduling and frontend consumers are not wired to it yet.
- Suite-owned metadata database and external scraper/provider strategy are planned but not yet implemented as the final metadata product.
- User accounts, permissions, profiles and policy hardening remain planned future work.
- Web, Windows, Android, iOS and TV frontends remain planned product layers; the current web frontend is a Phase 58 foundation, not the final client product.

### Current active focus

```text
Phase 58 - Frontend and Live Parity
```

### Later strategic milestones

- Multi-site backend federation and permission hardening
- Frontend and live-parity foundation
- EPG synchronization service and SSE/change-state triggered background synchronization
- Suite metadata database and external provider integration
- Safe production-grade recording operations

Progress source: planning/project-progress.md
<!-- PROJECT_PROGRESS_END -->

---

## Current Release State

### Core Platform

```text
Backend Foundation                         complete
Snapshot Runtime                           complete
Read API                                   complete
Change Feed                                complete
Backend Registry                           implemented
Multi-Backend Routing                      implemented
Multi-Backend Polling                      implemented
Multi-Backend Read API                     implemented
Live Transport                             implemented
Selective Event Queries                    implemented
Heavy Domain Policy                        implemented
EPG REST API Boundary                      implemented
EPG Search API                             implemented
Backend-Scoped EPG DB Foundation           implemented
Recording Query API                        implemented
Recording Actions                          implemented + diagnostics
Channel Move API                           implemented + guarded
Stable Channel Sorter                      implemented frontend slice
Lazy Recording Loading                     planned
Content Classification                     ADR + foundation implemented
Person Metadata                            implemented foundation
Recording Person Search                    implemented foundation
Recording Character Search                 implemented foundation
SearchTimer Route                          implemented
SearchTimer Daemon Provider                implemented
SearchTimer User Workflow                  completed foundation + verified execution
Real VDR Acceptance                        20/20 safe/dry-run probes verified
Daemon Lifecycle Hardening                 duplicate bind + SIGTERM verified
```

### Backend and Clients

```text
Multi-VDR                                  foundation implemented
Backend Registry                           runtime + API implemented
Multi-Snapshot Cache                       implemented
Backend-Aware Snapshots                    implemented
Runtime Contexts                           implemented
Capability System                          foundation implemented
Web Frontend                               Phase 58 foundation in progress
Windows Frontend                           planned
Android Frontend                           planned
iOS Frontend                               planned
TV Frontend                                planned
```

---

## Current Position

Latest Completed Major Phase:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Latest Completed Implementation Slice:

```text
Phase 58.90b - Stable Channel Sorter
```

Current Documentation Consolidation:

```text
Phase 58.90c - Documentation Consolidation
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
Backend-Scoped Persistent EPG Database Foundation
Real VDR Acceptance Foundation
Daemon Runtime Lifecycle Hardening
Documentation Handoff Verification
Recording Operations Audit and Safety Policy
Channel Move API
Stable Channel Sorter
```

Current Milestone In Progress:

```text
Frontend and Live Parity
```
