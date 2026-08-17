# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second source for active branch heads, PR tips, transient CI run numbers or exact live `main` SHAs.

Before continuing work, always read `CURRENT.md`, the Strict Roadmap, the applicable ADRs and live GitHub state.

## Platform position

Latest completed numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.

Current active numbered runtime phase: **none; Phase 65 has not started**.

Next strict numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.

Historical completed context remains relevant, including Phase 58 - Frontend and Live Parity, Phase 61 - Suite Metadata and Genre Platform, Phase 62 - Identity, RBAC and Accountability Foundation, and Phase 63 - Backend Agent and Secure Multi-Site Runtime.

## Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract.

That foundation remains authoritative beneath completed Phase-64 orchestration and future media/broadcast/compatibility execution.

## Phase 64 completion

Phase 64 is complete and separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The completed engine provides durable Timer intent/assignment/binding identities, deterministic backend ownership, managed native Timer create/update/toggle/delete fulfillment, durable no-blind-retry semantics, authoritative readback/reconciliation and controlled reassignment/failover.

The final reassignment/failover block allows replacement only before native dispatch or after exact authoritative absence. It atomically supersedes the old exclusive owner, creates the replacement with a new assignment identity/epoch, persists durable handover evidence and rechecks current candidate authority. Stale or ambiguous evidence fails closed.

The exact completion candidate, CI and real yaVDR evidence are recorded in [Phase 64 Closeout](phase-64-closeout.md). Exact live `main` must be read from GitHub rather than copied into narrative status documents.

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

## Revised future ordering

The strict numbered order is:

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [NEXT, NOT STARTED]
  -> Phase 66 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 Legacy OSD Compatibility Bridge
  -> Phase 68 Public API and Client Compatibility Hardening
  -> Phase 69 Recommendation and Content Knowledge Graph
```

Completed history through Phase 64 is unchanged. The reorder affects only not-yet-started future phases.

Proposed ADR-0054 defines the intended Teletext/HbbTV architecture for Phase 66. That phase remains runtime-blocked until the ADR is explicitly accepted.

## Streaming architecture preparation

Accepted ADR-0046 defines the server-side Streaming Gateway and MediaSession boundary. Accepted ADR-0053 defines the complementary client-playback/media-adaptation direction.

The intended media direction is provider-private and transformation-minimal:

```text
private source
  -> explicitly owned provider
  -> ProviderStreamLease
  -> pass-through / remux / transcode as required
  -> Streaming Gateway / MediaSession
  -> client adapter
  -> platform playback engine
```

The planned product order is Recording playback first, then Live TV, then truthful seek/growing-Recording behavior. Remux and transcode are evidence-driven compatibility escalation, not default architecture.

## Broadcast Companion direction

Teletext and HbbTV are now explicit planned television-domain capabilities rather than accidental Legacy-OSD backlog.

- Teletext is modeled as service/page/subpage data, not primarily as an OSD screenshot.
- HbbTV is modeled as broadcast application discovery plus an authorized isolated application session/runtime, not as raw URL/JavaScript/key plugin control.
- Phase-65 MediaSession rules remain authoritative when Suite-owned media is involved.
- Legacy OSD follows later for genuinely opaque native/plugin workflows.

## Broad Timer UI ordering

A broad polished Timer UI is not required to close the Phase-64 engine and is not inserted as a numbered phase.

It is a cross-cutting product milestone gated on:

```text
Phase 62 identity/RBAC [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

The product UI must remain intent-first and preserve assignment, readback, reconciliation, failover and unknown-outcome semantics.

Phase 65 may intentionally start before the broad Timer UI is complete.

## Product acceptance

Component tests, CI, architecture guards and real-system safety checks remain mandatory. User-visible milestones additionally use [Golden User Journeys](../planning/golden-user-journeys.md).

The key vertical journeys now cover:

- Live TV playback;
- Recording playback;
- record-one-programme orchestration;
- multi-backend scheduling without provider knowledge;
- fail-closed recovery;
- Teletext browsing;
- HbbTV application launch;
- one explicit Legacy OSD compatibility workflow;
- broad Timer UI operation as a cross-cutting milestone.

## Development rules

- Root-level `AGENTS.md` is binding.
- `CURRENT.md` is the sole repository copy of volatile operational phase status.
- Verify live `main`, exact PR head and exact-final-head CI before writes or status claims.
- Keep review/merge/retarget/close state changes behind explicit user approval.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires them.
- Provider availability never creates execution authority and active work never silently switches provider.
- Require real-system acceptance when an installed/runtime, native, media or broadcast-behaviour boundary changes.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- Proposed ADR work does not authorize runtime implementation.
- Phase 65 is not active until explicitly started.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded edit safely. Read the complete file content required for the change, write a coherent commit on the intended branch and inspect the resulting diff before treating the update as correct.

Use local edits first only when the change requires local build/test execution, multi-file transformations that are materially safer in a checked-out worktree, binary/generated-file handling unavailable through the connector, or a connector workaround.

GitHub-first does not weaken review safety: keep updates fast-forward-only, do not replace a complete file from a truncated fetch, and do not mark Draft PRs Ready or merge them without explicit approval.

## Next authorization boundary

Before the first Phase-65 runtime change:

1. review live `main`;
2. re-read ADR-0046 and ADR-0053;
3. inspect the current media/provider code gap;
4. define the first coherent Recording-playback vertical and its acceptance boundary;
5. explicitly authorize Phase-65 kickoff.

Until then, Phase 65 remains not started.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Agent Workflow Rules](../../AGENTS.md)
