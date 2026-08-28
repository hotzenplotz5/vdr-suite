# VDR-Suite Phase Map

## Purpose

This file is the canonical compact phase-number map. Detailed completed history belongs in [Completed Phases](../development/completed-phases.md); strict forward execution order and completion gates belong in the [Roadmap](roadmap.md); volatile completed/active/next status belongs only in [Current State](../CURRENT.md).

Future phase numbers may be reordered only before those phases start and only through an explicit planning/architecture reconciliation. Completed history is never renumbered.

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

Historical exact foundation marker retained for contract traceability: `Phase 63 - Backend Agent and Secure Multi-Site Runtime`. It is completed history, not current execution state.

Phase 58 remains a historical umbrella label only.

## Current position

```text
Latest completed numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
none - Phase 66 has not started

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience
```

Read [Current State](../CURRENT.md) and [Phase 65 Closeout](../development/phase-65-closeout.md) for exact operational/evidence state. ADR-0058 and the [Phase 66 Media Home contract](../development/phase-66-media-home-browse-experience.md) define the accepted next planning boundary; runtime is not started.

## Revised numbered forward sequence

| Order | Phase | Status | Track | Primary completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Reliable Timer intent/assignment/binding orchestration and controlled failover. |
| 2 | Phase 65 | Completed | Streaming Gateway and Media Sessions | Authenticated Recording/Live playback, least-transformation delivery/output policy and normalized persistent playback semantics. |
| 3 | Phase 66 | Next; not started | Media Home and Browse Experience | Responsive Home, immediate browse, deferred canonical preview, truthful Continue Watching and product acceptance. |
| 4 | Phase 67 | Planned after Phase 66 | Broadcast Companion Services: Teletext and HbbTV | Domain-first Teletext and broadcast-application runtime. |
| 5 | Phase 68 | Planned after Phase 67 | Legacy OSD Compatibility Bridge | Isolated OSD observation/control compatibility. |
| 6 | Phase 69 | Planned after Phase 68 | Public API and Client Compatibility Hardening | Stable `/api/v1` and independent-client contracts. |
| 7 | Phase 70 | Vision | Recommendation and Content Knowledge Graph | Explainable provenance-aware recommendations after prerequisites. |

## Phase 64 compact boundary

Phase 64 completed:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

including deterministic primary/replica assignment, managed create/update/toggle/delete fulfillment, native readback evidence, durable mutation-operation state, no-blind-retry semantics and controlled atomic reassignment/failover.

The broad polished Timer UI is not part of the Phase-64 completion gate.

## Phase 65 compact boundary

Binding decisions: ADR-0046 + ADR-0053 + ADR-0055 + ADR-0056 + ADR-0057.

```text
private media source
  -> explicitly owned provider / ProviderStreamLease
  -> least-transformation adaptation
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

Phase 65 is completed. Growing-Recording seek and Live-TV timeshift remain truthful deferred capabilities rather than unfinished Phase 65. Durable evidence: [Phase 65 Closeout](../development/phase-65-closeout.md).

## Phase 66 compact boundary

Binding architecture: accepted ADR-0058; implementation contract: [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md).

```text
existing Channel / EPG / Recording / Metadata truth
  -> responsive Home projection
  -> immediate browse focus
  -> optional deferred preview through canonical Phase-65 playback ownership
  -> explicit full playback through existing owners
```

Phase 66 is next but not started.

## Phase 67 compact boundary

Binding architecture: accepted ADR-0054.

```text
Live Channel / ProgramEvent
  +--> TeletextService -> Page/Subpage
  +--> BroadcastApplication -> HbbTV Application Session
```

## Phase 68 compact boundary

Binding architecture: ADR-0047. Legacy OSD remains compatibility-only; domain-first Home/EPG/Timer/Recording/Streaming/Teletext/HbbTV remains preferred.

## Phase 69 compact boundary

Binding architecture: ADR-0048. Stabilize `/api/v1`, errors, revisions/preconditions/idempotency, deterministic collections and compatibility/deprecation contracts.

## Phase 70 compact boundary

Recommendation/knowledge-graph runtime requires its own accepted ADR and does not gain hidden mutation authority.

## Cross-cutting non-numbered milestones

These are intentionally not inserted between numbered runtime phases:

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement migration;
- first-party client family rollout.

The Broad Timer Product UI depends on completed Phase 62 + completed Phase 64 + the required account/backend access administration. It may proceed alongside Phase 65 without blocking Streaming.

## Product acceptance

- Phase 64: Timer scheduling/fail-closed engine journeys.
- Phase 65: completed Live-TV and Recording-playback journeys.
- Phase 66: desktop and mobile Media Home browse/preview journeys.
- Phase 67: Teletext and HbbTV journeys.
- Phase 68: explicit Legacy OSD compatibility journey.
- Phase 69: public/client compatibility hardening.
- Broad Timer Product UI later completes its user-facing journey without reopening Phase 64.
- Phase 70 recommendation work must add its own accepted journey.

See [Golden User Journeys](golden-user-journeys.md).

## Numbering rules

- Completed history is never renumbered.
- Phases 61 through 65 are closed for their accepted scopes.
- Phase 66 Media Home and Browse Experience is next and has not started.
- ADR-0058 acceptance is planning authority, not runtime kickoff.
- Phase 67 Broadcast Companion retains ADR-0054 architecture.
- Phase 68 Legacy OSD retains ADR-0047 architecture.
- Phase 69 Public API hardening retains ADR-0048 architecture.
- Phase 70 Recommendation / Knowledge Graph remains vision and requires its own accepted runtime ADR.
- Future not-yet-started phases may be reordered only through explicit repository reconciliation.
- Cross-cutting product/admin work does not silently advance the numbered phase.

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
- [Phase 65 Recording Playback Closeout](../development/phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](../development/phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup](../development/phase-65-recording-startup-progressive-direct.md)
- [Phase 65 Media Transcode Performance Policy](../development/phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](../development/phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](../development/phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md)
- [ADR-0056 Playback Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Completed Phases](../development/completed-phases.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
