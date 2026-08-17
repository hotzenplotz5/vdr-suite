# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order and phase-completion gates. Exact active branch heads, PR tips and transient CI checkpoints belong only in [Current State](../CURRENT.md). Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md).

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
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
none - Phase 65 has not started

Next strict numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions
```

Phase 64 is complete. The durable evidence is maintained in [Phase 64 Closeout](../development/phase-64-closeout.md) and the exact current checkpoint in [Current State](../CURRENT.md).

## Completed prerequisites

### Phase 61 - Suite Metadata and Genre Platform

Status: **Completed.**

Persistent backend-scoped Recording/EPG metadata, people relations, canonical Genre assignments, indexed query-only browse paths and frontend integration.

### Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Completed.**

Persistent identities, exact backend-scoped authorization, browser-session/CSRF protection and append-only accountability are established. Legacy Basic compatibility remains transitional; removal requires a separate deployment-migration contract.

### Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Completed.**

Phase 63 established secure Agent enrollment/identity, generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit provider ownership/selection and the generic protected-write safety contract.

Historical exact guard spelling retained for traceability: `Phase 63 - Backend Agent and Secure Multi-Site Runtime`.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Completed.**

Binding architecture: [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md).

Phase 64 established the durable separation:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

and completed deterministic scheduling, safe managed native fulfillment, authoritative readback, reconciliation and controlled reassignment/failover around those identities.

### Completed Phase-64 capability boundary

The accepted engine provides:

- durable backend-neutral TimerIntent identity and optimistic-concurrency semantics;
- TimerAssignment persistence, primary/replica ownership and deterministic eligible-backend selection;
- NativeTimerBinding persistence and canonical observed-state evidence;
- managed native Timer create/update/toggle/delete execution through the Agent/SuiteBridge boundary;
- durable mutation-operation state, dispatch fencing and no-blind-retry semantics;
- authoritative PRESENT and ABSENCE verification;
- fail-closed handling of stale generations, providers, revisions, bindings, fingerprints and operation evidence;
- controlled replacement only before native dispatch or after exact verified absence;
- atomic old-owner supersession, replacement creation and durable reassignment evidence;
- exact replay without duplicate exclusive owners;
- bundled real yaVDR acceptance on the exact final candidate.

### Phase-64 completion gate — satisfied

The completion gate required coherent proof of the ADR-0044 lifecycle, including safe managed native mutation, durable unknown-outcome handling, authoritative readback, reconciliation, controlled reassignment and real-system acceptance. Those requirements were satisfied by the final accepted candidate and merged through PR #195.

See [Phase 64 Closeout](../development/phase-64-closeout.md).

### Historical Slice 1-3 traceability anchors

The following strings describe **historical intermediate Phase-64 boundaries only**. They are retained because early-slice architecture guards verify that those original contracts remain documented; they are not current project status or implementation authority.

- `Phase 64 Slice 1 — TimerIntent Domain Contract`
- `Phase 64 Slice 2 — TimerIntent Persistence and Repository Semantics`
- `Phase 64 Slice 3 — TimerAssignment Domain Contract`
- `Status: **Active; Slice 3 is the TimerAssignment domain contract.**`
- `No TimerAssignment persistence; no NativeTimerBinding; no scheduler or failover execution`
- `Account/backend access management is a hard prerequisite before broad Timer UI wiring`

Those statements must be read as historical slice boundaries; later accepted Phase-64 slices superseded their implementation limitations without rewriting the historical slice contracts.

### Broad Timer UI gate

A broad polished Timer UI is **not** a Phase-64 completion requirement. Broad Timer mutation controls remain separately gated on account/backend access management built on Phase 62.

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Next; not started.**

Accepted server architecture: [ADR-0046: Streaming Gateway and Media Session Boundary](../adr/ADR-0046-streaming-gateway-media-session-boundary.md).

Before the first runtime implementation slice, reconcile the existing playback/media-adaptation planning with the now-completed Phase-64 platform and re-read live repository state. Do not treat old draft PR numbers or historical planning checkpoints as automatic implementation authority.

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

Transformation preference is `pass-through -> remux/repackage -> transcode`. Streamdev may be a private explicitly owned provider, but is not the public API, universal platform dependency or implicit fallback.

The first Phase-65 vertical proof should reach one first-party client through authorized MediaSession/Gateway contracts with deterministic cleanup and no client-visible provider URL. Recording pass-through is a suitable first proof when it minimizes native live-receiver complexity; live pass-through and deterministic receiver cleanup follow under explicit scope.

Phase 65 begins only after an explicit kickoff decision. This roadmap does not itself authorize runtime changes.

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

Land the Phase-64 closeout/status synchronization. Then perform a bounded Phase-65 architecture and scope review against ADR-0046, current platform capabilities and Golden User Journeys. Only after that review should the first Phase-65 runtime slice be explicitly authorized.

## Related documents

- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Phase 64 Closeout](../development/phase-64-closeout.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [ADR-0044 Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
