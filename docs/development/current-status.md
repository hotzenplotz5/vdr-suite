# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second source for active branch heads, PR tips, transient CI run numbers or exact live `main` SHAs.

Before continuing work, always read `CURRENT.md`, the Strict Roadmap, the applicable ADRs and live GitHub state.

## Platform position

Latest completed numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.

Current active numbered runtime phase: **Phase 66 - Media Home and Browse Experience**.

Next strict numbered runtime phase: **Phase 66 - Media Home and Browse Experience**.

Current active runtime slice: **Slice 66.1 - Home Shell and Responsive Information Architecture**.

Phase 65.A through 65.D are completed for their accepted bounded scopes. The final runtime-sensitive follow-up was ADR-0057 bounded completed-Recording network recovery. Durable evidence is in [Phase 65 Closeout](phase-65-closeout.md).

Accepted ADR-0058 and [Phase 66 Media Home and Browse Experience](phase-66-media-home-browse-experience.md) define the responsive Home / Browse architecture and bounded implementation sequence. Phase 66 is active only for Slice 66.1; Slice 66.2 and later Phase-66 semantics remain outside the current authorization. ADR-0054 remains Broadcast Companion architecture for Phase 67.

Historical completed context includes Phase 58 - Frontend and Live Parity, Phase 61 - Suite Metadata and Genre Platform, Phase 62 - Identity, RBAC and Accountability Foundation, Phase 63 - Backend Agent and Secure Multi-Site Runtime, and Phase 64 - Timer Intent and Multi-Backend Orchestration.

## Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract.

That foundation remains authoritative beneath completed Phase-64 orchestration, completed Phase-65 media execution and active Phase-66 product composition.

## Phase 64 completion

Phase 64 is complete and separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The completed engine provides durable Timer intent/assignment/binding identities, deterministic backend ownership, managed native Timer create/update/toggle/delete fulfillment, durable no-blind-retry semantics, authoritative readback/reconciliation and controlled reassignment/failover.

The final reassignment/failover block allows replacement only before native dispatch or after exact authoritative absence. It atomically supersedes the old exclusive owner, creates the replacement with a new assignment identity/epoch, persists durable handover evidence and rechecks current candidate authority. Stale or ambiguous evidence fails closed.

The exact completion candidate, CI and real yaVDR evidence are recorded in [Phase 64 Closeout](phase-64-closeout.md). Exact live `main` must be read from GitHub rather than copied into narrative status documents.

## Timer mutation safety position

The Phase-64 completion retains the protected-write rules established across Phases 62-64:

- actor/backend authorization and backend write mode remain authoritative;
- backend generation and provider instance/generation are fenced;
- assignment, binding, operation and expected-state fingerprints are checked where applicable;
- a possible native dispatch is durably represented before completion can be claimed;
- ambiguous delivery is `outcome_unknown` and reconciliation-only;
- CREATE/UPDATE/TOGGLE require authoritative PRESENT readback;
- DELETE requires complete-inventory authoritative ABSENCE readback;
- reassignment cannot use `dispatching`, `executed_unverified`, `outcome_unknown`, incomplete inventory or ambiguous/external drift as replacement authority;
- public SuiteBridge SVDRP help remains closed for private Timer write commands.

## Revised forward ordering

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 Media Home and Browse Experience [ACTIVE; SLICE 66.1]
  -> Phase 67 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 Legacy OSD Compatibility Bridge
  -> Phase 69 Public API and Client Compatibility Hardening
  -> Phase 70 Recommendation and Content Knowledge Graph
```

Completed history through Phase 65 is unchanged. ADR-0058 owns the Phase-66 architecture and sequence; the explicit runtime kickoff authorizes only Slice 66.1.

## Completed Phase 65 streaming architecture

Accepted ADR-0046 defines the server-side Streaming Gateway and MediaSession boundary. Accepted ADR-0053 defines the complementary client-playback/media-adaptation direction. Accepted ADR-0055 defines the media-transcode backend-selection and hardware-acceleration contract. Accepted ADR-0056 defines normalized playback presentation, timeline, continuity and failure semantics above the internal presentation/worker plan. Accepted ADR-0057 defines the bounded completed-Recording interruption-recovery contract.

The accepted Phase-65 media direction is provider-private and transformation-minimal:

```text
private source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> pass-through / remux / transcode as required
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The implementation sequence that actually shipped is Recording playback, Live TV, then a Phase-65.C delivery/performance block that first accelerated completed Recording playback and then continued into backend-scoped media-transcode output policy/settings. Phase 65.D completed the stable first-party playback semantics layered over those accepted MediaSession contracts. Remux and transcode remain evidence-driven compatibility escalation, not default architecture; exact non-zero HLS video resume is an accepted operation-specific case where stream-copy is not sync-safe and stronger adaptation is therefore required.

