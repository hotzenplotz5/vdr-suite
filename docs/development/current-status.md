# VDR-Suite Current Project Status

## Status ownership

Exact operational state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second source for current branch heads, PR tips or CI run numbers.

Before continuing work, always read `CURRENT.md`, the Strict Roadmap, the applicable ADRs and live GitHub state.

## Platform position

Latest completed numbered runtime phase: **Phase 63 - Backend Agent and Secure Multi-Site Runtime**.

Current active numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.

Next strict numbered runtime phase after Phase 64: **Phase 65 - Streaming Gateway and Media Sessions**.

Historical completed context remains relevant, including Phase 58 - Frontend and Live Parity, Phase 61 - Suite Metadata and Genre Platform, Post-Phase 61 Performance Hardening (B1-B4), VDR Remote and Live Overlay hardening (#110), Backend-scoped Global Search (#111), Configurable photorealistic VDR Remote (#115), and Phase 62 - Identity, RBAC and Accountability Foundation.

Historical closeouts:

- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)

## Phase 63 foundation

Phase 63 is complete. It established Agent enrollment/identity, protected transport, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit local-provider ownership/selection and the generic protected-write safety contract. These remain prerequisites for Phase-64 orchestration and later media execution.

Historical Phase-63 documents may contain checkpoint sentences such as **Phase 63 is not complete**. Those sentences describe their original intermediate acceptance point and are not current project status.

## Phase 64 position

Phase 64 deliberately separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The merged foundation on `main` is the TimerIntent contract and TimerIntent repository. A large stacked Draft line has subsequently built deterministic assignment planning, binding/readback evidence, durable operation semantics and a fenced Timer-delete handoff through a disabled concrete SuiteBridge transport.

The exact stacked checkpoint is maintained in `CURRENT.md` and must be re-read from GitHub before use.

The important architectural result is that the Timer-delete path at the current checkpoint is fail-closed: the transport exists, but production native deletion is not enabled merely because the wire path exists. Native mutation still requires the complete accepted replay/idempotency, provider/generation, durable-starting, readback/reconciliation and real-system gates.

## Current planning hold

Implementation is intentionally paused after the PR-#190 checkpoint while project truth and forward ordering are synchronized. This is **not** a declaration that Phase 64 is complete and does not authorize a successor Timer implementation or `#191`.

The current authorized work is documentation and architecture review only. A later implementation restart requires an explicit decision after that review.

## Streaming and Timer UI ordering

The intended strict order is:

```text
Phase 64 reliable Timer orchestration engine
  -> Phase 65 Streaming Gateway and Media Sessions
  -> Phase 66 Legacy OSD Compatibility Bridge
  -> Phase 67 Public API and Client Compatibility Hardening
```

A broad polished Timer UI is not required to close the Phase-64 engine. It remains separately gated on account/backend access management built on the Phase-62 identity/authorization model.

Therefore Phase 65 may intentionally start before the broad Timer UI is complete, but only after the reliable Phase-64 Timer engine satisfies its own completion gates. Streaming is not technically dependent on the broad Timer UI.

## Streaming architecture preparation

Accepted ADR-0046 already defines the server-side Streaming Gateway and MediaSession boundary. Draft PR #156 contains the complementary proposed client-playback/media-adaptation strategy and should be reviewed against the current canonical documents before acceptance.

The intended media direction remains provider-private and transformation-minimal: clients request Suite media capabilities rather than Streamdev/SuiteBridge URLs, and delivery prefers pass-through before remux/repackage before transcoding.

## Product acceptance

Component tests, CI, architecture guards and real-system safety checks remain mandatory. User-visible milestones additionally use [Golden User Journeys](../planning/golden-user-journeys.md) so that a technically correct collection of components is not mistaken for a complete product path.

The key vertical journeys cover Live TV playback, Recording playback, record-one-programme orchestration, multi-backend scheduling without provider knowledge and fail-closed recovery from backend/provider/transport failures.

## Development rules

- Root-level `AGENTS.md` is binding.
- `CURRENT.md` is the sole repository copy of volatile operational status.
- Verify live `main`, exact PR head and exact-final-head CI before writes or status claims.
- Keep active stacked PRs Draft unless the user explicitly approves a review-state change.
- Do not merge, rebase, force-push, retarget or rewrite published history without explicit approval.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires them.
- Provider availability never creates execution authority and active work never silently switches provider.
- Require real-system acceptance when an installed/runtime or native-behaviour boundary changes.
- Broad Timer UI work must not bypass the account/backend access-management gate.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connected GitHub tooling can perform the complete bounded operation safely. Re-read the exact pull-request head immediately before every repository write and inspect the resulting diff before treating the change as correct.

Use local edits first only when the change requires:

- a programmatic transformation that cannot be performed safely through the GitHub connector;
- local build, test or generated-artifact work that requires a checkout;
- a workaround because the GitHub connector blocks a file operation.

Do not replace a complete existing file from a truncated or partial fetch. Fetch the complete content or the required ranges first. Continue through already-approved bounded steps without artificial confirmation pauses, keep published updates fast-forward-only, and evaluate GitHub Actions at the final stabilization head rather than after every intermediate commit.

## Project-decision rule

A chat discussion is not a binding VDR-Suite project decision until it is represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract. This prevents one conversation or stale handoff from silently overriding the project architecture.

## Exact next action

Use [Current State](../CURRENT.md) for the exact checkpoint. Complete the documentation/roadmap synchronization, review proposed playback ADR PR #156 against that synchronized truth, then explicitly decide what remaining Phase-64 engine work is required before implementation resumes.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Phase 64 TimerIntent Contract](phase-64-timer-intent-contract.md)
- [Phase 64 TimerIntent Repository](phase-64-timer-intent-repository.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Agent Workflow Rules](../../AGENTS.md)
