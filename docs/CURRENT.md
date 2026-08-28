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
- [Current Architecture State](development/current-architecture-state.md)
- [Phase 64 Closeout](development/phase-64-closeout.md)
- [Phase 65 Closeout](development/phase-65-closeout.md)
- [Phase 65 Recording Playback Closeout](development/phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](development/phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](development/phase-65-recording-startup-progressive-direct.md)
- [Phase 65.C Media Transcode Performance / Output Policy](development/phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](development/phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Browser-local Volume/Mute Closeout](development/phase-65d-browser-volume-mute-closeout.md)
- [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md)
- [Frontend Playback Integration Contract](development/frontend-playback-integration-contract.md)
- [ADR-0055 Media Transcode Backend Selection](adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR-0057 Recording Network Interruption Recovery](adr/ADR-0057-recording-network-interruption-recovery.md)
- [ADR-0058 Media Home, Responsive Browse and Preview Experience](adr/ADR-0058-media-home-responsive-browse-preview.md)
- [Phase 66 Media Home and Browse Experience](development/phase-66-media-home-browse-experience.md)
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
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Latest completed Phase-66 slice:
Slice 66.1 - Home Shell and Responsive Information Architecture

Current active runtime slice:
Slice 66.2 - Live-TV Hero Carousel

Completed Phase-65 product verticals:
65.A - Existing-Recording playback
65.B - Live-TV playback
65.C - Recording delivery performance and media output/transcode settings
65.D - Client playback abstraction

Accepted Phase-65.D bounded work:
65.D.1 - Persistent Browser Playback Shell
65.D.2 - Recording Playback Controls and Seek
normalized Recording audio-track selection
normalized Recording subtitle selection including browser WebVTT delivery
browser-local Volume/Mute controls
continuous-fMP4 browser MSE forward-buffer/backpressure
compatibility timeline drag ownership
exact non-zero HLS Recording resume synchronization
normalized MediaPlaybackContract
canonical playback-owner lifecycle publication
continuity/discontinuity and playback-presentation generation semantics
classified playback failures
bounded completed-Recording network-interruption recovery

