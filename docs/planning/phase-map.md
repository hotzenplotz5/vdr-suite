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
65.C - Recording delivery performance and media output/transcode settings

Current Phase-65 product vertical:
65.D - Client playback abstraction

Accepted Phase-65.D bounded work:
65.D.1 - Persistent Browser Playback Shell
65.D.2 - Recording Playback Controls and Seek
normalized Recording audio/subtitle selection
browser-local Volume/Mute
continuous-fMP4 browser MSE forward-buffer control
compatibility timeline ownership
exact non-zero HLS Recording resume synchronization

Active Phase-65.D architecture gate:
ADR-0056 playback semantic consolidation
```

The exact merged checkpoint and completion evidence are intentionally not duplicated here. Read [Current State](../CURRENT.md), [Phase 64 Closeout](../development/phase-64-closeout.md), [Phase 65 Recording Playback Closeout](../development/phase-65-recording-playback-closeout-readiness.md), [Phase 65 Live-TV Playback Closeout](../development/phase-65-live-tv-closeout.md), [Phase 65.C Recording Startup](../development/phase-65-recording-startup-progressive-direct.md), [Phase 65 Media Transcode Performance Policy](../development/phase-65-media-transcode-performance-policy.md), [Phase 65.D.1 Persistent Browser Playback Shell Closeout](../development/phase-65d1-persistent-browser-playback-shell-closeout.md), [Phase 65.D.2 Recording Playback Controls and Seek Closeout](../development/phase-65d2-recording-playback-controls-seek-closeout.md), [ADR-0056](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md) and [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md).

## Revised numbered forward sequence

| Order | Phase | Status | Track | Primary completion direction |
| ---: | --- | --- | --- | --- |
| 1 | Phase 64 | Completed | Timer Intent and Multi-Backend Orchestration | Reliable intent/assignment/binding orchestration, safe managed native fulfillment, authoritative readback, reconciliation, controlled reassignment and real-system write acceptance. |
| 2 | Phase 65 | Active | Streaming Gateway and Media Sessions | Authorized Recording + Live playback through MediaSession/Gateway, explicit provider leases, least-transformation delivery and real picture/sound acceptance. Recording playback, Live-TV, completed-Recording startup/progressive delivery, backend-scoped media-transcode policy and the accepted Phase-65.D playback/control/track/buffer/timeline/resume work are complete for their bounded scopes; ADR-0056 semantic consolidation remains active. |
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

Binding decisions: ADR-0046 + ADR-0053 + ADR-0055 + ADR-0056.

```text
private media source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> least-transformation adaptation / internal MediaPresentationProfile
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

Product order represented by accepted implementation history:

```text
Recording playback [65.A CLOSED]
  -> Live TV [65.B CLOSED]
  -> Recording delivery performance + media output/transcode settings [65.C CLOSED]
  -> Client playback abstraction [65.D ACTIVE]
       -> Persistent Browser Playback Shell [CLOSED]
       -> Recording Playback Controls and Seek [CLOSED]
       -> normalized audio/subtitle selection [CLOSED]
       -> browser-local Volume/Mute [CLOSED]
       -> continuous-fMP4 MSE forward buffering [CLOSED]
       -> compatibility timeline + exact HLS resume [CLOSED]
       -> ADR-0056 playback semantic consolidation [ACTIVE]
```

Current implementation position:

- existing-Recording browser playback, copy/transcode selection, interlace handling, HLS buffering/segmentation and calibrated VAAPI UHD adaptation have passed real yaVDR acceptance;
- graceful stop/pagehide cleanup and server-owned hard-disconnect cleanup have passed real yaVDR acceptance;
- 65.A existing-Recording playback is closed for its bounded scope;
- 65.B Live-TV playback is closed after exact-head CI plus real yaVDR picture/sound, repeated zap and stability acceptance;
- 65.C first closed the completed-Recording startup/performance path through PR #206, with truthful `progressive-direct`, low-latency `progressive-fmp4` and HLS fallback semantics;
- the same authorized 65.C scope then continued through PR #208 with backend-scoped `auto` / `software` / `vaapi` output settings, calibrated selection, hard VAAPI capability checks, session-stable settings, browser diagnostics, fail-closed forced-VAAPI behavior and progressive-fMP4 backpressure hardening;
- the old separate `65.D - Compatibility escalation` planning block was thereby consumed by demonstrated compatibility/performance work inside 65.C and never started as an independent vertical;
- 65.D.1 Persistent Browser Playback Shell and 65.D.2 Recording Playback Controls and Seek are closed for their accepted scopes;
- normalized Recording audio/subtitle selection and browser-local Volume/Mute are accepted;
- PR #219 closed the continuous-fMP4 browser MSE forward-buffer/backpressure gap;
- PR #220/#221 closed compatibility timeline drag ownership and exact non-zero HLS Recording resume synchronization;
- completed-Recording arbitrary time-seek and stop/resume are implemented for the supported profiles, while user-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain deferred;
- active 65.D work is now the ADR-0056 semantic consolidation: normalized `MediaPlaybackContract`, canonical owner lifecycle publication, continuity/discontinuity and presentation-generation semantics, and classified playback failures;
- read-only media diagnostics follow semantic correctness; shared fMP4/MSE helper deduplication is technical debt rather than a Phase-65 gate.

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
- Phase 65.C Recording delivery performance and media output/transcode settings is closed for its accepted scope.
- The old unstarted 65.D Compatibility escalation label is absorbed into completed 65.C; 65.D Client playback abstraction is active.
- Phase 65.D.1 Persistent Browser Playback Shell and Phase 65.D.2 Recording Playback Controls and Seek are closed for their accepted scopes.
- normalized Recording track selection, browser-local Volume/Mute, continuous-fMP4 MSE forward-buffer control and compatibility timeline/exact-HLS-resume work are accepted for their bounded scopes.
- ADR-0056 playback semantic consolidation remains the active 65.D architecture gate.
- Truthful range/seek/growing-recording capability remains a Phase-65 invariant; completed-Recording seek/resume is implemented where accepted while unsupported growing/timeshift capability remains explicit.
- Phase 65 does not close until the remaining required acceptance gates in the Strict Roadmap are satisfied.
- Future phases 66+ may be reordered only before runtime starts and only through explicit repository planning/architecture reconciliation.
- Broad Timer UI completion is not inserted as a numbered phase between 64 and 65.
- Cross-cutting product/admin work does not silently advance the numbered runtime phase.
- Accepted ADR architecture does not by itself prove runtime completion.

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
