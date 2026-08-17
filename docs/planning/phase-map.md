# VDR-Suite Phase Map

## Purpose

This file is the canonical compact phase-number map. Detailed history belongs in [Completed Phases](../development/completed-phases.md); strict forward order and completion gates belong in the [Roadmap](roadmap.md); exact active heads and CI checkpoints belong only in [Current State](../CURRENT.md).

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
| Phase 63 | Completed | Backend Agent and Secure Multi-Site Runtime | Secure Agent lifecycle, fenced observations/commands/native execution, explicit provider ownership and protected-write foundation. |
| Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Durable intent/assignment/binding model, managed native Timer fulfillment, authoritative readback/reconciliation and controlled failover. |

## Current position

```text
Latest completed numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
none - Phase 65 has not started

Next strict numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions
```

The exact merged checkpoint and completion evidence are intentionally not duplicated here. Read [Current State](../CURRENT.md) and [Phase 64 Closeout](../development/phase-64-closeout.md).

## Numbered forward sequence

| Order | Phase | Status | Track | Completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Reliable intent/assignment/binding orchestration, safe managed native fulfillment, authoritative readback, reconciliation, controlled reassignment and real-system write acceptance. |
| 2 | Phase 65 | Next; not started | Streaming Gateway and Media Sessions | Authorized short-lived media sessions, private-provider routing, Live/Recording delivery and first vertical playback acceptance. |
| 3 | Phase 66 | Planned after Phase 65 | Legacy OSD Compatibility Bridge | Isolated OSD snapshot/control compatibility with sequencing and controller lease. |
| 4 | Phase 67 | Planned after Phase 66 | Public API and Client Compatibility Hardening | Stabilize versioned public contracts, errors, revisions, pagination and compatibility. |
| 5 | Phase 68 | Vision | Recommendation and Knowledge Graph | Explainable recommendations after metadata, identity, accountability and public contracts mature. |

## Phase 64 compact boundary

Phase 64 completed the accepted separation:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The completed engine includes deterministic primary/replica assignment, native binding/readback evidence, durable mutation-operation state, managed native create/update/toggle/delete execution, no-blind-retry semantics, authoritative reconciliation and controlled reassignment/failover with atomic ownership handover.

A broad polished Timer UI is not part of the Phase-64 completion gate. It remains separately gated on account/backend access management and may be completed after Phase 65 begins.

## Phase 65 compact boundary

Phase 65 follows the reliable Phase-64 engine, not the broad Timer UI. Accepted ADR-0046 owns the server MediaSession/Gateway boundary.

The intended media direction remains:

```text
private source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> media adaptation
  -> Streaming Gateway / MediaSession profile
  -> client adapter
  -> platform playback engine
```

Prefer pass-through before remux/repackage before transcode. Streamdev remains a private provider candidate rather than a public platform contract.

Phase 65 is the next strict phase, but it is not active until explicitly started.

## Product acceptance

Vertical product acceptance is maintained in [Golden User Journeys](golden-user-journeys.md).

- Phase 64 uses record-one-programme, multi-backend ownership and fail-closed recovery journeys to prove orchestration semantics.
- Phase 65 adds Live-TV and Recording-playback journeys through Suite MediaSession contracts.

## Numbering rules

- Completed history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phases 61, 62, 63 and 64 are closed for their accepted scopes.
- Optional providers, diagnostics and administration products do not silently reopen completed phases.
- Phase 65 is the next numbered runtime phase, but is not active until explicitly started.
- Broad Timer UI completion is not inserted as a numbered phase between 64 and 65.
- Draft planning or ADR work does not advance a numbered runtime phase by itself.

## Verification

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

## Related documents

- [Current State](../CURRENT.md)
- [Roadmap](roadmap.md)
- [Phase 64 Closeout](../development/phase-64-closeout.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Completed Phases](../development/completed-phases.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
