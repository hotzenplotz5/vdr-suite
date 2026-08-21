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
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Completed Phase-65 product verticals:
65.A - Existing-Recording playback
65.B - Live-TV playback
65.C - Recording startup / progressive-direct
65.D - Media-transcode backend policy and output settings

Next Phase-65 product vertical:
65.E - Client playback abstraction
```

The exact merged checkpoint and completion evidence are intentionally not duplicated here. Read [Current State](../CURRENT.md), [Phase 64 Closeout](../development/phase-64-closeout.md), [Phase 65 Recording Playback Closeout](../development/phase-65-recording-playback-closeout-readiness.md), [Phase 65 Live-TV Playback Closeout](../development/phase-65-live-tv-closeout.md), [Phase 65.C Recording Startup](../development/phase-65-recording-startup-progressive-direct.md) and [Phase 65 Media Transcode Performance Policy](../development/phase-65-media-transcode-performance-policy.md).

## Revised numbered forward sequence

| Order | Phase | Status | Track | Primary completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Reliable intent/assignment/binding orchestration, safe managed native fulfillment, authoritative readback, reconciliation, controlled reassignment and real-system write acceptance. |
| 2 | Phase 65 | Active | Streaming Gateway and Media Sessions | Authorized Recording + Live playback through MediaSession/Gateway, explicit provider leases, least-transformation delivery and real picture/sound acceptance. Recording playback, Live-TV, completed-Recording startup/progressive delivery and media-transcode backend policy/settings are accepted; client playback abstraction is next. |
| 3 | Phase 66 | Planned after Phase 65 | Broadcast Companion Services: Teletext and HbbTV | Domain-first Teletext pages plus broadcast-application discovery/session runtime without reducing them to OSD proxying. |
| 4 | Phase 67 | Planned after Phase 66 | Legacy OSD Compatibility Bridge | Isolated OSD observation, sequencing/resync, exclusive controller lease and allowlisted native input. |
| 5 | Phase 68 | Planned after Phase 67 | Public API and Client Compatibility Hardening | Stabilized `/api/v1`, errors, revisions/preconditions, pagination, compatibility/deprecation and independent-client contracts. |
| 6 | Phase 69 | Vision | Recommendation and Content Knowledge Graph | Explainable, provenance-aware recommendations after stable identities, privacy, accountability and public resource semantics mature. |

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

Binding decisions: ADR-0046 + ADR-0053 + ADR-0055 for media-transcode backend selection.

```text
private media source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> least-transformation adaptation
  -> Streaming Gateway / MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Product order now represented by accepted implementation history:

```text
Recording playback [65.A CLOSED]
  -> Live TV [65.B CLOSED]
  -> Recording startup / progressive-direct [65.C CLOSED]
  -> Media-transcode backend policy and output settings [65.D CLOSED]
  -> Client playback abstraction [65.E NEXT]
```

Current implementation position:

- existing-Recording browser playback, copy/transcode selection, interlace handling, HLS buffering/segmentation and calibrated VAAPI UHD adaptation have passed real yaVDR acceptance;
- graceful stop/pagehide cleanup and server-owned hard-disconnect cleanup have passed real yaVDR acceptance;
- 65.A existing-Recording playback is closed for its bounded scope;
- 65.B Live-TV playback is closed after exact-head CI plus real yaVDR picture/sound, repeated zap and 15-minute stability acceptance;
- 65.C completed-Recording startup/performance is closed through the accepted progressive-fMP4/progressive-direct vertical from PR #206; the continuous fMP4 path deliberately does not advertise fake Range/Content-Length/time-seek semantics;
- completed-versus-growing source truth remains explicit and the completed-Recording fast paths fail closed when immutable-source guarantees are not valid;
- 65.D media-transcode backend policy/settings is closed through PR #208, including backend-scoped `auto` / `software` / `vaapi`, calibrated selection, hard VAAPI capability checks, session-stable settings, browser settings diagnostics and fail-closed forced-VAAPI behavior;
- full arbitrary VOD time-seek/VDR-index mapping and user-visible growing-Recording seek remain deferred capability work, not the current 65.C label; Phase-65 acceptance requires truthful capability advertisement, not invented support;
- 65.E Client playback abstraction is the next planned Phase-65 vertical.