## Phase 65.A Recording playback completion

The existing-Recording playback vertical is accepted and closed for its bounded browser product scope.

Accepted capabilities include:

- authenticated/authorized MediaSession creation from stable Suite Recording identity;
- private local Recording source resolution without leaking provider-native paths to the browser;
- source probing and capability-based least-transformation selection;
- H.264 progressive copy paths;
- interlaced H.264 deinterlace/transcode handling;
- HEVC/Main10 UHD adaptation to browser-compatible H.264;
- AAC, AC3 and DTS audio handling with AAC stereo fallback where required;
- fMP4/HLS delivery through the Suite Gateway;
- startup/rebuffer forward buffering and stable four-second publication cadence;
- calibrated x264 workload policy;
- calibrated VAAPI UHD hardware transcoding with fail-closed UHD auto selection when no measured path reaches the 1.25x minimum;
- graceful `pagehide`, stop, ended and error cleanup;
- server-owned cleanup after an ungraceful client disappearance using the existing 300-second MediaAccessGrant idle boundary;
- deterministic worker/workspace and MediaSession/Route/Lease/Grant terminal cleanup;
- real browser picture + sound acceptance including mobile/VPN-sensitive cases.

The hard-disconnect acceptance proved that active playback is not reaped while access continues, and that an idle client disappearance ends the worker with terminal reason `media_access_idle_expired` and removes the workspace.

See [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md) for the durable evidence summary.

## Phase 65.B Live-TV playback completion

The Live-TV playback vertical is accepted and closed for its bounded browser product scope.

Accepted capabilities include:

- stable Suite Channel identity resolved to an explicitly owned private native live provider;
- authorization through MediaSession / MediaRoute / ProviderStreamLease / MediaAccessGrant rather than provider URLs;
- bounded SuiteBridge receiver/replay transport;
- direct browser-facing fragmented-MP4 delivery without the old HLS readiness barrier;
- one continuous FFmpeg socket consumer in the Live-TV hot path and no separate ffprobe socket consumer;
- browser-safe H.264/AAC adaptation with conditional interlace handling;
- explicit channel replacement and deterministic old-session/provider cleanup;
- repeated real yaVDR zap acceptance across Pro7, ZDF, RTL and NDR;
- a 15-minute stable Pro7 run;
- no VDR restart reproduced during the final accepted Live-TV stress run.

The previous VDR restart remains causally unproven because no earlier stack trace/coredump was captured, but it was not reproducible after the single-consumer fix under the previously problematic Live-TV workload.

See [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md) for the exact candidate, CI and real-system evidence.

## Phase 65.C Recording delivery performance and media output/transcode settings

Phase 65.C is the bounded media delivery/performance scope completed through PR #206 and the subsequently continued PR #208 work.

### Completed-Recording startup / progressive delivery

The first 65.C block removes the HLS startup pipeline from compatible completed Recordings while keeping least-transformation and source-truth boundaries:

```text
completed Recording
  -> descriptor/source truth
  -> capability-driven selection
  -> progressive-direct when truthful byte ranges are supported
  -> otherwise progressive-fMP4 for the compatible browser path
  -> HLS compatibility fallback when required
```

The continuous progressive-fMP4 stream deliberately does not advertise byte-range or immutable content-length semantics. Growing Recordings do not become immutable merely because a faster completed-Recording path exists.

### Media-transcode backend policy and output settings

The same authorized 65.C scope then continued into the backend settings/output-policy work governed by ADR-0055.

Accepted capabilities include:

- backend-scoped managed output modes `auto`, `software` and `vaapi`;
- Web/REST settings integration with backend-scoped authorization, CSRF and accountability;
- deterministic precedence between managed backend setting, deployment environment and built-in Auto;
- measured 1.25x real-time eligibility for Auto;
- quality-first measured x264 preset selection;
- hard execution-host VAAPI capability checks;
- no silent x264 fallback from forced VAAPI;
- session-stable encoder selection for active workers;
- calibrated diagnostics without exposing a writable DRM path or arbitrary FFmpeg arguments;
- progressive-fMP4 slow-reader/backpressure hardening;
- durable Live fail-closed terminal persistence for unsupported forced-VAAPI transforms.

