# VDR-Suite Current State

## Operational status authority

**This file is the sole repository authority for volatile operational phase status.**

`README.md`, `NEW-CHAT-HANDOFF.md`, `development/current-status.md`, the Strict Roadmap and the Phase Map may describe stable architecture, phase order and workflow rules, but they must not become competing copies of active PR tips or transient CI state.

Before any implementation, review-state change, installation or status claim, re-read current GitHub state. Recorded completion evidence below is a durable checkpoint, not a substitute for a live read of `main`.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Architecture Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Current Project Status](development/current-status.md)
- [Phase 64 Closeout](development/phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](development/phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](development/phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](development/phase-65-recording-startup-progressive-direct.md)
- [Phase 65.D Media Transcode Performance Policy](development/phase-65-media-transcode-performance-policy.md)
- [ADR-0055 Media Transcode Backend Selection](adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [ADR-0044 Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Playback/Adaptation](adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [Architecture Decision Records](adr/index.md)
- [Agent Workflow Rules](../AGENTS.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main

Latest completed numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Next strict numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Completed Phase-65 product verticals:
65.A - Existing-Recording playback
65.B - Live-TV playback
65.C - Recording startup / progressive-direct
65.D - Media-transcode backend policy and output settings

Next Phase-65 product vertical:
65.E - Client playback abstraction
```

Phase 65 is active. The earlier planning label `65.C - Recording seek and growing-recording semantics` is superseded by the implementation history. PR #206 explicitly implemented and closed the bounded **Phase-65.C Recording startup/performance** vertical, and PR #208 then completed the media-transcode backend policy/settings work that corresponds to **65.D**.

Truthful range/seek/growing-recording capability remains a binding Phase-65 invariant. The implementation must not advertise Range, time-seek or immutable-source behavior where the selected source/profile cannot support it. Full arbitrary VOD time-seek/VDR-index mapping and user-visible growing-Recording seek remain deferred capability work; they are not the current 65.C product label and are not to be invented merely to satisfy an obsolete roadmap heading.

Phase 66 remains blocked until Phase 65 is completely closed and Phase 66 is explicitly started.

Phase 64 closed through PR #195. The exact accepted implementation candidate was `bdd70d527d640dc115a7c141e505140ce8cdba9a`; PR #195 merged that candidate into `main` as `72e298a76f7879ea7fc58f6a502e32eca7399f5a`.

The current `main` commit itself is intentionally **not** copied into this file. Query live GitHub state whenever an exact repository checkpoint matters.

## Historical foundation markers

`Phase 63 - Backend Agent and Secure Multi-Site Runtime` remains the completed execution/provider foundation beneath Phase 64. This marker is retained for historical contract guards and traceability only; it does not make Phase 63 current or active again.

## Phase 64 completion evidence

The final exact-head hosted CI and real-system gate both passed.

```text
accepted_candidate=bdd70d527d640dc115a7c141e505140ce8cdba9a
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7689
source_ci_run_id=32023780598
source_ci_result=PASS

PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=PASS
PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=PASS
ADVERTISEMENT=timer-commands-activated
REASSIGNMENT=atomic-fail-closed
OUTCOME_UNKNOWN=reconciliation-only
PUBLIC_SVDRP_TIMER_WRITES=closed

merge_pr=195
merge_commit=72e298a76f7879ea7fc58f6a502e32eca7399f5a
```

The real yaVDR acceptance ran on VDR 2.7.9 and the exact candidate above. It preserved the public SuiteBridge SVDRP help boundary while proving the complete managed Timer fulfillment and controlled reassignment/failover acceptance contract.

See [Phase 64 Closeout](development/phase-64-closeout.md) for the durable completion record.

## What Phase 64 established

The completed Timer engine coherently implements the accepted ADR-0044 separation:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The accepted scope includes:

- durable TimerIntent, TimerAssignment and NativeTimerBinding identity/revision semantics;
- deterministic primary/replica assignment and assignment-set concurrency fencing;
- managed native Timer create/update/toggle/delete fulfillment;
- explicit backend generation, provider, assignment, binding and expected-fingerprint fences;
- durable `starting`, no-blind-retry behavior and `outcome_unknown` reconciliation;
- authoritative PRESENT readback and complete-inventory ABSENCE proof;
- managed binding persistence and operation-aware verification;
- controlled reassignment/failover only before dispatch or after exact verified absence;
- atomic old-owner supersession plus replacement/evidence persistence;
- fail-closed handling of stale intent, assignment, set, generation, provider, binding and operation evidence;
- exact replay without duplicate replacement ownership;
- real-system acceptance of the shipped Timer command path.

Public SuiteBridge SVDRP help remains closed for `NTCREATE`, `NTMOD` and `NTDELETE`; private transport commands are implementation details, not a public mutation API.

## Revised phase ordering

The strict numbered order is now:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [ACTIVE]
  -> Phase 66 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 - Legacy OSD Compatibility Bridge
  -> Phase 68 - Public API and Client Compatibility Hardening
  -> Phase 69 - Recommendation and Content Knowledge Graph
```

Future phases 66+ are not runtime-authorized merely because they are named here. The strict details and gates live in the [Roadmap](planning/roadmap.md).

The Phase-66 Broadcast Companion architecture is defined by **accepted ADR-0054**. Teletext/HbbTV runtime remains blocked until Phase 66 is explicitly started after Phase 65.

## Timer Product UI decision

The broad polished Timer UI was intentionally not a Phase-64 completion gate and is not inserted as a numbered runtime phase.

It remains a cross-cutting product milestone with prerequisites:

```text
Phase 62 identity/RBAC foundation [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

The UI must remain intent-first and show assignment, fulfillment, reconciliation and failover state without collapsing uncertain native outcomes into fake success/failure.

Phase 65 may proceed before the broad Timer UI is completed.

## Active Phase 65 architecture

ADR-0046 and ADR-0053 jointly define the accepted server/client media direction. ADR-0055 adds the accepted media-transcode backend-selection and hardware-acceleration contract.

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> least-transformation media adaptation
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Transformation preference is `pass-through -> remux/repackage -> transcode`. Streamdev may be an explicitly owned private provider, but it is not the public playback API or an implicit fallback chain.

Browser playback is the first real-system validator, not the architecture authority; the same media/session/source contracts must remain usable by Android/Android TV, Apple platforms, Windows, Kodi-style clients and television runtimes.

## Phase 65.A Recording playback completion evidence

The existing-Recording playback vertical is accepted and closed for its bounded scope.

The accepted implementation includes:

- stable Suite Recording identity to private local provider source resolution;
- source probing and client-capability negotiation;
- least-transformation HLS/fMP4 presentation selection;
- H.264 copy, interlaced deinterlace/transcode and HEVC/Main10 UHD adaptation;
- AAC/AC3/DTS handling with AAC stereo fallback where required;
- startup/rebuffer forward buffering and stable HLS publication cadence;
- calibrated x264 policy plus calibrated VAAPI UHD selection with fail-closed unsuitable paths;
- graceful pagehide/stop/ended/error cleanup;
- server-owned idle cleanup after an ungraceful client disappearance;
- deterministic MediaSession/Route/ProviderStreamLease/MediaAccessGrant/workspace cleanup;
- real browser picture + sound acceptance including mobile/VPN-sensitive playback.

The final lifecycle implementation candidate was:

```text
accepted_65a_lifecycle_candidate=485c990c9f5692f00aa0e2e087967b236676c154
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7881
source_ci_run_id=32176309565
source_ci_result=PASS

ACTIVE_PLAYBACK_NOT_REAPED=PASS
HARD_DISCONNECT_IDLE_EXPIRY=PASS
FFMPEG_CLEANUP=PASS
WORKSPACE_CLEANUP=PASS
SESSION_TERMINAL_REASON=media_access_idle_expired
ROUTE_LEASE_GRANT_CLEANUP=PASS
```

See [Phase 65 Recording Playback Closeout](development/phase-65-recording-playback-closeout-readiness.md) for the durable evidence and compatibility history.

## Phase 65.B Live-TV playback completion evidence

The Live-TV playback vertical is accepted and closed for its bounded browser scope.

The accepted runtime candidate is:

```text
accepted_65b_runtime_candidate=7da9a3defc87b9442f1f75f90fb67ac514fd10cd
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7966
source_ci_run_id=32303041048
source_ci_result=PASS

LIVE_TV_PICTURE_SOUND=PASS
LIVE_TV_REPEATED_ZAPPING=PASS
ZAP_SEQUENCE=Pro7->ZDF->RTL->Pro7->NDR->Pro7
PRO7_15_MINUTE_STABILITY=PASS
LIVE_HOT_PATH_FFPROBE=absent
VDR_RESTART_DURING_FINAL_ACCEPTANCE=none_observed
```

The final hot path keeps one continuous FFmpeg consumer on the conditioned SuiteBridge replay instead of probing through one socket consumer and reconnecting a second consumer for playback. Browser compatibility remains a selected profile rather than a universal native-client contract.

The previously observed VDR restart under Live-TV stress was not reproduced on the accepted candidate. The single-consumer change is a plausible common fix for both zap/start instability and the earlier restart, but causality is not claimed as proven because the earlier crash had no captured stack trace or coredump.

See [Phase 65 Live-TV Playback Closeout](development/phase-65-live-tv-closeout.md) for durable implementation, CI and real-system evidence.

## Phase 65.C Recording startup / progressive-direct completion evidence

Phase 65.C is the accepted completed-Recording startup/performance vertical, not a seek/growing-recording implementation phase.

PR #206 introduced the fast completed-Recording delivery path while preserving ADR-0053 least-transformation semantics:

- `progressive-direct` remains available only when native MPEG-TS, selected codecs and truthful byte ranges are supported;
- `progressive-fmp4` provides the normal low-latency browser path for compatible completed Recordings without the old HLS startup gate;
- HLS remains a compatibility fallback;
- continuous fMP4 does not advertise fake `Accept-Ranges`, `Content-Range`, immutable `Content-Length` or browser time-seek semantics;
- completed/growing source truth remains server-owned and completed-only fast paths fail closed when their immutable-source guarantee is not valid;
- provider-native paths remain private and MediaSession/Gateway authorization and cleanup remain authoritative.

Accepted evidence:

```text
accepted_65c_candidate=51de13337edd0a072308a9df1bad6e245a764ac2
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7972
source_ci_run_id=32350815560
source_ci_result=PASS
merge_pr=206
merge_commit=0513edf6166e096aa60cf313b74a43073cacd786

COMPLETED_RECORDING_STARTUP=PASS
PICTURE_SOUND=PASS
PROGRESSIVE_FAST_PATH=accepted
FALSE_RANGE_SEEK_ADVERTISEMENT=closed
PHASE_66=not_started
```

See [Phase 65.C Recording Startup / Progressive Direct](development/phase-65-recording-startup-progressive-direct.md).

## Phase 65.D Media-transcode backend policy/settings completion evidence

Phase 65.D is the accepted compatibility/performance escalation for server-side video transformation and its backend-scoped output settings.

PR #208 and ADR-0055 established:

- backend-scoped managed output modes `auto`, `software` and `vaapi`;
- a Web/REST settings surface with backend scope, authorization, CSRF and accountability boundaries;
- calibrated Auto selection with a 1.25x real-time threshold;
- quality-first x264 preset selection from measured workload evidence;
- hard VAAPI execution-host capability checking and exact-transform eligibility;
- no silent software fallback when VAAPI is forced;
- session-stable encoder selection: settings changes affect new MediaSessions, not already running workers;
- calibrated diagnostics without exposing raw FFmpeg arguments or a writable DRM path;
- continuous progressive-fMP4 HTTP backpressure handling;
- durable terminal failure persistence for unsupported forced-VAAPI Live transforms.

Accepted evidence:

```text
accepted_65d_candidate=85478311b9af6c027a25980272a2acde551e5508
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7976
source_ci_run_id=32415860281
source_ci_result=PASS
merge_pr=208
merge_commit=8716bbe9f1ab8ebd4cdf597d620419ef0fcf098a

AUTO_UHD_VAAPI=PASS
AUTO_INTERLACED_X264=PASS
FORCED_SOFTWARE_RECORDING=PASS
FORCED_VAAPI_UHD=PASS
FORCED_VAAPI_LIVE_FAIL_CLOSED=PASS
FORCED_SOFTWARE_LIVE_RECOVERY=PASS
AUTO_LIVE_RESTORE=PASS
ACTIVE_SESSION_STABILITY=PASS
SETTINGS_PERSISTENCE_RESTART=PASS
HTTP_BACKPRESSURE_LONG_PLAYBACK=PASS
```

See [Phase 65 Media Transcode Performance Policy](development/phase-65-media-transcode-performance-policy.md) and [ADR-0055](adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md).

## Binding execution-governance decisions

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project phase status. Other documents link here instead of copying active PR tips.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones; applicable [Golden User Journeys](planning/golden-user-journeys.md) are also required.
5. No provider availability or reachability creates authority. No active operation silently changes provider.
6. No production native mutation is enabled merely to satisfy a roadmap number; applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.
7. Completed phases are never renumbered. Only future not-yet-started phases may be reordered by explicit repository planning/architecture reconciliation.

## Current authorization boundary

Phase 65 is **active**. Phase 65.A through 65.D are closed for their accepted bounded scopes.

The next planned Phase-65 product vertical is **65.E — Client playback abstraction**. It must keep platform playback engines behind a small Suite semantic layer rather than creating a universal Suite-owned decoder/player core. The first-party abstraction may expose operations such as open, play/pause/stop, seek where actually supported, track selection, position/state, classified failure and close while continuing to consume Suite-owned MediaSession semantics.

The media truthfulness boundary remains binding throughout 65.E and later client work:

1. advertise Range/seek only when the selected source/profile truly supports it;
2. represent completed versus growing source state explicitly where it affects capability;
3. do not invent immutable length or time-seek for continuous progressive fMP4;
4. preserve normalized Suite media/track identity independently of provider-native paths/PIDs where public semantics require it;
5. keep ADR-0053 least-transformation selection and ADR-0055 transcode policy independent of client brand/user-agent;
6. keep provider-native paths private and preserve MediaSession/Gateway authorization and deterministic cleanup.

Full arbitrary VOD time-seek/VDR-index mapping, user-visible growing-Recording seek, durable resume/progress and timeshift require their own demonstrated gap and coherent implementation scope. Their absence must be represented truthfully; they are not silently reintroduced as an already-started 65.C block.

Phase 66 Broadcast Companion, Legacy OSD and broad Timer UI are not authorized by this Phase-65 boundary.
