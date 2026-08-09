# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order. Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md).

A roadmap item is not automatically an implementation requirement. New runtime work requires a binding requirement, a concrete accepted-code gap, a real failure/security consequence and the smallest closing change.

## Current verified position

Baseline: `main @ cb6f56e28bc981c8a3c86605fd8e842df4a86ab3`.

```text
Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Previous completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current bounded slice:
Phase 64 Slice 3 - TimerAssignment Domain Contract
Contract-only; no TimerAssignment persistence, scheduler or native Timer mutation
```

## Completed prerequisites

### Phase 61 - Suite Metadata and Genre Platform

Status: **Completed.**

Persistent backend-scoped Recording/EPG metadata, people relations, canonical Genre assignments, indexed query-only browse paths and frontend integration.

### Post-Phase-61 hardening and platform features

Status: **Completed, non-numbered.**

- Performance Hardening B1-B4;
- VDR Remote and Live Overlay hardening (#110);
- Backend-scoped Global Search (#111);
- Configurable photorealistic VDR Remote (#115).

## Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Completed.**

Goal achieved: production-grade actor identity, scoped server-side authorization, browser-session lifecycle security and append-only accountability.

Accepted runtime includes persistent identity, exact grants and roles, protected central POSTs, browser-session lifecycle and CSRF policy, append-only pre-dispatch evidence, browser lifecycle outcomes and protected mutation success/failure outcomes.

Final runtime evidence:

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

### Compatibility-retirement decision

Legacy Basic compatibility remains transitional and is retained at closeout. Immediate removal is not deployment-ready because `legacy-basic` remains the code default and packaged deployments do not yet mandate migration to `enforced`.

Removal requires a separate future migration contract. This explicit decision satisfies the Phase-62 exit criterion and does not authorize another Phase-62 slice.

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Completed.**

Phase 63 established the security and execution foundation required before multi-backend orchestration:

- Backend Agent enrollment, device identity, lease, generation and credential lifecycle;
- generation-/sequence-fenced read-only Observation and Snapshot Ingestion;
- explicit Channel observation from `channels.conf` without hidden source fallback;
- durable Agent command assignment, receipt, result and reconciliation state;
- side-effect-free fenced SuiteBridge native execution with plugin-instance epoch and authoritative readback;
- explicit local-provider facts, ownership and immutable provider selection with no silent fallback;
- a generic protected-write safety contract covering durable idempotency, resource-scoped leases, authority/provider fences, expected resource revisions, unknown-outcome reconciliation and authoritative readback requirements.

The final Phase-63 stack was merged through PRs #147, #148 and #149. The verified phase boundary is:

```text
main commit: 40d30c3e83f60688a75bf48bb3c5970b382e336c
main tree:   c88c2d5fc76bdd008202b2c36e4fc420f583e6ca
```

The provider-selection runtime was accepted on real yaVDR with VDR native state unchanged, Agent identity/credential generation preserved, original configuration restored and VDR, daemon and Agent active. The final protected-write slice remained contract-only and did not enable production mutation.

Phase 63 deliberately did not introduce TimerIntent, TimerAssignment, NativeTimerBinding, a multi-backend scheduler or a production native Timer write.

### Historical Phase-63 observation checkpoint

The earlier read-only observation milestone remains part of the verified Phase-63 history and is retained here because its regression guard binds these provenance markers. Phase 63 Slice 1 established the Agent enrollment foundation. Phase 63 Slice 2, **Observation and Snapshot Ingestion**, was contract-merged in **PR #138** as `24b1d7938ddaa15834a8da6323a270761868f4ba`; **PR #139** then implemented the first bounded `backend-health` runtime.

At that historical checkpoint, before the later command, native-operation and provider-selection slices were completed, the correct status statement was: **Phase 63 is not complete**. That sentence is historical evidence only; the current phase status above is authoritative and records Phase 63 as completed.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Active; Slice 3 is the TimerAssignment domain contract.**

Phase 64 separates TimerIntent, TimerAssignment and NativeTimerBinding, then adds deterministic scheduling/reconciliation, readback, drift handling and uncertain-dispatch recovery in bounded slices.

### Phase 64 Slice 1 — TimerIntent Domain Contract

Status: **Completed.**

Binding architecture: [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md).

Slice 1 established the Control-Plane-owned, backend-neutral TimerIntent value contract:

- stable Suite `timerIntentId` separate from backend-native Timer identity;
- opaque `intentRevision` for exact optimistic-concurrency fencing;
- versioned collision-safe semantic identity for exact equivalence evidence;
- canonical `programme_event`, `manual_window` and `recurring_schedule` intent types;
- canonical durable lifecycle states and fail-closed transition rules;
- bounded owner, automation-source, event, channel, schedule, recording-option, assignment-policy, replica-policy and duplicate-policy values;
- explicit separation from SearchTimer definitions and existing direct native Timer action paths.

Slice 1 was squash-merged through PR #150 as `eeec518de1e7fef4c452390c608d1a4316a1fa52`; its post-merge VDR-Suite CI #7384 (`31316807398`) completed successfully.

### Phase 64 Slice 2 — TimerIntent Persistence and Repository Semantics

Status: **Completed.**

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)

Slice 2 added the durable prerequisite needed before assignment and scheduling:

- Suite-owned `timer_intents` persistence under `core/timers/`;
- repository-issued opaque revision tokens backed by monotonic durable revisions;
- exact compare-and-update optimistic-concurrency fencing;
- stale-write conflict with current durable readback;
- immutable creation provenance and terminal TimerIntent state;
- exact semantic-identity lookup evidence without turning equivalence into an unconditional uniqueness rule;
- narrow SQLite permission for Timer-domain `*Repository.cpp` implementations only;
- in-memory SQLite regression coverage integrated into the fast test graph.

Slice 2 was squash-merged through PR #152 as `cb6f56e28bc981c8a3c86605fd8e842df4a86ab3`; VDR-Suite CI #7385 (`31317471713`) completed successfully on its final PR head.

### Phase 64 Slice 3 — TimerAssignment Domain Contract

Status: **Active contract-only slice.**

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)

This slice defines only the backend-neutral durable assignment value semantics required before persistence and scheduling:

- stable `timerAssignmentId`, opaque `assignmentRevision` and exact owning `timerIntentId`/`intentRevision` relationship;
- monotonic `assignmentEpoch` as future ownership-fencing input;
- canonical assignment lifecycle and `primary`, `replica`, `replacement` roles;
- backend generation, channel mapping, capability and health evidence;
- bounded decision-policy evidence;
- explicit target-free `unassigned` representation;
- `bound` requiring native-binding identity evidence;
- exact active-ownership-state classification for later single-primary-owner persistence invariants.

No TimerAssignment persistence; no NativeTimerBinding; no scheduler or failover execution; no public Timer API; no Agent Timer command; no native Timer mutation.

### Frontend access-management gate

Account/backend access management is a hard prerequisite before broad Timer UI wiring.

The implementation must build on the completed Phase-62 persistent actors, credentials, browser sessions and backend-scoped permission grants. It must not introduce a second user or authorization model. User onboarding, manageable backend membership/roles and backend sharing may be delivered as bounded security slices before TimerIntent/TimerAssignment mutation controls are exposed broadly in the frontend.

This frontend gate does not block the current backend-neutral Phase-64 domain slices and does not weaken server-side authorization.

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Planned after Phase 64.**

Authorized short-lived media sessions, Gateway-owned connections, capacity leases, Live/Recording pass-through and private provider routing.

## Phase 66 — Legacy OSD Compatibility Bridge

Status: **Planned after Phase 65.**

Immutable OSD snapshots/deltas, resynchronization, one fenced controller lease and allowlisted input.

## Phase 67 — Public API and Client Compatibility Hardening

Status: **Planned after Phase 66.**

Versioned discovery, compatible errors, resource-specific revisions/preconditions, durable operations where required, pagination and migration aliases.

## Phase 68 — Recommendation and Content Knowledge Graph

Status: **Later vision.**

Requires stable metadata/provenance, actor privacy, stable identities, mature accountability and public API contracts.

## Cross-cutting completion gates

- **Identity gate:** stable Suite identity and explicit native binding.
- **Provider gate:** provider facts carry provenance and never become hidden authority.
- **Mutation gate:** authentication, CSRF where applicable, authorization, required preconditions, dispatch evidence, verification and accountability.
- **Native boundary gate:** no raw VDR pointer/lock crosses async, network or database work.
- **Client gate:** clients consume Suite contracts, never private provider details.
- **Acceptance gate:** focused tests, regressions, build/package validation and real-system proof where runtime behaviour changes.

## Phase-64 Slice-3 hard exclusions

No TimerAssignment persistence; no NativeTimerBinding; no scheduler or failover execution; no SearchTimer direct execution; no public TimerIntent/TimerAssignment API; no Agent Timer command; no SuiteBridge Timer mutation command; no native Timer create/update/delete/toggle; no production reconciliation; no `mutations=enabled`; no Phase-65-or-later runtime.

## Exact next action

Stabilize the Phase-64 TimerAssignment domain-contract Draft PR on one exact head and obtain the complete repository CI graph. Because this slice changes no installed runtime path, do not require yaVDR installation or service restart. Keep the PR Draft until explicit approval.

## Related documents

- [Current State](../CURRENT.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Phase Map](phase-map.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 63 Slice-1 Closeout](../development/phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](../development/phase-63-observation-ingestion.md)
- [Phase 64 TimerIntent Contract](../development/phase-64-timer-intent-contract.md)
- [Phase 64 TimerIntent Repository](../development/phase-64-timer-intent-repository.md)
- [Phase 64 TimerAssignment Contract](../development/phase-64-timer-assignment-contract.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
