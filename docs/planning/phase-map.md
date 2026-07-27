# VDR-Suite Phase Map

## Purpose

This file is the canonical compact phase-number map. Detailed history belongs in [Completed Phases](../development/completed-phases.md); strict future order belongs in [Roadmap](roadmap.md).

## Completed phase ranges

| Range | Status | Track | Result |
| --- | --- | --- | --- |
| Phase 1.x-7.x | Completed | Core Platform | Database, repositories, services, REST and daemon foundation. |
| Phase 8.x | Completed | VDR Backend | VDR domain and RESTfulAPI adapter foundations. |
| Phase 9.x-29.x | Completed | Multi-Backend Runtime | Registry, snapshots, selective reads, change feed and live transport foundation. |
| Phase 30.x-44.x | Completed | Recording Actions and Hardening | Validation, guarded execution, real-backend regression and safety transition. |
| Phase 45.x | Completed | EPG Search | Selective EPG query/search foundation. |
| Phase 46.x | Completed | Metadata and People | Classification, metadata and people foundations. |
| Phase 47.x-50.50 | Completed | SearchTimer | Backend, compatibility, preview, safety gates, readback and controlled workflow. |
| Phase 51.x-55.6 | Completed | Live/Adapter/Acceptance | Parity discovery, preview runtime, adapter hardening, acceptance and documentation. |
| Phase 56 | Completed | Library Boundary and Packaging | Source boundaries, packaging, staging and developer documentation. |
| Phase 57 | Completed | Backend Administration and Permissions | Backend access modes and server-enforced read-only foundation. |
| Phase 58.0-58.90b | Completed slices; umbrella historical | Frontend and Live Parity | Frontend foundations, EPG input, channel movement and sorting. |
| Phase 59.00-59.15e | Completed | Frontend Client API and Modules | Client API consolidation and ownership guards. |
| Phase 60.1-60.15 | Completed | Frontend Platform and Metadata Preparation | Recordings 2, lazy cache, metadata and authenticated artwork preparation. |
| Phase 61 | Completed | Suite Metadata and Genre Platform | Persistent Recording/EPG metadata, people and Genre assignments, query-only browse paths and frontend integration. |

## Completed non-numbered blocks

| Block | Status | Result |
| --- | --- | --- |
| Post-Phase 61 Performance Hardening (B1-B4) | Completed | PRs #102-#108: query, transaction, no-op and snapshot-cadence hardening. |
| VDR Remote and Live Overlay hardening (#110) | Completed | Isolated pressed-state and duplicate-dispatch guard while preserving backend-neutral routing. |
| Backend-scoped Global Search (#111) | Completed | Persisted Recording/EPG title, subtitle and people search with query-only reads. |

These blocks do not consume or invent a phase number.

## Current position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Latest completed cross-cutting features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Planned phase sequence

| Order | Phase | Status | Track | Goal |
| ---: | --- | --- | --- | --- |
| 1 | Phase 62 | Next | Identity, RBAC and Accountability | Actor identities, scoped authorization and append-only accountability. |
| 2 | Phase 63 | Planned | Backend Agent and Multi-Site Runtime | Enrollment, protected transport, generation, lease, health and fenced commands. |
| 3 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler/reconciler. |
| 4 | Phase 65 | Planned | Streaming Gateway | Authenticated short-lived media sessions over private providers. |
| 5 | Phase 66 | Planned | Legacy OSD Bridge | Isolated view/control compatibility with sequencing and controller lease. |
| 6 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions and compatibility. |
| 7 | Phase 68 | Vision | Recommendation and Knowledge Graph | Explainable recommendations after platform foundations mature. |

## Numbering rules

- Completed history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phase 61 is closed for its accepted metadata/Genre runtime scope.
- Optional providers and diagnostics do not silently reopen Phase 61.
- Cross-cutting completed PRs may be recorded without inventing a phase.
- The next available runtime phase in this sequence is Phase 62.

## Verification

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

## Related documents

- [Current State](../CURRENT.md)
- [Roadmap](roadmap.md)
- [Completed Phases](../development/completed-phases.md)
- [Phase 61 Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Closeout](../development/post-phase-61-platform-runtime-closeout.md)