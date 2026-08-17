# VDR-Suite Current State

## Operational status authority

**This file is the sole repository authority for volatile operational status.**

`README.md`, `NEW-CHAT-HANDOFF.md`, `development/current-status.md`, the Strict Roadmap and the Phase Map may describe stable architecture, phase order and workflow rules, but they must not become competing copies of active PR tips or transient CI state.

Before any implementation, review-state change, installation or status claim, re-read current GitHub state. Recorded values below are verified checkpoints, not a substitute for a live read.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Current Project Status](development/current-status.md)
- [Phase 64 Closeout](development/phase-64-closeout.md)
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
72e298a76f7879ea7fc58f6a502e32eca7399f5a

Latest completed numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
none - Phase 65 has not started

Next strict numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions
```

Phase 64 closed through PR #195. The exact accepted implementation candidate was `bdd70d527d640dc115a7c141e505140ce8cdba9a`; PR #195 merged that candidate into `main` as `72e298a76f7879ea7fc58f6a502e32eca7399f5a`.

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

The completed Timer engine now coherently implements the accepted ADR-0044 separation:

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

## Phase ordering and Timer UI decision

The binding numbered order is now:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [NEXT, NOT STARTED]
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

The broad polished Timer UI was intentionally not a Phase-64 completion gate. It remains separately gated on account/backend access management built on the Phase-62 identity and authorization model. Phase 65 may therefore begin before the broad Timer UI is completed.

## Streaming architecture already prepared

ADR-0046 remains the accepted server-side MediaSession/Gateway boundary. Existing playback/media-adaptation planning must be reconciled with the completed Phase-64 platform before Phase-65 runtime implementation begins.

The intended direction remains:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Transformation preference is `pass-through -> remux/repackage -> transcode`. Streamdev may be an explicitly owned private provider, but it is not the public playback API or an implicit fallback chain.

## Binding execution-governance decisions

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project status. Other documents link here instead of copying active PR tips.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones; applicable [Golden User Journeys](planning/golden-user-journeys.md) are also required.
5. No provider availability or reachability creates authority. No active operation silently changes provider.
6. No production native mutation is enabled merely to satisfy a roadmap number; applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.

## Exact next action

The next repository task is the Phase-64 closeout documentation synchronization represented by this branch/PR. After that lands, perform a bounded Phase-65 planning review against ADR-0046 and the current architecture before authorizing the first Streaming runtime vertical.

Do **not** infer from this document that Phase 65 implementation has already started or is automatically authorized.
