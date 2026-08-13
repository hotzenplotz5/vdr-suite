# VDR-Suite Current State

## Operational status authority

**This file is the sole repository authority for volatile operational status.**

Stable architecture belongs in `docs/architecture/` and accepted ADRs. Binding numbered phase order and completion gates belong in `docs/planning/roadmap.md`. Historical exact acceptance evidence belongs in phase/slice closeouts. Other current/navigation documents must link here rather than copy active heads, PR tips or CI checkpoints.

Before implementation, review-state changes, installation or status claims, re-read live GitHub state. Values below are verified checkpoints, not a substitute for that live read.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Current Project Status](development/current-status.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [ADR-0044 Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Playback and Media Adaptation](adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
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
```

Planning/documentation synchronization PR #170 is merged on `main`. Its merge established the repository authority model, Golden User Journeys, the post-#190 implementation hold and the rule that Phase 65 may precede the broad Timer UI once the reliable Phase-64 Timer engine is complete.

## Current Phase-64 implementation checkpoint

The current stacked Timer implementation checkpoint remains Draft PR #190:

```text
PR #190 - Add disabled SuiteBridge Timer delete transport
branch: agent/phase64-suitebridge-timer-delete-disabled-transport
head: f81bf14c34deb878681833cff84a5b1f45c54811
state: open Draft; not merged
exact-head hosted CI at synchronization audit: PASS
```

PR #190 is a strong fail-closed checkpoint but **not** the Phase-64 completion gate.

Through that stack, VDR-Suite has established the TimerIntent, TimerAssignment and NativeTimerBinding model, deterministic scheduling, native observation/readback, durable mutation-operation state, fenced Agent delivery, durable local starting/outcome state and a private typed SuiteBridge Timer-delete transport.

The transport remains deliberately disabled. The installed Agent does not gain production native Timer deletion from #190, and no real VDR Timer delete is accepted by that slice.

Before accepted native delete can exist, the remaining safety work includes the required exact-request replay/idempotency protection, reserve-before-side-effect semantics, typed native mutation callback and the authoritative readback/reconciliation path required by ADR-0044/ADR-0042.

## Current implementation hold

**No Phase-64 successor implementation is currently authorized.**

Do not create or start `#191` merely because an earlier slice document names a possible successor. The next Timer work must first be explicitly selected as the smallest **coherent** remaining Phase-64 engine-completion change after the architecture/planning review.

This hold does not declare Phase 64 complete.

Phase-65 runtime work is also not authorized while the Phase-64 reliable Timer-engine gate remains open.

## Phase ordering and Timer UI decision

The binding numbered order is:

```text
Phase 64 - reliable Timer Intent and Multi-Backend Orchestration engine
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

The **broad polished Timer UI is not a Phase-64 completion gate**. It remains separately gated on account/backend access management built on the Phase-62 actor, credential, browser-session and backend-scoped authorization model.

Therefore Phase 65 Streaming may intentionally begin before the broad Timer UI, but only after the reliable Phase-64 Timer engine itself satisfies its completion gates.

## Streaming and playback planning state

ADR-0046 is the accepted server-side Streaming Gateway / MediaSession boundary.

Draft PR #156 is the current separate architecture workstream for proposed ADR-0053, covering client playback engines and media adaptation. Its synchronized direction is:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

The transformation preference is:

```text
pass-through -> remux/repackage -> transcode
```

Streamdev may be an explicitly owned private provider, but it is not the public playback API, not a universal dependency and not an implicit fallback.

Kodi remains an architecture reference; VDR-Suite does not extract Kodi VideoPlayer into a universal Suite player core. First-party clients use mature platform-appropriate playback engines.

The initial Phase-65 product-validation direction, once Phase 65 is authorized, is a coherent vertical browser playback proof through Suite-owned contracts to **real picture and sound**, followed by Live-TV channel-change/resource-cleanup and truthful Recording seek/growing semantics. Golden User Journeys 1, 2 and the media failure behavior from Journey 5 are the product acceptance anchors.

## Binding execution-governance decisions

1. A chat discussion is not a project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
2. `CURRENT.md` owns volatile project status; stable documents do not duplicate active PR/SHA/CI snapshots.
3. A slice is the **smallest coherent safety or product change**, not the smallest mechanically possible diff.
4. Technical CI and architecture guards are necessary but not sufficient for user-visible milestones; relevant Golden User Journeys must also pass.
5. Provider availability or reachability never creates authority. Active operations and media routes do not silently change provider.
6. Native mutation is never enabled merely to satisfy a roadmap number; applicable revision, generation, provider, idempotency, durable-starting, readback and real-system gates remain mandatory.
7. Phase-65 media work must preserve the ADR-0046 security/route boundary and ADR-0053 least-transformation/player boundary once ADR-0053 is accepted.

## Exact next action

The current planning sequence is:

1. finish review/synchronization of Draft PR #156 / proposed ADR-0053 against ADR-0046, the merged #170 planning model and Golden User Journeys;
2. decide explicitly whether ADR-0053 is accepted;
3. determine the smallest coherent remaining Phase-64 engine-completion work required by ADR-0044 and Golden User Journeys 3-5;
4. only then authorize a successor Timer implementation, if required;
5. after the reliable Phase-64 Timer engine is complete, begin Phase 65 with a vertical media proof before broad Timer-UI completion.

No merge/Ready action for PR #156 and no Phase-65 runtime implementation is implied by this status update.
