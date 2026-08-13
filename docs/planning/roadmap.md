# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order and phase-completion gates. Exact active branch heads, PR tips and CI checkpoints belong only in [Current State](../CURRENT.md). Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md).

A roadmap entry is not automatic permission to implement the next possible diff. New runtime work requires a binding requirement, an accepted-code gap and the smallest **coherent** change that closes a real correctness, security or product need.

## Execution governance

- A chat discussion becomes a project decision only when represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessarily long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires the split.
- Technical CI and architecture guards remain mandatory. User-visible milestones additionally use [Golden User Journeys](golden-user-journeys.md).
- Provider reachability never creates authority and active work never silently switches provider.

## Current phase position

```text
Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Next strict numbered runtime phase after Phase 64:
Phase 65 - Streaming Gateway and Media Sessions

Current implementation boundary:
planning hold after the PR-#190 checkpoint; Phase 64 is not complete
```

The implementation hold authorizes planning/documentation synchronization only. It does not authorize a Phase-64 successor implementation or imply a `#191`.

## Completed prerequisites

### Phase 61 - Suite Metadata and Genre Platform

Status: **Completed.**

Persistent backend-scoped Recording/EPG metadata, people relations, canonical Genre assignments, indexed query-only browse paths and frontend integration.

Completed non-numbered work includes Post-Phase 61 Performance Hardening (B1-B4), VDR Remote and Live Overlay hardening (#110), Backend-scoped Global Search (#111) and Configurable photorealistic VDR Remote (#115).

Historical umbrella implementation track: Phase 58 - Frontend and Live Parity.

### Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Completed.**

Persistent identities, exact backend-scoped authorization, browser-session/CSRF protection and append-only accountability are established. Legacy Basic compatibility remains transitional; removal requires a separate deployment-migration contract.

See [Phase 62 Final Closeout](../development/phase-62-closeout.md) and [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md).

### Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Completed.**

Phase 63 established secure Agent enrollment/identity, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit provider ownership/selection and the generic protected-write safety contract.

Historical Phase-63 documents retain their original checkpoint evidence. Historical sentences saying **Phase 63 is not complete** do not describe current status.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Active; implementation paused for planning review after the PR-#190 checkpoint.**

Binding architecture: [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md).

Phase 64 separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

and builds deterministic scheduling, safe managed native fulfillment, authoritative readback and reconciliation around those durable identities.

### Phase 64 Slice 1 — TimerIntent Domain Contract

Status: **Completed.** PR #150 established stable backend-neutral TimerIntent identity, revision, semantic identity, intent types and lifecycle semantics.

### Phase 64 Slice 2 — TimerIntent Persistence and Repository Semantics

Status: **Completed.** PR #152 established Suite-owned TimerIntent persistence, repository-issued revisions and optimistic-concurrency fencing.

Historical Slice-2 boundary: No TimerAssignment; no NativeTimerBinding; no scheduler or failover execution; no production native Timer mutation.

### Current stacked checkpoint

After the merged TimerIntent foundation, the stacked Draft line has established TimerAssignment planning/persistence, NativeTimerBinding/readback evidence, durable mutation-operation state, Timer-delete preparation/delivery/local-state safety and a concrete private SuiteBridge Timer-delete transport.

The PR-#190 transport remains deliberately disabled. Its existence does not enable production native Timer deletion and does not complete Phase 64. Exact stacked PR/head/CI state is maintained in `CURRENT.md`.

### Phase-64 completion gate

Phase 64 closes on the reliable Timer **engine**, not on a broad polished Timer UI. Completion requires coherent proof of the applicable ADR-0044 lifecycle, including:

- durable TimerIntent, TimerAssignment and NativeTimerBinding identity/revision semantics;
- deterministic eligible-backend selection and explicit primary/replica ownership;
- fenced managed native Timer create/update/delete execution;
- durable no-blind-retry handling and unknown-outcome reconciliation;
- authoritative post-operation readback, including complete-inventory absence proof for delete;
- drift/reconciliation behavior that distinguishes expected transitions from external or ambiguous change;
- safe controlled reassignment/handover without unintended overlapping primary ownership;
- real-VDR acceptance of enabled writes with configuration, identity and service restoration;
- regression proof that stale revisions, generations, provider epochs and concurrent writers fail closed.

ADR-0044 remains authoritative where it is more specific.

### Broad Timer UI gate

A broad polished Timer UI is **not** a Phase-64 completion requirement. Broad Timer mutation controls remain separately gated on account/backend access management built on Phase 62.

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Planned after Phase-64 engine completion; broad Timer UI is not a prerequisite.**

Accepted server architecture: [ADR-0046: Streaming Gateway and Media Session Boundary](../adr/ADR-0046-streaming-gateway-media-session-boundary.md).

Draft PR #156 contains the complementary proposed client-playback/media-adaptation strategy and must be reviewed against current canonical planning before acceptance. The intended direction is:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Transformation preference is `pass-through -> remux/repackage -> transcode`. Streamdev may be a private explicitly owned provider, but is not the public API, universal platform dependency or implicit fallback.

The first Phase-65 vertical proof should reach one first-party client through authorized MediaSession/Gateway contracts with deterministic cleanup and no client-visible provider URL. Recording pass-through is a suitable first proof when it minimizes native live-receiver complexity; live pass-through and deterministic receiver cleanup follow immediately.

## Phase 66 — Legacy OSD Compatibility Bridge

Status: **Planned after Phase 65.**

Isolated immutable OSD snapshots/deltas, resynchronization, one fenced controller lease and allowlisted input.

## Phase 67 — Public API and Client Compatibility Hardening

Status: **Planned after Phase 66.**

Versioned discovery, compatible errors, resource-specific revisions/preconditions, durable operations where required, pagination and migration aliases.

## Phase 68 — Recommendation and Content Knowledge Graph

Status: **Later vision.**

Requires stable metadata/provenance, actor privacy, stable identities, mature accountability and public API contracts.

## Cross-cutting completion gates

- **Identity:** stable Suite identity and explicit native binding.
- **Provider:** provider facts carry provenance and never become hidden authority.
- **Mutation:** authentication/authorization, required preconditions, durable idempotency/starting state, dispatch evidence, verification and accountability.
- **Native boundary:** no raw VDR pointer/lock crosses async, network or database work.
- **Client:** clients consume Suite contracts, never private provider details.
- **Acceptance:** focused tests, regressions, build/package validation and real-system proof where runtime behaviour changes.
- **Product:** relevant [Golden User Journeys](golden-user-journeys.md) for user-visible milestone claims.

## Exact next action

Do not start another Timer implementation slice from the PR-#190 successor note. Complete project-truth synchronization, preserve the Streaming-before-broad-Timer-UI sequencing decision, review Draft PR #156 against the synchronized architecture, then explicitly decide what remaining Phase-64 engine work is required before implementation resumes.

## Related documents

- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [ADR-0044 Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
