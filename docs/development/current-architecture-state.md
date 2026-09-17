# VDR-Suite Current Architecture State

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65 Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Playback Semantics Consolidation](phase-65d-playback-semantics-consolidation.md)
- [Frontend Playback Integration Contract](frontend-playback-integration-contract.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This document summarizes **implemented architecture by durable capability boundary**. It intentionally does not copy active PR tips or transient CI checkpoints from [Current State](../CURRENT.md).

Historical exact acceptance evidence stays in phase/slice closeouts. Volatile implementation progress belongs in `docs/CURRENT.md`.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugins

VDR-Suite Control Plane
  -> Suite identity and backend scope
  -> authentication, authorization and policy
  -> domain services and persistent read models
  -> operations, orchestration and reconciliation
  -> client-facing Suite contracts

Backend Agent
  -> enrolled technical identity and generation/instance fencing
  -> bounded observations and durable command/result transport
  -> explicit local provider ownership/selection
  -> site-local execution and cleanup

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge and future adapters
```

Frontend modules do not call private backend protocols directly and do not own authentication, authorization, provider selection or retry safety decisions.

## Persistence and read architecture

Implemented foundations include:

- daemon-owned SQLite persistence and migrations;
- repository-owned SQL with service/controller separation;
- backend registry and backend-scoped read models;
- snapshots, caches and change-feed foundations;
- channels and EPG read models;
- Recordings 2, Recording metadata/people/artwork/Genre data;
- query-oriented global search and related persistent metadata reads;
- backend-neutral RemoteAction and LiveOverlay contracts.

Provider data remains evidence behind Suite persistence/contracts rather than becoming public authority.

## Identity, authorization and accountability

The implemented security boundary includes:

- persistent actor, device, credential and session identity;
- browser-session lifecycle and CSRF enforcement;
- exact backend-scoped permission grants and fixed-role semantics;
- strict server-side protected-mutation classification;
- append-only pre-dispatch accountability and protected operation outcomes;
- secret handling that keeps raw session/credential material out of durable accountability data.

Legacy Basic compatibility remains transitional where retained by deployment compatibility policy; its historical Phase-62 decision does not define the target identity architecture.

## Backend Agent and secure multi-site foundation

The implemented Agent architecture includes bounded support for:

- Agent enrollment and technical identity;
- credential lifecycle and generation/instance fencing;
- heartbeat/lease and backend health semantics;
- read-only observation/snapshot ingestion with sequence/resync rules;
- durable command delivery, receipts/results and reconnect handling;
- fenced native operation execution;
- explicit local provider ownership and selection;
- protected-write safety contracts that prevent silent provider fallback and stale-generation completion.

This Phase-63 capability is a reusable platform foundation for completed Timer orchestration, completed media execution foundations and later bounded native operations.

## Protected-write safety model

The implemented direction for protected native writes is:

```text
operation / intent
  -> authorization and backend/provider eligibility
  -> idempotency scope
  -> expected revision where applicable
  -> resource-scoped concurrency/ownership fencing
  -> backend generation / provider ownership fence
  -> durable dispatch boundary
  -> one-shot or otherwise bounded native execution
  -> authoritative readback
  -> verified success / verified no-effect / outcome unknown
  -> reconciliation before any unsafe retry
```

A transport timeout after possible dispatch is not treated as proof of failure. Provider availability does not create authority and active execution does not silently switch providers.

## Timer architecture — implemented and accepted

Phase 64 is complete and implements the accepted separation:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

`TimerIntent` is backend-neutral Suite-owned intent. `TimerAssignment` records deterministic backend ownership/role with repository-issued revision/epoch and assignment-set fencing. `NativeTimerBinding` represents the backend-native VDR Timer relationship and canonical observation/readback state.

The accepted runtime boundary includes:

- deterministic primary/replica scheduling;
- managed native create/update/toggle/delete fulfillment;
- durable mutation-operation preparation/dispatch/completion;
- stable Suite correlation and expected-state fingerprint fencing;
- durable Agent `starting` before possible side effect;
- no blind retry after possible dispatch;
- authoritative PRESENT and complete-inventory ABSENCE readback;
- controlled replacement before dispatch or after exact verified absence;
- atomic exclusive-owner handover with durable reassignment evidence;
- reconciliation-only handling for uncertain native outcomes.

Exact final acceptance evidence belongs in [Phase 64 Closeout](phase-64-closeout.md).

A broad polished Timer UI remains outside the completed Phase-64 engine boundary and is separately gated on account/backend access management.

## Media architecture state — completed Phase 65 foundation

The accepted media target is defined by ADR-0046 and ADR-0053, with ADR-0055 defining media-transcode backend selection/hardware-acceleration policy and ADR-0056 defining normalized playback presentation, timeline, continuity and failure semantics:

```text
Client
  -> authenticated Suite playback request
  -> MediaSession
  -> Streaming Gateway
  -> MediaRoute
  -> Backend Agent / explicitly owned provider where required
  -> ProviderStreamLease
  -> private media provider / VDR source

source + client capabilities + Suite policy
  -> internal MediaPresentationProfile
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

Phase 65 is completed. Its accepted bounded verticals/slices include:

- **65.A Existing-Recording playback** — authenticated MediaSession/Gateway playback, least-transformation adaptation, real picture/sound and deterministic lifecycle cleanup;
- **65.B Live-TV playback** — bounded SuiteBridge live provider/replay, one continuous FFmpeg consumer, real picture/sound, repeated zap and stability acceptance;
- **65.C Recording delivery performance and media output/transcode settings** — completed-Recording `progressive-direct`/`progressive-fMP4` startup optimization followed by backend-scoped `auto`/`software`/`vaapi` output policy/settings, calibrated selection, hard VAAPI capability checks, session-stable policy, fail-closed forced-VAAPI behavior and stream-backpressure hardening;
- **65.D.1 Persistent Browser Playback Shell** — one persistent browser playback owner and the same HTML media element across internal first-party presentation changes, including native browser/Android Picture-in-Picture without a second MediaSession;
- **65.D.2 Recording Playback Controls and Seek** — Play/Pause/Stop, truthful completed-Recording position/duration, relative/timeline/direct absolute seek, progressive-fMP4 MediaSession worker repositioning, HLS restart-seek through fresh authorized sessions, stop/resume semantics and Android direct-time entry;
- **normalized Recording track selection** — public audio/subtitle identities, progressive/HLS owner integration and supported browser SRT/WebVTT delivery without PID/provider leakage;
- **browser-local Volume/Mute** — client-local `HTMLMediaElement` state across the accepted persistent/replaceable owner topology without server/VDR volume mutation;
- **continuous-fMP4 MSE forward-buffer control** — bounded browser read/append-ahead behavior without changing MediaSession/provider ownership;
- **compatibility timeline and exact HLS resume follow-up** — user-owned timeline drag target survives active playback updates, and exact non-zero HLS video resume uses a sync-safe implemented transcode path or fails closed while ordinary start-at-zero retains least-transformation copy/remux;
- **ADR-0056 semantic consolidation** — normalized provider-free playback contract, canonical owner lifecycle state, explicit continuity/discontinuity and classified failure semantics.

Current accepted delivery/client rules remain:

- pass-through first, then remux/repackage only where required, then transcode only where materially required;
- operation-specific correctness may require stronger adaptation, as proven by exact non-zero HLS video resume where arbitrary stream-copy startup was not sync-safe;
- provider-native URLs, paths, credentials and socket details remain private;
- MediaSession, route, grant and provider lease remain Suite-owned authorization/lifecycle boundaries;
- active sessions do not silently retarget provider or encoder policy;
- Range/seek/growing capability is truthful; completed-Recording seek is exposed only through accepted profile/semantic contracts and unsupported growing/timeshift capability remains explicit;
- continuous progressive fMP4 does not invent HTTP byte-range semantics merely because the owned MediaSession can reposition playback by time;
- completed-only immutable fast paths fail closed when a Recording is growing or its source fingerprint changes;
- Web output settings are backend-scoped policy controls for new sessions, not arbitrary FFmpeg/device configuration;
- user-owned seek preview, transport-local presentation time and canonical absolute Recording position are separate state domains;
- MediaSession identity, route epoch and playback presentation generation are separate concepts and must not be conflated;
- the client semantic layer remains above mature platform playback engines rather than becoming a Suite-owned universal decoder/rendering core.

The old roadmap label `65.C - Recording seek and growing-recording semantics` is superseded. Its truthfulness invariant remains. Phase 65.D.2 and follow-up fixes provide accepted arbitrary completed-Recording time-seek and stop/resume for the supported progressive-fMP4 and HLS restart-seek profiles. User-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain deferred until a demonstrated product gap justifies a coherent implementation.

Phase 65.D Client playback abstraction is completed for its accepted scope. The ADR-0056 consolidation requirements are implemented and retained as architecture contracts:

- one normalized provider-free `MediaPlaybackContract` above internal `MediaPresentationProfile` execution detail;
- canonical persistent-owner lifecycle snapshot/subscription for session-bound extensions;
- explicit continuity/discontinuity and presentation-generation semantics;
- classified playback failure behavior that preserves detailed reason codes without silent provider/profile/session recovery.

Read-only media diagnostics remain observational only. Shared fMP4/MSE primitive extraction is technical debt, not a numbered-phase architecture gate.

Streamdev remains a private possible provider rather than the public API/security boundary.

## Post-closeout Live-TV lifecycle semantics

The persistent playback shell must not infer an explicit user/session stop from every media-element terminal-looking browser event. Continuous Live/MSE playback is open-ended, and a transient transport EOF may surface as `HTMLMediaElement.ended` while the canonical MediaSession is still valid.

The accepted post-closeout rule is therefore:

```text
HTMLMediaElement ended during owned Live playback
  -> not an implicit MediaSession STOP
  -> keep canonical owner/session alive

explicit Live stop / backend change / browser-session loss / replacement handoff
  -> real lifecycle boundary

fatal player error
  -> terminal playback failure
```

This correction preserves the Phase-65 server/client ownership model rather than adding retry or replacement magic. Exact accepted evidence is recorded in [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md).

## Phase 66 and post-phase product architecture

Phase 66 - Media Home and Browse Experience is completed. Later non-numbered work hardens that accepted product model without creating a second Home or Recording architecture.

The retained Home/Recording presentation ownership is:

```text
canonical Suite read models
  -> first-party Home browse projections
  -> canonical Recording identity
  -> Recordings 2 detail owner
  -> existing metadata / marks / playback owners
```

Current accepted post-phase product behavior includes:

- responsive Home shell, Now/Next, newly recorded, Movies/Genres and Series -> seasons -> episodes;
- scoped/progressive Recording metadata completion instead of global browser metadata fan-out;
- canonical TVScraper/native portrait artwork priority and manual Series hierarchy/artwork overrides;
- native Recording marks/cutting with VDR as canonical marks/cutter authority;
- a cinematic Recordings 2 full-page Hero detail with compact facts, cast/person presentation and related-Genre rail;
- Recording playback prewarm through the existing canonical playback owner, without autoplay, duplicate MediaSessions or pre-playback Continue Watching/history truth;
- canonical `kind=poster` selection for related Recording cards, keeping locked manual poster authority and preferring native portrait metadata before weaker preferred-artwork fallback.

The Recording-detail presentation is a presentation layer over the existing Recording domain; it is not a new content owner. Exact evidence belongs in [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md).

## Public API and client boundary

Current first-party clients use Suite-owned REST/client-wrapper semantics rather than private provider contracts. Stable independent third-party `/api/v1` compatibility remains governed by its separate roadmap/ADR boundary and must not be inferred from internal transition endpoints.

`MediaPlaybackContract` is first-party Phase-65 semantic architecture. ADR-0056 does not by itself promote the current internal media endpoints to a stable third-party `/api/v1` contract.

## Acceptance model

Implementation claims require the appropriate combination of:

- domain/repository/service tests;
- architecture/static guards;
- aggregate CI;
- packaging/install validation;
- real yaVDR acceptance for native runtime changes;
- real client/media acceptance where product behaviour requires it;
- Golden User Journey proof for vertical product outcomes.

Exact historical acceptance heads/hashes belong in the closeout that accepted them.

For playback semantic work, the production ownership topology is itself part of acceptance: user-style action -> canonical owner/session transition -> expected Suite request/state chain must be proven rather than inferred from isolated wrappers.

## Related documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](current-status.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65 Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Playback Semantics Consolidation](phase-65d-playback-semantics-consolidation.md)
- [Frontend Playback Integration Contract](frontend-playback-integration-contract.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Completed Phases](completed-phases.md)

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
