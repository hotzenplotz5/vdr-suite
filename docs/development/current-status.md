# VDR-Suite Current Project Status

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
PR #150 - TimerIntent Domain Contract
PR #152 - TimerIntent Persistence and Repository Semantics

Current stacked Draft tip:
PR #169 - Add NativeTimerBinding absence application
branch: agent/phase64-native-timer-binding-absence-application
head: 9e54a1c2c3087f6eb9a9317b5c1f8ab3dd43525e
CI: VDR-Suite CI #7413 / run 31463690316; recheck exact current result
runtime acceptance: not required for Slice 17 because no installed runtime path changes
```

Exact GitHub state always overrides a recorded checkpoint.

## Completed Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract. These remain prerequisites for Phase-64 orchestration.

## Historical completed context

Earlier completed platform foundations remain authoritative history, including:

- Phase 62 - Identity, RBAC and Accountability Foundation
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)

## Phase 64 progress

```text
Slice 1   PR #150 - TimerIntent domain contract                         MERGED
Slice 2   PR #152 - TimerIntent persistence/repository                  MERGED
Slice 3   PR #153 - TimerAssignment domain contract                     DRAFT
Slice 4   PR #154 - TimerAssignment persistence repository              DRAFT
Slice 5   PR #155 - deterministic TimerAssignment planner               DRAFT
Slice 6   PR #158 - primary assignment scheduling handoff               DRAFT
Slice 7   PR #159 - assignment-set revision fence                       DRAFT
Slice 8   PR #160 - replica assignment scheduling handoff               DRAFT
Slice 9   PR #161 - NativeTimerBinding domain contract                  DRAFT
Slice 10  PR #162 - NativeTimerBinding persistence repository           DRAFT
Slice 11  PR #163 - VDR NativeTimerObservation mapper                   DRAFT
Slice 12  PR #164 - safe present-readback application                   DRAFT
Slice 13  PR #165 - expected PRESENT readback contract                  DRAFT
Slice 14  PR #166 - operation-aware PRESENT verification                DRAFT
Slice 15  PR #167 - complete inventory / authoritative absence evidence DRAFT
Slice 16  PR #168 - failure-aware RESTfulAPI inventory reader           DRAFT
Slice 17  PR #169 - NativeTimerBinding absence application              DRAFT
```

Draft PR #157 is a separate SQLite architecture-baseline repair. Draft PR #156 is a separate proposed media/player ADR.

## Current Timer guarantees

The stack now provides stable Suite identity, revision/concurrency fencing, deterministic primary/replica planning, copied native Timer observations, operation-aware PRESENT verification, complete-inventory absence proof, failure-aware inventory acquisition and durable missing-state application.

Slice 17 preserves the last known present state/fingerprint and first authoritative `missingSince`, advances only valid observation evidence, uses exact binding-revision optimistic concurrency and deliberately keeps first missing-state cause conservative. It never turns absence alone into an external-delete conclusion.

No current Phase-64 Draft enables production native Timer mutation.

## Exact next bounded slice

Define an operation-aware expected-absence contract for Suite-managed native Timer removal. It must bind operation identity/state, binding identity/revision, backend/generation, native Timer identity and `readbackNotBefore`.

Only explicit operation context plus complete inventory absence may later verify a removal. External-change classification, assignment transitions, replacement/failover and native execution remain separate later work.

## Streaming and Timer UI ordering

Strict phase order remains:

```text
Phase 64 Timer orchestration
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD bridge
  -> Phase 67 Public API/client hardening
```

The broad Timer UI is not required to close Phase 64. It remains gated on account/backend access management built on Phase 62. Phase 65 may therefore start before that broad Timer UI is finished; this is intended sequencing, not a technical dependency.

## Development rules

- Root-level `AGENTS.md` is binding.
- Verify current `main`, exact PR head and final-head CI before writes or status claims.
- Keep active Timer stack PRs Draft unless the user explicitly approves a review-state change.
- Do not merge, rebase, force-push or rewrite published history without explicit approval.
- Use the smallest evidence-backed slice and no unrelated refactors.
- Require real-system acceptance only when an installed/runtime boundary actually changes.
- Broad Timer UI work must not bypass its account/backend access-management gate.

## Documentation synchronization note

The direct status entry points are [Current State](../CURRENT.md), [New Chat Handoff](../NEW-CHAT-HANDOFF.md) and this file.

The active-slice markers in [Strict Roadmap](../planning/roadmap.md) and [Phase Map](../planning/phase-map.md) still lag and require a separate guard-aware documentation synchronization. Their architecture and phase-order rules remain valid.

## Exact next action

1. Re-read PR #169 and its exact current CI before continuation.
2. Continue from its exact head only if the stack remains unchanged.
3. Add the expected-absence contract as a separate bounded stacked Draft.
4. Keep absence verification, external-change classification, failover and native mutation out of that contract slice.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Phase 64 TimerIntent Contract](phase-64-timer-intent-contract.md)
- [Phase 64 TimerIntent Repository](phase-64-timer-intent-repository.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Agent Workflow Rules](../../AGENTS.md)
