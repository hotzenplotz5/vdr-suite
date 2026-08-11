# VDR-Suite Current State

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR-0044 Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [Architecture Decision Records](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
96fab8ad88eae9ea0d46adf4db50ccf8d750a19b

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Next strict runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Merged Phase-64 foundation:
Slice 1 - TimerIntent Domain Contract, PR #150
Slice 2 - TimerIntent Persistence and Repository Semantics, PR #152

Current stacked Draft tip:
PR #169 - Add NativeTimerBinding absence application
branch: agent/phase64-native-timer-binding-absence-application
head: 9e54a1c2c3087f6eb9a9317b5c1f8ab3dd43525e
state: open Draft, mergeable
CI: VDR-Suite CI #7413 / run 31463690316; recheck the exact current result before relying on it
runtime change: none; Slice 17 does not wire an installed native Timer mutation path
```

Always re-read current `main`, the exact active PR head and CI before resuming work.

## Phase 64 stack

```text
#153  TimerAssignment domain contract
#154  TimerAssignment persistence repository
#155  deterministic TimerAssignment planner
#158  primary assignment scheduling handoff
#159  assignment-set revision concurrency fence
#160  replica assignment scheduling handoff
#161  NativeTimerBinding domain contract
#162  NativeTimerBinding persistence repository
#163  VDR -> NativeTimerObservation mapper
#164  safe present-readback application
#165  operation-bound expected PRESENT readback evidence
#166  operation-aware PRESENT readback verification
#167  complete native Timer inventory / authoritative absence evidence
#168  failure-aware RESTfulAPI complete-Timer-inventory reader
#169  NativeTimerBinding authoritative absence application
```

Draft PR #157 is the separate SQLite architecture-baseline repair. Draft PR #156 is a separate proposed client playback/media-adaptation ADR and does not implement Phase-65 runtime.

## Current Timer safety position

The stack now has distinct Suite intent, assignment and native-binding identities; revision and assignment-set concurrency fences; deterministic primary/replica planning; backend-neutral native observations; operation-aware PRESENT verification; complete-inventory absence proof; a failure-aware RESTfulAPI inventory producer; and durable absence application.

Slice 17 records the authoritative fact that a bound native Timer is missing while preserving the last known present state and fingerprint, the first `missingSince`, existing verified-operation evidence and exact binding-revision fencing. It deliberately does not invent an external-delete cause. A later inventory showing the Timer present after durable missing evidence requires reconciliation rather than silently clearing the state.

Production native Timer create/update/remove remains disabled.

## Exact next bounded work

After PR #169, the next slice is an operation-aware **expected absence** contract for Suite-managed native Timer removal.

It must bind the operation identity/state, exact binding identity and expected revision, exact backend ID/generation, exact backend-native Timer identity and a post-operation `readbackNotBefore` fence. Only this explicit operation context plus authoritative complete-inventory absence may later verify a removal.

The contract itself must not classify external changes, transition assignments, perform failover or execute native mutation. Those remain later bounded slices.

## Phase ordering and Timer UI gate

The strict numbered runtime order is:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

A broad polished Timer UI is not the Phase-64 completion gate. It remains separately gated on account/backend access management built on Phase 62.

Therefore Phase 65 Streaming may deliberately begin after the reliable Phase-64 Timer engine is complete even if the broad Timer UI is still deferred. Streaming is not technically dependent on that UI, and the broad Timer UI is not a prerequisite for Phase 65.

## Authority boundary

Phase-62 identity/authorization/accountability and Phase-63 Agent/generation/provider-ownership contracts remain authoritative. Stale or ambiguous generation, revision and readback evidence fails closed. Provider availability never creates implicit execution authority. TVScraper remains upstream-only.

## Historical completed context

The following markers are retained for documentation-entrypoint continuity; they are historical, not the current active phase:

- Phase 61 - Suite Metadata and Genre Platform
- Post-Phase 61 Performance Hardening (B1-B4)
- VDR Remote and Live Overlay hardening (#110)
- Backend-scoped Global Search (#111)
- Phase 62 - Identity, RBAC and Accountability Foundation

## Documentation synchronization note

`CURRENT.md`, `NEW-CHAT-HANDOFF.md` and `development/current-status.md` are the direct operational status entry points.

The current-position blocks in `planning/roadmap.md` and `planning/phase-map.md` still lag the active Phase-64 stack. Their architecture and phase-order rules remain useful, but stale active-slice markers must not override exact GitHub state. A broader guarded planning-document synchronization is separate.
