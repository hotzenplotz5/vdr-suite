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
