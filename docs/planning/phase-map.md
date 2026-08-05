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
| Phase 62 | Completed | Identity, RBAC and Accountability | Persistent identities, scoped authorization, browser-session security, protected central mutations and append-only decision/outcome evidence. |

## Completed non-numbered blocks

| Block | Status | Result |
| --- | --- | --- |
| Post-Phase 61 Performance Hardening (B1-B4) | Completed | Query, transaction, no-op and snapshot-cadence hardening. |
| VDR Remote and Live Overlay hardening (#110) | Completed | Isolated pressed-state and duplicate-dispatch guard. |
| Backend-scoped Global Search (#111) | Completed | Persisted Recording/EPG title, subtitle and people search. |
| Configurable photorealistic VDR Remote (#115) | Completed | Backend-neutral configurable remote asset and interaction path. |

These blocks do not consume or invent a phase number.

## Current position

```text
Current merged main baseline:
24b1d7938ddaa15834a8da6323a270761868f4ba

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Completed Phase-63 slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
PR #137 merged and exact-head real yaVDR acceptance passed

Current active runtime slice:
Phase 63 Slice 2 - Backend Health Observation Ingestion Runtime
Draft PR #139 implements the first bounded read-only observation domain

Phase 63 is not complete
```

## Planned phase sequence

| Order | Phase | Status | Track | Goal |
| ---: | --- | --- | --- | --- |
| 1 | Phase 63 | Active Slice 2 | Backend Agent and Secure Multi-Site Runtime | Slice 1 completed enrollment, protected transport, generation, lease, capabilities and credential lifecycle. Slice 2 establishes generation-/sequence-fenced read-only Observation and Snapshot Ingestion; commands remain later. |
| 2 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler/reconciler. |
| 3 | Phase 65 | Planned | Streaming Gateway | Authenticated short-lived media sessions over private providers. |
| 4 | Phase 66 | Planned | Legacy OSD Bridge | Isolated view/control compatibility with sequencing and controller lease. |
| 5 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions and compatibility. |
| 6 | Phase 68 | Vision | Recommendation and Knowledge Graph | Explainable recommendations after platform foundations mature. |

## Phase 63 slice progression

| Slice | Status | Result / Goal |
| --- | --- | --- |
| Phase 63 Slice 1 | Completed and merged | Controlled Agent enrollment, technical identity, protected HTTPS transport, protocol/generation/instance fencing, heartbeat/lease, read-only capabilities, credential rotation/revocation/replacement and guarded real-system acceptance. |
| Phase 63 Slice 2 | Active runtime | PR #138 merged the contract; Draft PR #139 implements complete baseline plus exact-next change ingestion, idempotent replay, gap-driven `resync-required`, atomic receipt/cursor persistence and protected Agent retry for `backend-health`. |
| Later Phase 63 slices | Not started | Durable command/receipt/result flow, native execution and provider ownership/selection require separate contracts after read-only ingestion. |

## Numbering rules

- Completed history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phase 61 is closed for its accepted metadata/Genre runtime scope.
- Phase 62 is closed for its accepted identity, authorization and accountability scope.
- Optional providers, diagnostics and administration products do not silently reopen completed phases.
- Phase 63 is the active numbered runtime phase.
- Completion of Phase 63 Slice 1 does not complete Phase 63.
- Observation and Snapshot Ingestion precedes any command/result or VDR-native execution slice.

## Verification

```bash
make test-phase-map-coverage
make test-phase63-observation-ingestion-contract
make test-docs
make test-phase
```

## Related documents

- [Current State](../CURRENT.md)
- [Roadmap](roadmap.md)
- [Phase 63 Slice-1 Closeout](../development/phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](../development/phase-63-observation-ingestion.md)
- [Completed Phases](../development/completed-phases.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
