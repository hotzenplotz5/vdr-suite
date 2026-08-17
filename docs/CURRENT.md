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
```

Phase 65 was explicitly started after the accepted post-Phase-64 roadmap/ADR reconciliation. Its first bounded implementation vertical is existing-Recording playback through the Suite-owned MediaSession / Streaming Gateway path. Live TV, general seek/growing-Recording behavior and Phase 66 remain outside this first vertical.

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

ADR-0046 and ADR-0053 jointly define the accepted server/client media direction:

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

The active first product vertical is Recording playback. Browser playback is the first real-system validator, not the architecture authority; the same media/session/source contracts must remain usable by Android/Android TV, Apple platforms, Windows, Kodi-style clients and television runtimes.

The bounded implementation contract lives in [Phase 65 Media Capability Negotiation Contract](development/phase-65-media-capability-negotiation-contract.md). Its completion requires real yaVDR picture + sound and deterministic cleanup on the exact candidate, not merely an HTTP response.

## Binding execution-governance decisions

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project phase status. Other documents link here instead of copying active PR tips.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones; applicable [Golden User Journeys](planning/golden-user-journeys.md) are also required.
5. No provider availability or reachability creates authority. No active operation silently changes provider.
6. No production native mutation is enabled merely to satisfy a roadmap number; applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.
7. Completed phases are never renumbered. Only future not-yet-started phases may be reordered by explicit repository planning/architecture reconciliation.

## Current authorization boundary

Phase 65 is **active**.

The currently authorized runtime scope is only the first coherent Recording-playback vertical:

1. stable Suite Recording identity resolves to a private local provider source;
2. source facts and client capabilities select the least-transformation presentation profile;
3. MediaSession / MediaRoute / ProviderStreamLease / MediaAccessGrant lifecycle is explicit and durable where required by ADR-0046;
4. the Gateway delivers bytes without exposing native paths/provider URLs or materializing the whole Recording in memory;
5. browser playback proves visible picture and audible sound;
6. stop/revoke deterministically cleans provider/adaptation/session state;
7. exact-head CI and real yaVDR acceptance close the vertical.

Live-TV allocation/channel switching, general seek/growing-Recording behavior, Phase 66 Broadcast Companion, Legacy OSD and broad Timer UI are not authorized by this kickoff.