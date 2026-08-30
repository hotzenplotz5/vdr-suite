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
- [Phase 66.4 Continue Watching Closeout](development/phase-66-continue-watching-closeout.md)
- [Phase 66.5 Recording Discovery Rails Closeout](development/phase-66-recording-discovery-rails-closeout.md)
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
Current Slice-66.6 work branch: work/phase66-recently-watched-history

Latest completed numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Latest completed Phase-66 slice:
Slice 66.5 - Recording Discovery Rails (PR #236)

Current active runtime slice:
Slice 66.6 - Recently Watched / History

Slice 66.4 Real-System Acceptance:
PASS

Slice 66.5 Real-System Acceptance:
PASS

Slice 66.5:
COMPLETED / ACCEPTED / MERGED

Slice 66.6:
AUTHORIZED / ACTIVE

Slice 66.7 and later Phase-66 slices:
NOT AUTHORIZED

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

Phase 66 has completed **Slice 66.5 — Recording Discovery Rails** after Slice 66.1 was accepted on the real yaVDR system and merged through PR #231, Slice 66.2 was merged through PR #232 with the real-browser keyboard-focus correction in PR #233, Slice 66.3 Deferred Live Preview was completed through PR #234, and Slice 66.4 Continue Watching was completed through PR #235. Slice 66.5 projects existing Recording, Genre and folder truth into lazy Home discovery rails without creating a second content catalog or navigation owner. The accepted runtime-sensitive candidate is `cd8133a814ba5325638fef0407915e294f8d125c`; VDR-Suite CI #8405 / run `33307208279` passed on that candidate and real yaVDR/Android-browser acceptance passed after exact install identity verification. Series remains conditional and fail-closed on canonical series evidence rather than Home-side heuristics. PR #236 was merged into `main` as `b789c72dc7770a9128226fa7e605eb570591e541` after final PR CI #8406 / run `33328122135` passed on the Slice-66.5 closeout head `39a55f8bb4e52b50c4920b487e5747b8844398d6`. Slice 66.6 — Recently Watched / History is now explicitly authorized on `work/phase66-recently-watched-history`; Slice 66.7 and every later Phase-66 semantic remain **NOT AUTHORIZED**. Accepted ADR-0054 remains the Broadcast Companion architecture for the following Phase 67.

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
  -> Phase 66 - Media Home and Browse Experience [ACTIVE; SLICE 66.6 ACTIVE]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Future phases 67+ are not runtime-authorized merely because they are named here. The strict details and gates live in the [Roadmap](planning/roadmap.md).

The Phase-66 Media Home architecture is defined by accepted ADR-0058. Slice 66.1, Slice 66.2, Slice 66.3, Slice 66.4 and Slice 66.5 are completed. Slice 66.6 is the only currently authorized Phase-66 runtime scope. Slice 66.7 and later Phase-66 slices remain gated by the accepted sequence and current scope. Broadcast Companion architecture remains defined by ADR-0054 for Phase 67.
