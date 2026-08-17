# VDR-Suite Current Project Status

## Status ownership

Exact operational state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second source for active branch heads, PR tips or transient CI run numbers.

Before continuing work, always read `CURRENT.md`, the Strict Roadmap, the applicable ADRs and live GitHub state.

## Platform position

Latest completed numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.

Current active numbered runtime phase: **none; Phase 65 has not started**.

Next strict numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.

Historical completed context remains relevant, including Phase 58 - Frontend and Live Parity, Phase 61 - Suite Metadata and Genre Platform, Phase 62 - Identity, RBAC and Accountability Foundation, and Phase 63 - Backend Agent and Secure Multi-Site Runtime.

## Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract.

That foundation remains authoritative beneath completed Phase-64 orchestration and future Phase-65 media execution.

## Phase 64 completion

Phase 64 is complete and deliberately separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The completed engine provides durable Timer intent/assignment/binding identities, deterministic backend ownership, managed native Timer create/update/toggle/delete fulfillment, durable no-blind-retry semantics, authoritative readback/reconciliation and controlled reassignment/failover.

The final reassignment/failover block allows replacement only before native dispatch or after exact authoritative absence. It atomically supersedes the old exclusive owner, creates the replacement with a new assignment identity/epoch, persists durable handover evidence and rechecks current candidate authority. Stale or ambiguous evidence fails closed.

The exact completion candidate, CI and real yaVDR evidence are recorded in [Phase 64 Closeout](phase-64-closeout.md). Exact current `main` remains owned by [Current State](../CURRENT.md).

## Timer mutation safety position

The Phase-64 completion retains the protected-write rules established across Phases 62-64:

- actor/backend authorization and backend write mode remain authoritative;
- backend generation and provider instance/generation are fenced;
- assignment, binding, operation and expected-state fingerprints are checked where applicable;
- a possible native dispatch is durably represented before completion can be claimed;
- ambiguous delivery is `outcome_unknown` and reconciliation-only;
- CREATE/UPDATE/TOGGLE require authoritative PRESENT readback;
- DELETE requires complete-inventory authoritative ABSENCE readback;
- reassignment cannot use `dispatching`, `executed_unverified`, `outcome_unknown`, incomplete inventory or ambiguous/external drift as replacement authority;
- public SuiteBridge SVDRP help remains closed for private Timer write commands.

## Streaming and Timer UI ordering

The intended strict order is now:

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [NEXT, NOT STARTED]
  -> Phase 66 Legacy OSD Compatibility Bridge
  -> Phase 67 Public API and Client Compatibility Hardening
```

A broad polished Timer UI is not required to close the Phase-64 engine. It remains separately gated on account/backend access management built on the Phase-62 identity/authorization model.

Therefore Phase 65 may intentionally start before the broad Timer UI is complete.

## Streaming architecture preparation

Accepted ADR-0046 already defines the server-side Streaming Gateway and MediaSession boundary. Existing client-playback/media-adaptation planning must be reviewed against the completed Phase-64 platform before the first Phase-65 runtime change is authorized.

The intended media direction remains provider-private and transformation-minimal: clients request Suite media capabilities rather than Streamdev/SuiteBridge URLs, and delivery prefers pass-through before remux/repackage before transcoding.

## Product acceptance

Component tests, CI, architecture guards and real-system safety checks remain mandatory. User-visible milestones additionally use [Golden User Journeys](../planning/golden-user-journeys.md).

The key vertical journeys cover Live TV playback, Recording playback, record-one-programme orchestration, multi-backend scheduling without provider knowledge and fail-closed recovery from backend/provider/transport failures.

## Development rules

- Root-level `AGENTS.md` is binding.
- `CURRENT.md` is the sole repository copy of volatile operational status.
- Verify live `main`, exact PR head and exact-final-head CI before writes or status claims.
- Keep review/merge/retarget/close state changes behind explicit user approval.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires them.
- Provider availability never creates execution authority and active work never silently switches provider.
- Require real-system acceptance when an installed/runtime or native-behaviour boundary changes.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- Phase 65 is not active until explicitly started.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded edit safely. Read the complete file content required for the change, write a coherent commit on the intended branch and inspect the resulting diff before treating the update as correct.

Use local edits first only when the change requires:

- local build/test execution that cannot be represented by the connector;
- multi-file transformations that are materially safer in a checked-out worktree;
- binary/generated-file handling unavailable through the connector; or
- a workaround because the GitHub connector blocks a file operation.

GitHub-first does not weaken review safety: keep updates fast-forward-only, do not replace a complete file from a truncated fetch, and do not mark Draft PRs Ready or merge them without explicit approval.

## Exact next action

Complete the Phase-64 closeout documentation synchronization. Then review ADR-0046 and existing playback/media-adaptation planning against current `main`, define the first coherent Phase-65 vertical and explicitly authorize it before runtime implementation begins.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Agent Workflow Rules](../../AGENTS.md)
