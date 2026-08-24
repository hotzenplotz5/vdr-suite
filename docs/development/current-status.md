# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second source for active branch heads, PR tips, transient CI run numbers or exact live `main` SHAs.

Before continuing work, always read `CURRENT.md`, the Strict Roadmap, the applicable ADRs and live GitHub state.

## Platform position

Latest completed numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.

Current active numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.

Completed Phase-65 verticals: **65.A - Existing-Recording playback**, **65.B - Live-TV playback**, and **65.C - Recording delivery performance and media output/transcode settings**.

Current Phase-65 product vertical: **65.D - Client playback abstraction**.

Completed Phase-65.D slices: **65.D.1 - Persistent Browser Playback Shell** and **65.D.2 - Recording Playback Controls and Seek**.

The earlier planning label `65.C - Recording seek and growing-recording semantics` is superseded. Truthful Range/seek/growing capability remains a Phase-65 requirement. Phase 65.D.2 now implements arbitrary completed-Recording time-seek and stop/resume for the accepted progressive-fMP4 and HLS restart-seek profiles; growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain explicit deferred capability work.

Historical completed context remains relevant, including Phase 58 - Frontend and Live Parity, Phase 61 - Suite Metadata and Genre Platform, Phase 62 - Identity, RBAC and Accountability Foundation, and Phase 63 - Backend Agent and Secure Multi-Site Runtime.

## Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract.

That foundation remains authoritative beneath completed Phase-64 orchestration and active Phase-65 media execution.

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

The strict numbered order is:

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [ACTIVE]
  -> Phase 66 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 Legacy OSD Compatibility Bridge
  -> Phase 68 Public API and Client Compatibility Hardening
  -> Phase 69 Recommendation and Content Knowledge Graph
```

Completed history through Phase 64 is unchanged. Accepted ADR-0054 defines the Teletext/HbbTV architecture for Phase 66, but Phase 66 remains runtime-blocked until Phase 65 is complete and a separate Phase-66 kickoff is explicit.

## Active Phase 65 streaming architecture

Accepted ADR-0046 defines the server-side Streaming Gateway and MediaSession boundary. Accepted ADR-0053 defines the complementary client-playback/media-adaptation direction. Accepted ADR-0055 defines the media-transcode backend-selection and hardware-acceleration contract.

The active media direction is provider-private and transformation-minimal:

```text
private source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> pass-through / remux / transcode as required
  -> Streaming Gateway / MediaSession
  -> client adapter
  -> platform playback engine
```

The implementation sequence that actually shipped is Recording playback, Live TV, then a Phase-65.C delivery/performance block that first accelerated completed Recording playback and then continued into backend-scoped media-transcode output policy/settings. Phase 65.D is now active and layers stable first-party playback semantics over those accepted MediaSession contracts. Remux and transcode remain evidence-driven compatibility escalation, not default architecture.

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
- user-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain deferred until a coherent demonstrated gap authorizes them.

This means Phase 65 truthfully reports unsupported capability instead of fabricating seek merely because an old roadmap label named it.

## Phase 65.D active direction

Phase 65.D is the active client playback abstraction vertical:

```text
Suite MediaSession
  -> semantic playback adapter
  -> platform playback engine
```

The abstraction exposes stable playback semantics such as open, play/pause/stop, seek where supported, track selection, position/state, discontinuity/failure and close. Browser remains the initial product validator; Android/Android TV, Windows, Kodi-style, Apple and television clients keep mature platform engines behind the Suite adapter rather than sharing a Suite-owned decoder core.

Phase 65.D.1 is accepted and closed for the Persistent Browser Playback Shell. It establishes one persistent browser playback owner across internal navigation and preserves native browser/Android Picture-in-Picture on the same HTML media element and MediaSession.

Phase 65.D.2 is accepted and closed for Recording Playback Controls and Seek. It provides Play/Pause/Stop, truthful completed-Recording position/duration, relative/timeline/direct absolute seek, progressive-fMP4 MediaSession worker repositioning, HLS restart-seek through fresh authorized sessions, stop/resume versus start-from-beginning choice and Android-friendly direct-time entry.

Remaining Phase-65.D semantic work includes normalized audio/subtitle selection, discontinuity handling and classified playback failure behavior. Client capability negotiation, least-transformation selection and ADR-0055 transcode policy remain independent of browser/device brand or user-agent strings.

See [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md) and [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md).

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

Phase 65 intentionally proceeds before the broad Timer UI is complete.

## Product acceptance

Component tests, CI, architecture guards and real-system safety checks remain mandatory. User-visible milestones additionally use [Golden User Journeys](../planning/golden-user-journeys.md).

The key vertical journeys cover:

- Live TV playback;
- Recording playback;
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
- Proposed ADR work does not authorize runtime implementation.
- Phase 65 is active; Phase 66 runtime remains blocked until Phase 65 closes and Phase 66 is explicitly started.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded edit safely. Read the complete file content required for the change, write a coherent commit on the intended branch and inspect the resulting diff before treating the update as correct.

Use local edits first only when the change requires:

- local build/test execution that cannot be represented by the connector;
- multi-file transformations that are materially safer in a checked-out worktree;
- binary/generated-file handling unavailable through the connector; or
- a workaround because the GitHub connector blocks a file operation.

GitHub-first does not weaken review safety: keep updates fast-forward-only, do not replace a complete file from a truncated fetch, and do not mark Draft PRs Ready or merge them without explicit approval.

## Current authorization boundary

Phase 65.A, 65.B and 65.C are closed for their accepted bounded scopes. **Phase 65.D is active; Phase 65.D.1 and Phase 65.D.2 are accepted and closed.** Remaining Phase-65.D work stays inside the accepted client playback abstraction and must preserve all existing MediaSession, provider-privacy, least-transformation, output-policy and cleanup boundaries.

Completed-Recording arbitrary seek and stop/resume are accepted for the supported D.2 profiles. Growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain capability-driven and must be represented truthfully rather than fabricated.

Phase 66 remains out of scope until Phase 65 closes and Phase 66 receives an explicit kickoff.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65 Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [ADR-0055 Media Transcode Backend Selection](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Agent Workflow Rules](../../AGENTS.md)