Real yaVDR acceptance covered Auto UHD -> VAAPI, Auto interlaced -> x264/`veryfast` + `bwdif`, persisted forced-Software settings, forced VAAPI UHD, forced VAAPI Live fail-closed, forced-Software Live recovery, Auto Live restoration and active-session stability.

See [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md), [Phase 65 Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md), [ADR-0055](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md) and [Current State](../CURRENT.md).

## Retained seek/growing-recording truthfulness boundary

The obsolete 65.C seek heading is removed, but its important safety semantics are retained:

- advertise Range/seek only when the selected source/profile actually supports it;
- completed versus growing state remains server-owned source truth;
- continuous progressive-fMP4 is not presented as an immutable HTTP byte-range representation;
- native `progressive-direct` is available only when its byte-range/source-fingerprint contract is valid;
- Phase 65.D.2 provides accepted arbitrary completed-Recording time-seek and stop/resume for supported progressive-fMP4 and HLS restart-seek profiles;
- compatibility timeline interactions commit canonical absolute Recording positions rather than transport-local guesses;
- exact non-zero HLS video resume must provide sync-safe A/V random-access startup through the implemented adaptation path or fail closed;
- user-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain deferred until a coherent demonstrated gap authorizes them.

This means Phase 65 truthfully reports unsupported capability instead of fabricating seek merely because an old roadmap label named it.

## Phase 65.D completed direction

Phase 65.D is the completed client playback abstraction vertical:

