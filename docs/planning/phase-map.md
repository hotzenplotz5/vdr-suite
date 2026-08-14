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

## Completed non-numbered blocks

| Block | Status | Result |
| --- | --- | --- |
| Post-Phase 61 Performance Hardening (B1-B4) | Completed | Query, transaction, no-op and snapshot-cadence hardening. |
| VDR Remote and Live Overlay hardening (#110) | Completed | Isolated pressed-state and duplicate-dispatch guard. |
| Backend-scoped Global Search (#111) | Completed | Persisted Recording/EPG title, subtitle and people search. |
| Configurable photorealistic VDR Remote (#115) | Completed | Backend-neutral configurable remote asset and interaction path. |

Historical umbrella implementation track: **Phase 58 - Frontend and Live Parity**.

## Current position

```text
Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Next strict numbered runtime phase after Phase 64:
Phase 65 - Streaming Gateway and Media Sessions

Current implementation boundary:
planning hold after the PR-#190 checkpoint; Phase 64 is not complete
```

The exact PR head, branch relation and CI status are intentionally not duplicated here. Read `CURRENT.md` and live GitHub state.

## Numbered forward sequence

| Order | Phase | Status | Track | Completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Active; planning hold | Timer Intent and Multi-Backend Orchestration | Reliable intent/assignment/binding orchestration, safe managed native fulfillment, authoritative readback, reconciliation and real-system write acceptance. |
| 2 | Phase 65 | Planned after Phase-64 engine completion | Streaming Gateway and Media Sessions | Authorized short-lived media sessions, private-provider routing, Live/Recording delivery and first vertical playback acceptance. |
| 3 | Phase 66 | Planned after Phase 65 | Legacy OSD Compatibility Bridge | Isolated OSD snapshot/control compatibility with sequencing and controller lease. |
| 4 | Phase 67 | Planned after Phase 66 | Public API and Client Compatibility Hardening | Stabilize versioned public contracts, errors, revisions, pagination and compatibility. |
| 5 | Phase 68 | Vision | Recommendation and Knowledge Graph | Explainable recommendations after metadata, identity, accountability and public contracts mature. |

## Phase 64 compact boundary

Phase 64 separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The current stacked implementation checkpoint has already explored deterministic primary/replica planning, native binding/readback evidence, durable mutation-operation state and a fenced Timer-delete handoff to a concrete but disabled SuiteBridge transport.

That checkpoint is not Phase-64 completion. The reliable engine still requires the applicable ADR-0044 managed native lifecycle, safe mutation/replay semantics, authoritative reconciliation and real-VDR acceptance before production write enablement can close the phase.

A broad polished Timer UI is not a Phase-64 completion gate. It remains separately gated on account/backend access management and therefore may be completed after Phase 65 begins.

## Phase 65 compact boundary

Phase 65 follows the reliable Phase-64 engine, not the broad Timer UI. Accepted ADR-0046 owns the server MediaSession/Gateway boundary; Draft PR #156 contains the complementary proposed playback/media-adaptation strategy.

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

## Product acceptance

Vertical product acceptance is maintained in [Golden User Journeys](golden-user-journeys.md). In particular:

- Phase 64 uses record-one-programme, multi-backend ownership and fail-closed recovery journeys to prove orchestration semantics.
- Phase 65 adds Live-TV and Recording-playback journeys through Suite MediaSession contracts.

## Numbering rules

- Completed history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phases 61, 62 and 63 are closed for their accepted scopes.
- Optional providers, diagnostics and administration products do not silently reopen completed phases.
- Phase 64 is the active numbered runtime phase.
- Phase 65 is the next numbered runtime phase only after Phase-64 engine completion.
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
- [Golden User Journeys](golden-user-journeys.md)
- [Completed Phases](../development/completed-phases.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
