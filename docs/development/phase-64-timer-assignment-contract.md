# Phase 64 Slice 3 — TimerAssignment Domain Contract

## Scope

Phase 64 Slice 3 adds the smallest backend-neutral scheduling contract after the merged TimerIntent value and persistence slices: the durable value semantics of a TimerAssignment.

This slice is contract-only. It does not add TimerAssignment persistence, scheduler selection, failover execution, NativeTimerBinding persistence, Agent Timer commands, public Timer APIs or native VDR Timer mutation.

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)

## Ownership

A `TimerAssignment` is a Control-Plane-owned durable scheduler decision. It is distinct from both the owning `TimerIntent` and any backend-native Timer.

The contract carries:

- stable `timerAssignmentId` and opaque `assignmentRevision`;
- owning `timerIntentId` and the exact `intentRevision` used by the decision;
- monotonic `assignmentEpoch` for ownership fencing;
- target `backendId` and `backendGeneration`;
- canonical assignment lifecycle state and assignment role;
- selected backend-native channel plus mapping provenance;
- capability and backend-health revision evidence;
- scheduler policy version plus bounded decision reasons, warnings, exclusions, conflict facts and score;
- optional `nativeTimerBindingId` once authoritative readback proves a native binding;
- durable creation and update timestamps.

## Lifecycle

Canonical states are:

```text
proposed
selected
provisioning
bound
reconciling
unassigned
superseding
superseded
cancel_requested
cancelled
failed
```

The contract rejects direct transitions that would skip required scheduling, provisioning or reconciliation states. Terminal assignments do not reactivate.

The active ownership states are exactly:

```text
selected
provisioning
bound
reconciling
superseding
```

This classification is the domain prerequisite for the later durable single-primary-owner invariant. The cross-assignment uniqueness constraint belongs to TimerAssignment persistence, not to this standalone value object.

## Backend and channel evidence

Every assignment except `unassigned` identifies one backend generation and carries complete channel, capability, health and decision-policy evidence.

An `unassigned` value deliberately carries no target backend, generation, channel binding, target capability revision, target health revision or native binding. It still carries the TimerIntent relationship, assignment epoch, decision policy and bounded evidence explaining why no backend is currently eligible.

Channel binding is all-or-nothing for target assignments. A canonical channel ID may be absent when canonical mapping does not exist, but backend-native channel identity, mapping source and mapping revision are required.

## Native binding boundary

`bound` requires a non-empty `nativeTimerBindingId`. This value is only an identity reference. Slice 3 does not define `NativeTimerBinding`, does not perform readback and cannot manufacture binding success.

Later runtime code must only enter `bound` after authoritative native readback under ADR-0044 and the protected-write contracts.

## Roles and replica boundary

Canonical roles are:

```text
primary
replica
replacement
```

The value contract can represent these roles but does not decide whether a replica is permitted. The owning TimerIntent replica policy and later repository/scheduler invariants must authorize multiple active assignments deliberately; accidental duplicates are not converted into replicas.

## Security and frontend sequencing

This slice does not add frontend Timer management or account-management endpoints.

Before a broad Timer UI is connected to TimerIntent or TimerAssignment mutations, VDR-Suite must expose manageable user/backend access on top of the existing Phase-62 actor, credential, session and backend-scoped grant foundations. Account management must reuse those identities and grants rather than create a second authentication or authorization system.

That frontend prerequisite does not weaken the Phase-64 backend-domain sequence and is not an authorization bypass: server-side authorization remains authoritative.

## Regression contract

The focused TimerAssignment regression proves:

- canonical state and role names;
- stable identity, revision, TimerIntent linkage and assignment-epoch validation;
- backend/generation/channel/capability/health evidence for target assignments;
- the explicit target-free `unassigned` shape;
- bounded decision evidence;
- `bound` requiring a native binding identity;
- canonical lifecycle transitions;
- exact active-ownership-state classification;
- terminal-state classification;
- revision-token comparison.

## Runtime boundary

This slice adds no installed runtime path and no:

- TimerAssignment repository or SQLite schema;
- scheduler or failover decision engine;
- NativeTimerBinding contract or persistence;
- SearchTimer execution change;
- REST or frontend TimerIntent/TimerAssignment endpoint;
- Backend Agent Timer command;
- SuiteBridge/RESTfulAPI/SVDRP Timer mutation;
- native Timer create/update/delete/toggle;
- production reconciliation loop;
- `mutations=enabled` mode.

`mutations=disabled` therefore remains unchanged.

## Acceptance

Acceptance for this contract-only slice is:

1. existing TimerIntent domain and repository regressions remain green;
2. focused TimerAssignment domain regression passes;
3. the Phase-64 architecture guard rejects premature TimerAssignment runtime wiring;
4. repository-wide architecture checks pass;
5. the complete hosted VDR-Suite CI graph is green on one exact Draft-PR head.

There is no real yaVDR acceptance requirement for this slice because it adds no daemon wiring, installed service path, Backend Agent execution, SuiteBridge execution or native VDR Timer mutation.

## Deferred

The next bounded Phase-64 slice may add TimerAssignment persistence with exact revision, epoch and single-primary-owner invariants. Deterministic scheduling, NativeTimerBinding, reconciliation and native mutation remain separate later slices.