```text
Suite MediaSession
  -> normalized MediaPlaybackContract
  -> persistent semantic playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The abstraction exposes stable playback semantics such as open, play/pause/stop, seek where supported, track selection, absolute position/state, continuity/discontinuity, classified failure and close. Browser remains the initial product validator; Android/Android TV, Windows, Kodi-style, Apple and television clients keep mature platform engines behind the Suite adapter rather than sharing a Suite-owned decoder core.

Phase 65.D.1 is accepted and closed for the Persistent Browser Playback Shell. It establishes one persistent browser playback owner across internal navigation and preserves native browser/Android Picture-in-Picture on the same HTML media element and MediaSession.

Phase 65.D.2 is accepted and closed for Recording Playback Controls and Seek. It provides Play/Pause/Stop, truthful completed-Recording position/duration, relative/timeline/direct absolute seek, progressive-fMP4 MediaSession worker repositioning, HLS restart-seek through fresh authorized sessions, stop/resume versus start-from-beginning choice and Android-friendly direct-time entry.

Subsequent accepted Phase-65.D work adds normalized audio/subtitle selection, browser-local Volume/Mute, continuous-fMP4 MSE forward-buffer control, compatibility timeline drag ownership and exact HLS resume synchronization. The latter proves that user preview position, transport-local time and canonical Recording position are distinct state domains, and that exact non-zero HLS video resume may require operation-specific transcode despite ordinary start-at-zero remaining copy/remux.

The mandatory semantic consolidation governed by [ADR-0056](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md) and [Phase 65.D Playback Semantics Consolidation](phase-65d-playback-semantics-consolidation.md) is complete:

- one normalized provider-free `MediaPlaybackContract` rather than profile-name reconstruction;
- canonical playback-owner lifecycle snapshots/events;
- explicit continuity/discontinuity and presentation-generation semantics without conflating `mediaSessionId`, `routeEpoch` and worker generation;
- classified playback failures while preserving detailed `reasonCode` evidence and the no-silent-recovery rule.

Read-only media diagnostics remain optional observational follow-up. Shared fMP4/MSE helper deduplication is technical debt, not a Phase-65 completion gate.

Client capability negotiation, least-transformation selection and ADR-0055 transcode policy remain independent of browser/device brand or user-agent strings.

See [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md), [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md), [Frontend Playback Integration Contract](frontend-playback-integration-contract.md) and [Phase 65.D Playback Semantics Consolidation](phase-65d-playback-semantics-consolidation.md).

## Phase 66 active direction

Phase 66 is the active product-composition phase under accepted ADR-0058. The current authorization is limited to **Slice 66.1 — Home Shell and Responsive Information Architecture**.

Slice 66.1 extends the existing installed browser composition root instead of creating a second Home application. The existing `overview` module state and `app.js` navigation owner remain canonical; Home adds semantic Hero / primary-rail / additional-section layout zones and responsive recomposition while reusing real backend snapshot data for the initial rail.

No Live-TV hero-carousel semantics, deferred preview, Home-triggered MediaSession creation, Continue Watching, history or recommendation authority is introduced by Slice 66.1. Existing Phase-65 playback ownership remains canonical across Home navigation.

## Broadcast Companion direction

Teletext and HbbTV are explicit planned television-domain capabilities rather than accidental Legacy-OSD backlog.

- Teletext is modeled as service/page/subpage data, not primarily as an OSD screenshot.
- HbbTV is modeled as broadcast application discovery plus an authorized isolated application session/runtime, not as raw URL/JavaScript/key plugin control.
- Phase-65 MediaSession rules remain authoritative when Suite-owned media is involved.
- Legacy OSD follows later for genuinely opaque native/plugin workflows.

## Broad Timer UI ordering

A broad polished Timer UI is not required to close the Phase-64 engine and is not inserted as a numbered phase.

It is a cross-cutting product milestone gated on:

```text
Phase 62 identity/RBAC [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

The product UI must remain intent-first and preserve assignment, readback, reconciliation, failover and unknown-outcome semantics.

The broad Timer UI remains an independent cross-cutting milestone and does not silently advance active Phase-66 runtime work.

## Product acceptance

Component tests, CI, architecture guards and real-system safety checks remain mandatory. User-visible milestones additionally use [Golden User Journeys](../planning/golden-user-journeys.md).

The key vertical journeys cover:

- Live TV playback;
- Recording playback;
- Media Home browsing and responsive navigation;
- record-one-programme orchestration;
- multi-backend scheduling without provider knowledge;
- fail-closed recovery;
- Teletext browsing;
- HbbTV application launch;
- one explicit Legacy OSD compatibility workflow;
- broad Timer UI operation as a cross-cutting milestone.

## Development rules

- Root-level `AGENTS.md` is binding, including its top-level non-stop execution mandate.
- `CURRENT.md` is the sole repository copy of volatile operational phase status.
- Verify live `main` and the exact PR head before writes or status claims. During approved iterative work, validation is surface-scoped and unrelated CI does not block progress; the complete repository CI graph is evaluated only at the documented Ready/merge/phase-closeout full-stabilization boundary.
- Keep review/merge/retarget/close state changes behind explicit user approval; existing explicit authorization remains valid and must not trigger a second artificial stop.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires them.
- Provider availability never creates execution authority and active work never silently switches provider.
- Require real-system acceptance when an installed/runtime, native, media or broadcast-behaviour boundary changes.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- Accepted ADR work defines architecture but does not by itself prove or complete runtime implementation.
- Phase 65 is completed; Phase 66 is active only for the explicitly authorized Slice 66.1 scope.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded edit safely. Read the complete file content required for the change, write a coherent commit on the intended branch and inspect the resulting diff before treating the update as correct.

Use local edits first only when the change requires:

- local build/test execution that cannot be represented by the connector;
- multi-file transformations that are materially safer in a checked-out worktree;
- binary/generated-file handling unavailable through the connector; or
- a workaround because the GitHub connector blocks a file operation.

GitHub-first does not weaken review safety: keep updates fast-forward-only, do not replace a complete file from a truncated fetch, and do not mark Draft PRs Ready or merge them without explicit approval.

## Current authorization boundary

Phase 65.A through 65.D are closed for their accepted bounded scopes, including normalized Recording tracks, browser-local Volume/Mute, continuous-fMP4 browser MSE forward-buffer control, compatibility timeline ownership, exact non-zero HLS Recording resume synchronization, ADR-0056 playback semantics and bounded completed-Recording interruption recovery.

**Phase 66 is active only for Slice 66.1 — Home Shell and Responsive Information Architecture.** The current slice may change the real browser composition, responsive navigation/layout foundation and structural loading/empty/error presentation while preserving the existing `app.js` module owner and canonical Phase-65 playback lifecycle.

Slice 66.2 Live-TV Hero Carousel, Slice 66.3 Deferred Live Preview, Continue Watching, Recently Watched/history, recommendation intelligence and every later Phase-66 semantic block remain outside the current authorization. Phase 67 Broadcast Companion remains not started.

Completed-Recording arbitrary seek and stop/resume are accepted for the supported profiles. Growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain capability-driven and must be represented truthfully rather than fabricated.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65 Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Browser-local Volume/Mute Closeout](phase-65d-browser-volume-mute-closeout.md)
- [Phase 65.D Playback Semantics Consolidation](phase-65d-playback-semantics-consolidation.md)
- [Phase 66 Media Home and Browse Experience](phase-66-media-home-browse-experience.md)
- [Frontend Playback Integration Contract](frontend-playback-integration-contract.md)
- [ADR-0055 Media Transcode Backend Selection](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR-0058 Media Home, Responsive Browse and Preview Experience](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Agent Workflow Rules](../../AGENTS.md)