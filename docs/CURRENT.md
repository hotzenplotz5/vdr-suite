# VDR-Suite Current State

## Operational status authority

**This file is the sole repository authority for volatile operational status.**

`README.md`, `NEW-CHAT-HANDOFF.md`, `development/current-status.md`, the Strict Roadmap and the Phase Map may describe stable architecture, phase order and workflow rules, but they must not become competing copies of exact branch heads, active PR tips or CI checkpoints.

Before any implementation, review-state change, installation or status claim, re-read current GitHub state. Recorded values below are checkpoints, not a substitute for a live read.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Current Project Status](development/current-status.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [ADR-0044 Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Playback and Media Adaptation](adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Architecture Decision Records](adr/index.md)
- [Agent Workflow Rules](../AGENTS.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main checkpoint:
39de4d0b1ba2a670ae1677ee83d7029e89266f77

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Next strict numbered runtime phase after Phase 64:
Phase 65 - Streaming Gateway and Media Sessions

Merged planning synchronization:
PR #170 - Synchronize project planning at Phase 64 PR #190 checkpoint
merge: 39de4d0b1ba2a670ae1677ee83d7029e89266f77

Current stacked implementation checkpoint:
PR #190 - Add disabled SuiteBridge Timer delete transport
head: f81bf14c34deb878681833cff84a5b1f45c54811
state at latest audit: open Draft; not merged
```

## Current implementation hold

The agreed implementation checkpoint remains **after PR #190**.

No Phase-64 successor implementation is currently authorized. In particular, do not create or start a `#191` Timer implementation merely because the Slice-34 development note names a possible successor. The project is intentionally paused while the remaining Phase-64 completion work is explicitly determined.

This hold changes neither ADR-0044 nor the strict phase order. It is a planning gate, not a declaration that Phase 64 is complete.

## What PR #190 proves

The stacked Phase-64 work through PR #190 has reached a strong fail-closed native Timer-delete boundary:

- stable TimerIntent, TimerAssignment and NativeTimerBinding concepts and persistence exist in the stack;
- deterministic primary/replica scheduling and assignment-set concurrency fencing exist in the stack;
- native Timer present/absence evidence, operation-aware readback verification and durable mutation-operation state exist in the stack;
- Agent delivery, durable local `starting`, one-shot executor semantics and unknown-outcome recovery are fenced;
- a concrete private SuiteBridge `NTDEL` transport exists;
- SuiteBridge advertises that Timer-delete execution as disabled;
- the installed Agent does not advertise/configure `vdr.timer.delete` through this checkpoint;
- no real native VDR Timer delete is enabled by PR #190.

## What PR #190 does not prove

PR #190 is **not** the Phase-64 completion gate.

Its own slice contract deliberately has no accepted mutation outcome and no real VDR Timer-delete callback. The next technical prerequisites named by that slice include a plugin-instance-scoped exact-request replay ledger, reserve-before-side-effect semantics and a typed mutation callback before any accepted native delete can exist.

More broadly, ADR-0044 remains authoritative for the reliable Timer engine. Phase 64 is complete only when the required managed Timer lifecycle is coherently proved across intent, assignment, native binding, safe mutation, authoritative readback, reconciliation and the required real-VDR acceptance.

## Phase ordering and Timer UI decision

The binding numbered order remains:

```text
Phase 64 - reliable Timer Intent and Multi-Backend Orchestration engine
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

The **broad polished Timer UI is not a Phase-64 completion gate**. It remains separately gated on account/backend access management built on the Phase-62 identity and authorization model.

Therefore Phase 65 Streaming may intentionally begin before the broad Timer UI is completed, but only after the reliable Phase-64 Timer engine itself satisfies its completion gates.

## Streaming architecture decision

ADR-0046 remains the accepted server-side MediaSession/Gateway boundary.

ADR-0053, **Client Playback Engine and Media Adaptation Strategy**, was explicitly accepted on 2026-08-13 in PR #156. Until #156 is merged, live GitHub PR state remains the source for its exact head and CI evidence.

The accepted direction is:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Transformation preference is:

```text
pass-through -> remux/repackage -> transcode
```

The browser is the initial first-party Phase-65 product-validation client. Product acceptance must prove actual picture and sound, deterministic Live-TV channel-change cleanup, truthful Recording seek/growing semantics and classified failure behavior. Streamdev remains an explicitly owned private provider, never the public playback API or an implicit fallback chain.

Acceptance of ADR-0053 does **not** authorize Phase-65 runtime before the reliable Phase-64 Timer engine is complete.

## Binding execution-governance decisions

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project status. Other documents link here instead of copying exact active heads and CI state.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff. Avoid artificial intermediate states and unnecessarily long dependent stacks unless a real safety, concurrency, compatibility or acceptance boundary requires the split.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones. Relevant milestones also require end-to-end user-journey acceptance.
5. No provider availability or reachability creates authority. No active operation silently changes provider.
6. No production native mutation is enabled merely to satisfy a roadmap number; all applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.

## Exact next action

The next action remains **Phase-64 completion planning**, not another automatically authorized Timer slice and not Phase-65 runtime:

- complete review/merge of accepted ADR-0053 through PR #156;
- explicitly determine the smallest coherent remaining Phase-64 engine completion work required by ADR-0044 and Golden User Journeys 3-5;
- authorize successor Timer implementation only after that explicit decision;
- once the reliable Phase-64 Timer engine is complete, begin Phase 65 with the coherent Recording-playback vertical proof defined by ADR-0053, before the broad polished Timer UI if access-management gating still delays that UI.

No PR #191, Timer mutation enablement or Phase-65 runtime start is implied by acceptance of ADR-0053.