Browser is the initial first-party product-validation client. Streamdev may be an internal explicitly owned provider but is not the public media API. Android/Android TV, Kodi, desktop and television clients remain capability-driven and may select cheaper direct/remux profiles when supported.

## Phase 66 compact boundary

Binding architecture: accepted ADR-0054.

Teletext and HbbTV are television-domain capabilities and therefore precede Legacy OSD compatibility.

```text
Live Channel / ProgramEvent
  +--> TeletextService -> Page/Subpage
  +--> BroadcastApplication -> HbbTV Application Session
```

Normal Teletext browsing does not require a VDR OSD frame. HbbTV application discovery/runtime does not expose raw local plugin URL/JS/key command channels as a public API.

## Phase 67 compact boundary

Binding architecture: ADR-0047.

```text
LegacyOsdSession
  -> immutable OSD full frame / delta
  -> sequence + resync
  -> viewer bindings
  -> optional exclusive controller lease
  -> allowlisted input
```

This remains compatibility-only. Domain-first EPG, Timer, Recording, Streaming, Teletext and HbbTV surfaces remain preferred.

## Phase 68 compact boundary

Binding architecture: ADR-0048.

```text
/api/v1
  -> stable Suite resource IDs
  -> stable error semantics
  -> revisions / preconditions / idempotency
  -> deterministic collections / partial results
  -> compatibility and deprecation tests
```

The Agent protocol, Media Plane, OSD data plane and plugin-local contracts remain independently versioned.

## Phase 69 compact boundary

Recommendation/knowledge-graph runtime requires its own accepted ADR before implementation. It may consume stable metadata, people, genre, ProgramEvent, Recording and actor-preference evidence, but it does not gain hidden mutation authority.

## Cross-cutting non-numbered milestones

These are intentionally not inserted between numbered runtime phases:

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement migration;
- first-party client family rollout.

The Broad Timer Product UI depends on completed Phase 62 + completed Phase 64 + the required account/backend access administration. It may proceed alongside Phase 65 without blocking Streaming.

## Product acceptance

Vertical product acceptance is maintained in [Golden User Journeys](golden-user-journeys.md).

- Phase 64 uses Timer scheduling/fail-closed journeys for engine completion.
- Phase 65 owns Live-TV and Recording-playback journeys.
- Phase 66 adds Teletext and HbbTV journeys.
- Phase 67 adds one explicit Legacy OSD compatibility journey.
- the Broad Timer Product UI later completes the user-facing Timer journey without reopening Phase 64.

## Numbering rules

- Completed history is never renumbered.
- Phases 61, 62, 63 and 64 are closed for their accepted scopes.
- Phase 65 is the active numbered runtime phase.
- Phase 65.A existing-Recording playback is closed for its accepted scope.
- Phase 65.B Live-TV playback is closed for its accepted scope.
- Phase 65.C Recording startup / progressive-direct is closed for its accepted scope.
- Phase 65.D media-transcode backend policy and output settings is closed for its accepted scope.
- Phase 65.E Client playback abstraction is next in the current Phase-65 sequence.
- Truthful range/seek/growing-recording capability remains a Phase-65 invariant even where advanced seek is not implemented.
- Phase 65 does not close until the remaining required acceptance gates in the Strict Roadmap are satisfied.
- Future phases 66+ may be reordered only before runtime starts and only through explicit repository planning/architecture reconciliation.
- Broad Timer UI completion is not inserted as a numbered phase between 64 and 65.
- Cross-cutting product/admin work does not silently advance the numbered runtime phase.
- Draft planning or ADR work does not by itself authorize runtime implementation.

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
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Completed Phases](../development/completed-phases.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
