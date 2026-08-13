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
- [Architecture Decision Records](adr/index.md)
- [Agent Workflow Rules](../AGENTS.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main checkpoint:
08a87f2f8afb1ccec30ad739155a2eb121d98e37

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Next strict numbered runtime phase after Phase 64:
Phase 65 - Streaming Gateway and Media Sessions

Merged Phase-64 foundation on main:
PR #150 - TimerIntent Domain Contract
PR #152 - TimerIntent Persistence and Repository Semantics

Current stacked implementation checkpoint:
PR #190 - Add disabled SuiteBridge Timer delete transport
branch: agent/phase64-suitebridge-timer-delete-disabled-transport
head: f81bf14c34deb878681833cff84a5b1f45c54811
state: open Draft; not merged
relation to current main at audit: diverged, 63 commits ahead / 3 behind; merge base cb6f56e28bc981c8a3c86605fd8e842df4a86ab3
GitHub CI: VDR-Suite CI #7461 / run 31691807149 - PASS on the exact head
```

## Current implementation hold

The agreed implementation checkpoint is **after PR #190**.

No Phase-64 successor implementation is currently authorized. In particular, do not create or start a `#191` Timer implementation merely because the Slice-34 development note names a possible successor. The project is intentionally paused for architecture, roadmap and documentation synchronization.

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

The exact-head GitHub CI is green. The supplied real-system gate for the same head additionally recorded PASS while keeping the Timer inventory and `timers.conf` unchanged, executing no Timer deletion, restoring the original SuiteBridge plugin and preserving the Agent identity.

## What PR #190 does not prove

PR #190 is **not** the Phase-64 completion gate.

Its own slice contract deliberately has no accepted mutation outcome and no real VDR Timer-delete callback. The next technical prerequisites named by that slice include a plugin-instance-scoped exact-request replay ledger, reserve-before-side-effect semantics and a typed mutation callback before any accepted native delete can exist.

More broadly, ADR-0044 remains authoritative for the reliable Timer engine. Phase 64 is complete only when the required managed Timer lifecycle is coherently proved across intent, assignment, native binding, safe mutation, authoritative readback, reconciliation and the required real-VDR acceptance. A transport being wired but disabled is therefore a checkpoint, not an engine closeout.

## Current Phase-64 stack checkpoint

The current stacked implementation line is:

```text
#153  TimerAssignment domain contract
#154  TimerAssignment persistence repository
#155  deterministic TimerAssignment planner
#158  primary assignment scheduling handoff
#159  assignment-set revision fence
#160  replica assignment scheduling handoff
#161  NativeTimerBinding domain contract
#162  NativeTimerBinding persistence repository
#163  VDR -> NativeTimerObservation mapper
#164  present-readback application
#165  expected PRESENT readback contract
#166  operation-aware PRESENT verification
#167  complete native Timer inventory / absence evidence
#168  failure-aware RESTfulAPI inventory reader
#169  NativeTimerBinding absence application
#171  expected absence readback contract
#172  operation-aware absence verification
#173  shared MutationOperation repository
#174  delete-operation completion after verified readback
#175  delete-operation preparation handoff
#176  delete dispatch claim/outcome
#177  native Timer-delete Agent contract
#178  Timer-delete assignment persistence
#179  fenced Timer-delete delivery
#180  durable local Timer-delete starting state
#181  generic Agent command-state extension
#182  commands.state v3 integration
#183  Timer-delete local-state lifecycle
#184  fresh durable starting handoff
#185  fenced Timer-delete executor contract
#186  durable executor outcomes
#187  extracted protected Agent command-state store
#188  extracted Native Probe command handler
#189  extracted Timer-delete command handler
#190  disabled SuiteBridge Timer-delete transport
```

PR #156 is the separate proposed client playback/media-adaptation ADR. PR #157 is the separate SQLite architecture-baseline repair. PR #170 is the separate documentation/status synchronization workstream. These are not additional Timer-engine slices.

## Phase ordering and Timer UI decision

The binding numbered order remains:

```text
Phase 64 - reliable Timer Intent and Multi-Backend Orchestration engine
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

The **broad polished Timer UI is not a Phase-64 completion gate**. It remains separately gated on account/backend access management built on the Phase-62 identity and authorization model.

Therefore Phase 65 Streaming may intentionally begin before the broad Timer UI is completed, but only after the reliable Phase-64 Timer engine itself satisfies its completion gates. Streaming is not technically dependent on the broad Timer UI.

## Streaming architecture already prepared

ADR-0046 remains the accepted server-side MediaSession/Gateway boundary. Draft PR #156 already contains the complementary proposed playback/media-adaptation ADR. Its direction remains compatible with the current architecture:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Transformation preference is `pass-through -> remux/repackage -> transcode`. Streamdev may be an explicitly owned private provider, but it is not the public playback API or an implicit fallback chain. This planning work does not authorize Phase-65 runtime before Phase 64 completes.

## Binding execution-governance decisions

The following rules apply to further planning and implementation:

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project status. Other documents link here instead of copying exact active heads and CI state.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff. Avoid artificial intermediate states and unnecessarily long dependent stacks unless a real safety, concurrency, compatibility or acceptance boundary requires the split.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones. Relevant milestones also require end-to-end user-journey acceptance.
5. No provider availability or reachability creates authority. No active operation silently changes provider.
6. No production native mutation is enabled merely to satisfy a roadmap number; all applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.

The target user journeys are maintained in [Golden User Journeys](planning/golden-user-journeys.md).

## Exact next action

The next action is planning/documentation synchronization, not another Timer implementation:

- bring PR #170 up to the PR-#190 checkpoint and make the document-authority hierarchy explicit;
- synchronize the Strict Roadmap and Phase Map without treating PR #190 as Phase-64 completion;
- preserve the already-decided ordering that Streaming may precede the broad Timer UI after the Timer engine is complete;
- review/update proposed playback ADR PR #156 against the then-current canonical planning documents;
- only after that review decide and explicitly authorize the remaining Phase-64 completion work.

No PR Ready/merge/close/retarget action is implied by this planning synchronization.
