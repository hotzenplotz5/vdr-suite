# Phase 64 Slice 4 — TimerAssignment Persistence and Repository Semantics

## Scope

Phase 64 Slice 4 adds the smallest durable prerequisite after the TimerAssignment value contract: Control-Plane-owned persistence with exact optimistic concurrency, repository-issued assignment epochs and a durable single-active-primary boundary.

The slice remains below scheduling and native execution. It does not create scheduler decisions, NativeTimerBinding persistence, Agent commands, public APIs or native VDR Timer mutations.

Binding decisions:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)

## Ownership

`TimerAssignmentRepository` is Control-Plane domain persistence under `core/timers/`.

The repository owns:

- the durable `timer_assignments` schema;
- mapping between rows and the `TimerAssignment` domain contract;
- issuance of the initial and successor `assignmentRevision`;
- issuance of the next monotonic per-`timerIntentId` `assignmentEpoch`;
- exact compare-and-update optimistic-concurrency fencing;
- exact parent TimerIntent revision validation;
- durable single active primary enforcement;
- deterministic per-intent listing and active-primary lookup.

The repository does not own:

- backend scoring or scheduler policy;
- failover decisions;
- NativeTimerBinding creation or reconciliation;
- operation/job dispatch;
- authorization or public HTTP semantics;
- Agent or SuiteBridge execution;
- native Timer mutation.

## Creation contract

On create, the caller supplies:

- a stable `timerAssignmentId`;
- the owning `timerIntentId`;
- the exact observed `intentRevision`;
- assignment state, role and bounded decision evidence;
- backend/channel/capability/health evidence required by the value contract;
- creation time.

The caller must leave `assignmentRevision` empty and `assignmentEpoch` at zero.

Inside one `BEGIN IMMEDIATE` transaction the repository:

1. verifies the assignment ID is unused;
2. verifies the parent TimerIntent exists;
3. verifies the supplied `intentRevision` exactly matches the current durable TimerIntent revision;
4. issues `assignmentRevision = "1"`;
5. issues `assignmentEpoch = max(existing epoch for intent) + 1`;
6. normalizes `updatedAt` to `createdAt`;
7. validates the complete durable assignment value;
8. enforces the single active primary invariant;
9. inserts the row and commits.

The repository therefore issues both revision and assignment epoch. Concurrent writers cannot independently claim the same ownership epoch.

## Parent TimerIntent fence

Every create and update is checked against the current durable TimerIntent revision.

A stale assignment decision is returned as `intentRevisionConflict`; a missing parent is returned as `intentNotFound`.

The repository does not silently rewrite the assignment's `intentRevision` to current state. The caller must explicitly rebuild or reconcile its decision from current intent evidence.

## Single active primary

ADR-0044 defines these active ownership states:

```text
selected
provisioning
bound
reconciling
superseding
```

For role `primary`, at most one assignment for a TimerIntent may be in one of those states.

The invariant is enforced twice inside the write transaction:

- an explicit conflict check gives deterministic `ownershipConflict` semantics;
- a partial unique SQLite index is the final durable race fence.

`proposed` and `unassigned` are not active ownership states. Replicas and replacement assignments are not reclassified as primaries merely to bypass this invariant.

## Update contract

Updates require the exact `expectedRevision`, and the submitted assignment must carry the same observed `assignmentRevision`.

The repository rejects changes to:

- `timerAssignmentId`;
- owning `timerIntentId`;
- repository-issued `assignmentEpoch`;
- assignment role;
- `createdAt`;
- a previously selected non-empty backend identity.

A target-free `unassigned` assignment may later select its first backend through a valid domain transition. Reassignment to another backend requires a distinct assignment/handover decision rather than silently changing ownership in place.

State changes must satisfy `timerAssignmentCanTransition`. Terminal assignments are immutable. `updatedAt` must advance.

The repository alone issues the successor assignment revision, and the SQL update is fenced by both stable assignment ID and old revision.

## Native binding boundary

The repository stores `nativeTimerBindingId` only as value evidence already permitted by the TimerAssignment contract.

This slice does not create, persist or reconcile NativeTimerBinding objects. A `bound` assignment remains invalid unless native-binding identity is present, but the authoritative native readback that justifies that identity belongs to later slices.

## SQLite boundary

Direct SQLite remains restricted by ADR-0050. `TimerAssignmentRepository.cpp` qualifies through the existing `core/timers/src/*Repository.cpp` domain-repository rule.

The focused regression uses the generic `Database` abstraction with an in-memory SQLite database and does not use the SQLite C API directly.

## Regression contract

The focused repository regression proves:

- schema creation against an in-memory database;
- repository-issued initial revision and first per-intent epoch;
- durable round-trip of decision evidence;
- duplicate stable-ID rejection;
- rejection of caller-manufactured revisions and epochs;
- missing/stale parent TimerIntent outcomes;
- monotonic epochs for multiple assignments of one intent;
- deterministic per-intent listing;
- active-primary lookup;
- durable single active primary conflict;
- exact optimistic-concurrency conflict with current readback;
- immutable role, epoch, creation identity and selected backend identity;
- `bound` requiring native binding evidence;
- explicit refresh to a newer parent TimerIntent revision;
- terminal assignment immutability;
- replicas remaining distinct from the primary-owner constraint.

## Runtime boundary

This slice adds no installed TimerAssignment runtime path. In particular it adds no scheduler, no failover execution, no NativeTimerBinding persistence, no public Timer API, no Agent Timer command, no SuiteBridge Timer mutation command, no native Timer create/update/delete/toggle and no production reconciliation.

`mutations=enabled` remains forbidden.

## Acceptance

Acceptance for this repository-only slice is:

1. focused TimerAssignment contract regression remains green;
2. focused TimerAssignment repository regression passes against in-memory SQLite;
3. Slice-4 architecture guard passes;
4. repository-wide architecture checks pass;
5. complete hosted VDR-Suite CI is green on one exact Draft-PR head.

There is no real yaVDR acceptance requirement because this slice changes no installed runtime path, service process, Backend Agent execution or native VDR state.

## Deferred

The next Phase-64 slice may add deterministic assignment planning only after separate review. Native Timer mutation remains later and must continue through the protected-write, operation, job, generation-fencing and authoritative-readback contracts established before Phase 64.

Account/backend access management remains a hard prerequisite before broad Timer UI wiring and continues to build on the existing Phase-62 identity/RBAC model rather than creating another authorization system.