Phase-65.D optional follow-up, not a completion gate:
read-only media pipeline diagnostics
shared fMP4/MSE helper deduplication
```

**Phase 65 is completed.** Phase 65.A through 65.D are closed for their accepted bounded scopes. The final Phase-65 runtime-sensitive follow-up was the completed-Recording network-interruption recovery merged through PR #228 after exact yaVDR install/runtime identity verification, full hosted CI and real Android/Edge long-outage acceptance.

The earlier planning label `65.C - Recording seek and growing-recording semantics` remains superseded by the implementation history. Phase 65.C actually delivered completed-Recording startup/progressive performance and backend-scoped media-transcode/output policy. Truthful range/seek/growing capability remains a binding media invariant rather than a product-vertical label.

The old separate `65.D - Compatibility escalation` planning block was absorbed by demonstrated compatibility/performance work inside 65.C and never started as an independent vertical. The replacement **65.D - Client playback abstraction is now closed.** Its accepted scope includes the persistent shell, Recording controls/seek, normalized tracks, browser-local Volume/Mute, continuous-fMP4 forward-buffer control, compatibility timeline ownership, exact HLS resume, the four mandatory ADR-0056 semantic slices and the bounded ADR-0057 Recording network-recovery follow-up.

The ADR-0056 mandatory semantic sequence is complete: normalized provider-free `MediaPlaybackContract`, canonical playback-owner lifecycle publication, explicit continuity/discontinuity plus presentation-generation semantics and classified playback failures. Read-only media diagnostics were explicitly sequenced after semantic correctness and are recommended observational follow-up rather than a Phase-65.D completion gate. Shared fMP4/MSE helper deduplication remains technical debt.

Truthful range/seek/growing-recording capability remains binding after Phase 65 closeout. Completed-Recording arbitrary time-seek and stop/resume are accepted for supported progressive-fMP4 and HLS restart-seek paths. Compatibility timeline interactions preserve canonical absolute Recording position across transport-local time, and exact non-zero HLS video resume uses a synchronized implemented adaptation path or fails closed. User-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping not required by the accepted completed-Recording paths remain deferred and must not be fabricated.

Phase 66 is **active for Slice 66.2 — Live-TV Hero Carousel** after Slice 66.1 was accepted on the real yaVDR system and merged through PR #231. The active slice is limited to immediate browse-only Live-TV hero selection, canonical Channel/EPG projection, responsive neighbor context and explicit delegation to the existing Live-TV/EPG owners. Selection must remain separate from playback state. Slice 66.3 delayed preview and every later Phase-66 semantic remain outside the current authorization. Accepted ADR-0054 remains the Broadcast Companion architecture for the following Phase 67.

Phase 64 closed through PR #195. The exact accepted implementation candidate was `bdd70d527d640dc115a7c141e505140ce8cdba9a`; PR #195 merged that candidate into `main` as `72e298a76f7879ea7fc58f6a502e32eca7399f5a`.

The current `main` commit itself is intentionally **not** copied into this file. Query live GitHub state whenever an exact repository checkpoint matters.

## Historical foundation markers

`Phase 63 - Backend Agent and Secure Multi-Site Runtime` remains the completed execution/provider foundation beneath Phase 64 and Phase 65. This marker is retained for historical contract guards and traceability only; it does not make Phase 63 current or active again.

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
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [ACTIVE; SLICE 66.2]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Future phases 67+ are not runtime-authorized merely because they are named here. The strict details and gates live in the [Roadmap](planning/roadmap.md).

The Phase-66 Media Home architecture is defined by accepted ADR-0058. Runtime is currently authorized only for Slice 66.2. Slice 66.1 is completed; later Phase-66 slices remain gated by the accepted sequence and current scope. Broadcast Companion architecture remains defined by ADR-0054 for Phase 67.

## Timer Product UI decision

The broad polished Timer UI was intentionally not a Phase-64 or Phase-65 completion gate and is not inserted as a numbered phase.

It remains a cross-cutting product milestone with prerequisites:

```text
Phase 62 identity/RBAC foundation [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

The UI must remain intent-first and show assignment, fulfillment, reconciliation and failover state without collapsing uncertain native outcomes into fake success/failure.

Phase 65 completed independently of the broad Timer UI.

## Completed Phase 65 architecture

ADR-0046 and ADR-0053 jointly define the accepted server/client media direction. ADR-0055 adds the accepted media-transcode backend-selection and hardware-acceleration contract. ADR-0056 adds the accepted normalized playback presentation, timeline, continuity and failure-semantics layer. ADR-0057 adds the bounded completed-Recording network-interruption recovery policy.

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> least-transformation media adaptation
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

`MediaPresentationProfile` remains the internal adaptation/execution plan. `MediaPlaybackContract` is the provider-free first-party semantic contract. Transformation preference remains `pass-through -> remux/repackage -> transcode`, except where a demonstrated operation requires stronger transformation for correctness. Exact non-zero HLS video resume is one accepted operation-specific example: ordinary start-at-zero retains copy/remux when valid, while an exact non-zero resume uses the implemented synchronized transcode path or fails closed.

Streamdev may be an explicitly owned private provider, but it is not the public playback API or an implicit fallback chain.

Browser playback was the first real-system validator, not the architecture authority; the same media/session/source contracts remain intended for Android/Android TV, Apple platforms, Windows, Kodi-style clients and television runtimes.

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

## Phase 65.C Recording delivery performance and media output/transcode settings

Phase 65.C is the accepted bounded media-delivery/performance vertical that was implemented in two coherent successive blocks.

### 65.C Recording startup / progressive delivery

PR #206 introduced the fast completed-Recording delivery path while preserving ADR-0053 least-transformation semantics:

- `progressive-direct` remains available only when native MPEG-TS, selected codecs and truthful byte ranges are supported;
- `progressive-fmp4` provides the normal low-latency browser path for compatible completed Recordings without the old HLS startup gate;
- HLS remains a compatibility fallback;
- continuous fMP4 does not advertise fake `Accept-Ranges`, `Content-Range`, immutable `Content-Length` or browser time-seek semantics;
- completed/growing source truth remains server-owned and completed-only fast paths fail closed when their immutable-source guarantee is not valid;
- provider-native paths remain private and MediaSession/Gateway authorization and cleanup remain authoritative.

Accepted evidence:

```text
accepted_65c_startup_candidate=51de13337edd0a072308a9df1bad6e245a764ac2
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
```

See [Phase 65.C Recording Startup / Progressive Direct](development/phase-65-recording-startup-progressive-direct.md).

### 65.C Media-transcode backend policy and output settings

The same authorized 65.C scope then continued through PR #208 with the backend-scoped output/encoder policy under ADR-0055:

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
accepted_65c_output_policy_candidate=85478311b9af6c027a25980272a2acde551e5508
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

See [Phase 65 Media Transcode Performance / Output Policy](development/phase-65-media-transcode-performance-policy.md) and [ADR-0055](adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md).

Phase 65.C is closed for this bounded combined delivery-performance/output-policy scope.

## Phase 65.D Client playback abstraction

Phase 65.D is accepted and closed. It keeps one Suite semantic owner around replaceable transports and mature platform playback engines.

### 65.D.1 Persistent Browser Playback Shell — CLOSED

Phase 65.D.1 established one persistent browser playback owner across first-party internal navigation. The same HTML media element remains the owned playback surface, can move between the Live-TV presentation and the persistent playback shell, and supports native browser/Android Picture-in-Picture without creating a second MediaSession or a second player architecture.

See [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md).

### 65.D.2 Recording Playback Controls and Seek — CLOSED

Phase 65.D.2 adds truthful completed-Recording playback controls and position semantics:

- Play, Pause and Stop;
- current position and authoritative completed-Recording duration;
- relative, timeline and direct absolute seek;
- progressive-fMP4 MediaSession worker repositioning without fake HTTP byte-range semantics;
- HLS/transcoding restart-seek through a fresh authorized MediaSession with `startPositionSeconds`;
- stop-state resume/from-beginning choice;
- Android-friendly direct-time entry;
- real yaVDR/browser acceptance across the supported progressive-fMP4 and HLS fallback paths.

The accepted runtime candidate is `fd1e64c3c28b3e184fb120d71ce692061b282c82`. Later documentation, workflow-governance and test-registration-only commits do not invalidate that runtime evidence.

See [Phase 65.D.2 Recording Playback Controls and Seek Closeout](development/phase-65d2-recording-playback-controls-seek-closeout.md).

### Normalized Recording audio/subtitle selection — CLOSED

PR #216 established the normalized Recording track contract and explicit audio-track selection without provider/PID leakage. Its accepted head was `c41a510e2ad6947e744fc4a688276bdbe22cc477`; VDR-Suite CI #8183 passed and the PR merged as `71a59fea4615729d9eba170890d312423adf98a2`.

PR #217 completed browser-selectable Recording subtitles for the proven VDR/vdr-rectools sidecar shape. The accepted head was `482de6529b87e21cdd8dfe4bd4791376c012fb02`; VDR-Suite CI #8225 (`32862649168`) passed and the PR merged as `736d7833318eee3ec11335fdffc72de2f578c032`.

Real yaVDR/browser acceptance proved normalized audio selection and the Recording SRT path with browser-native WebVTT delivery. The real inventory contained 179 `00001.srt` sidecars directly in their `.rec` directories. Normalized `subtitle-N`/`off` semantics remain public; DVB bitmap subtitles and Teletext are not falsely advertised as browser-selectable text tracks, and provider/PID details remain private.

### Browser-local Volume/Mute — CLOSED

The bounded browser-local Volume/Mute slice is accepted on runtime candidate `932aef5cd6e85b0fac1a5bf290a4bbeb06ff2d4b`. VDR-Suite CI #8238 (`32877244600`) passed on that exact runtime head.

Real yaVDR/browser acceptance proved audible 0..100 Recording volume changes, mute/unmute, unchanged seek and SRT behavior, the shared controls on the HLS compatibility path and applicable Live-TV behavior. The accepted owner remains the current `HTMLMediaElement`; Volume/Mute creates no second player or MediaSession, does not restart playback and does not mutate VDR/server volume.

See [Phase 65.D Browser-local Volume/Mute Closeout](development/phase-65d-browser-volume-mute-closeout.md).

### Continuous-fMP4 browser MSE forward-buffer/backpressure — CLOSED

PR #219 closed the demonstrated browser-side continuous-fMP4 forward-buffer defect while preserving one MediaSession, one Gateway stream and one HTML media owner.

```text
accepted_65d_mse_backpressure_candidate=bcb8b1a0cdbc874de296eb57967d260038d70ed8
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8243
source_ci_run_id=32885936866
source_ci_result=PASS
merge_pr=219
merge_commit=5a5789e09bbf79d0e87a25f09ebadce72918f68b

REAL_YAVDR_ANDROID_ACCEPTANCE=PASS
SOURCEBUFFER_FULL_REPRODUCED_ON_PREVIOUS_PATH=YES
FORWARD_HIGH_WATER_SECONDS=12
RECORDING_SEEK_REGRESSION=PASS
STOP_RESUME_REGRESSION=PASS
VOLUME_MUTE_REGRESSION=PASS
```

The browser MSE owner stops pulling the continuous HTTP stream while its SourceBuffer is at the accepted 12-second forward high-water mark and resumes only when playback/seeking creates room.

### Compatibility timeline ownership and exact HLS Recording resume — CLOSED

PR #220 closed the Android compatibility-mode timeline drag race and merged the stacked exact HLS resume synchronization work from PR #221.

The accepted implementation keeps ordinary HLS start at position zero on the least-transformation copy/remux path. For an exact non-zero HLS Recording resume with video, implemented H.264/AAC copied tracks are promoted to transcode, video encoder selection remains under ADR-0055 backend-scoped policy, unsupported exact-resume codec paths fail closed, and the lower HLS worker-plan guard rejects an unsafe copied-A/V non-zero video resume.

```text
accepted_65d_exact_hls_resume_candidate=870609e7b9a00949448aca1ea1c4db97399bf949
stacked_merge_commit=0d237855188ca2483649a72e5a6bb93c4c4b86f1
stacked_merge_tree_equivalent_to_candidate=YES
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8256
source_ci_run_id=32929643924
source_ci_result=PASS
parent_merge_pr=220
parent_merge_commit=5f9ecc3b1c0af831d8017204da9da63c9ad62610
```

### ADR-0056 Playback Semantics Consolidation — CLOSED

The required semantic layer above internal presentation/worker planning is implemented:

```text
internal MediaPresentationProfile
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The mandatory implementation sequence completed through PRs #224–#227:

```text
1. normalized MediaPlaybackContract                         [CLOSED]
2. canonical owner lifecycle snapshot/subscription         [CLOSED]
3. timeline + continuity/discontinuity semantics           [CLOSED]
4. classified playback failure semantics                   [CLOSED]
5. read-only media diagnostics after semantic correctness  [OPTIONAL / DEFERRED]
```

Accepted semantic heads and CI:

```text
PR #224 head=16b90081e2f458d84fccefeece49c70874f3267d CI=8266 PASS
PR #225 head=ec3af8506eb8086a3f717f977aaca85a93bdb2f3 CI=8280 PASS
PR #226 head=9cc26870515c32970767e0e418e841fcdbdcba5d CI=8282 PASS
PR #227 head=64c7f2c7dfdb4d3291d51d23daeec1a300e176a7 CI=8286 PASS
```

Read-only media diagnostics remain observational only. Shared fMP4/MSE helper extraction is separate technical debt and must not merge continuous-stream and HLS lifecycles into a universal player.

See [ADR-0056](adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md), [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md) and [Frontend Playback Integration Contract](development/frontend-playback-integration-contract.md).

### Completed-Recording network-interruption recovery — CLOSED

PR #228 closed the final demonstrated Phase-65 runtime-sensitive Recording gap without broadening into Live-TV recovery, provider switching or Phase 66.

The exact accepted final runtime candidate is:

```text
accepted_final_phase65_runtime_candidate=7193797368cd1ff637062d02d0d7c9e5bf435ebe
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8303
source_ci_run_id=33166818230
source_ci_result=PASS
merge_pr=228
merge_commit=131f669c0f4e360f3306cfb34f50380653a9fdfc
```

Real yaVDR install/runtime identity passed. Real Android/Edge acceptance reproduced a long outage through `waiting`, showed the explicit interrupted-network state and then resumed automatically near the interrupted absolute position after connectivity returned. No manual Play/Restart action, provider switch, hidden HLS fallback or unbounded retry was required.

## Binding execution-governance decisions

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project phase status. Other documents link here instead of copying active PR tips.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones; applicable [Golden User Journeys](planning/golden-user-journeys.md) are also required.
5. No provider availability or reachability creates authority. No active operation silently changes provider.
6. No production native mutation is enabled merely to satisfy a roadmap number; applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.
7. Completed phases are never renumbered. Only future not-yet-started phases may be reordered by explicit repository planning/architecture reconciliation.

## Current authorization boundary

Phase 65 is **completed**. **Phase 66 - Media Home and Browse Experience is active, with only Slice 66.2 - Live-TV Hero Carousel currently authorized.**

Slice 66.1 - Home Shell and Responsive Information Architecture is completed and merged. The next strict numbered runtime phase remains Phase 66 until its completion gate closes. Slice 66.3 Deferred Live Preview and every later Phase-66 slice remain outside the current Slice-2 authorization. Phase 67 Broadcast Companion Services: Teletext and HbbTV remains not started.

The media truthfulness boundary remains binding after closeout:

1. advertise Range/seek only when the selected source/profile truly supports it;
2. represent completed versus growing source state explicitly where it affects capability;
3. do not invent immutable length or HTTP byte-range semantics for continuous progressive fMP4;
4. preserve normalized Suite media/track identity independently of provider-native paths/PIDs where public semantics require it;
5. keep ADR-0053 least-transformation selection and ADR-0055 transcode policy independent of client brand/user-agent;
6. allow operation-specific stronger adaptation only when demonstrated correctness requires it and the implemented path remains policy-governed/fail-closed;
7. keep user-owned preview, transport-local time and canonical absolute Recording position distinct;
8. keep MediaSession identity, route epoch and playback presentation generation distinct;
9. keep provider-native paths private and preserve MediaSession/Gateway authorization and deterministic cleanup;
10. classified failures preserve detailed reason evidence and do not themselves trigger silent provider/profile/session recovery.

Completed-Recording arbitrary time-seek and stop/resume are accepted for the supported completed-Recording profiles. User-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping not required by those accepted paths remain deferred until a demonstrated gap justifies a coherent future scope. Their absence must remain truthful rather than silently fabricated.

Phase 66 Slice 2 does not authorize delayed Live preview, focus-triggered MediaSession creation, Continue Watching persistence, Recently Watched history, Broadcast Companion, Legacy OSD or broad Timer UI work.