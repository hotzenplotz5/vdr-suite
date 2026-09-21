# Phase 64 Slice 2 — TimerIntent Persistence and Repository Semantics

## Scope

Phase 64 Slice 2 adds the smallest durable prerequisite after the merged TimerIntent domain contract: Suite-owned TimerIntent persistence with exact optimistic-concurrency semantics.

The slice remains below scheduling and native execution. It does not create TimerAssignments, NativeTimerBindings, scheduler decisions, Agent commands, public APIs or native VDR Timer mutations.

Binding decisions:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)

## Ownership

`TimerIntentRepository` is Control-Plane domain persistence under `core/timers/`.

The repository owns:

- the `timer_intents` durable schema;
- mapping between durable rows and the `TimerIntent` domain contract;
- issuance of the initial and successor `intentRevision` values;
- exact compare-and-update semantics for optimistic concurrency;
- exact semantic-identity lookup evidence;
- durable rejection of duplicate stable TimerIntent IDs.

The repository does not own:

- duplicate policy decisions;
- assignment or backend selection;
- scheduling or failover;
- operation/job dispatch;
- authorization or public HTTP semantics;
- native Timer identity or mutation.

## Revision contract

`intentRevision` remains opaque to callers even though this repository stores a monotonic integer internally.

Creation rules:

1. the caller supplies no revision token;
2. the repository validates the complete TimerIntent domain value;
3. the durable row is created at revision `1`;
4. `updatedAt` is normalized to `createdAt` for the creation event;
5. an existing `timerIntentId` is rejected as `alreadyExists`.

Update rules:

1. the caller supplies the exact revision it observed as `expectedRevision`;
2. the submitted TimerIntent must carry that same observed revision, not a manufactured successor;
3. the repository opens an immediate transaction and reads the current durable row;
4. a revision mismatch returns `conflict` together with current durable state;
5. immutable creation identity and timestamp fields cannot change;
6. terminal TimerIntent states are immutable in this slice;
7. lifecycle changes must satisfy `timerIntentCanTransition`;
8. `updatedAt` must advance;
9. only the repository issues the successor revision;
10. the SQL update is fenced by both stable TimerIntent ID and the old revision.

A stale writer therefore cannot silently overwrite a newer TimerIntent.

## Semantic identity

The repository stores the versioned `timerIntentSemanticIdentity()` value alongside each row and indexes it for exact lookup.

This is equivalence evidence, not a uniqueness constraint. Two distinct TimerIntent IDs may have the same semantic identity because the final duplicate-policy decision belongs above persistence and may later require operator review, intentional replicas or automation-source policy.

Every read recomputes the semantic identity from the decoded domain value. A row whose stored identity no longer matches the domain contract is treated as storage failure rather than trusted silently.

## SQLite boundary

Direct SQLite remains restricted by ADR-0050.

For the Timer domain, only implementation files matching:

```text
core/timers/src/*Repository.cpp
```

may use the SQLite C API. `tools/check_architecture.py` contains positive and negative self-checks so similarly located services, schedulers, persistence helpers and generic helpers remain rejected.

The repository regression uses the generic `Database` abstraction with an in-memory SQLite database. The test itself does not require direct SQLite access.

## Regression contract

The focused repository regression proves:

- schema creation against an in-memory database;
- complete durable round-trip of TimerIntent fields used by Slice 1;
- repository-owned initial revision;
- stable ID uniqueness;
- exact semantic-equivalence lookup without forced deduplication;
- successful revision-controlled update and successor revision;
- stale update conflict with current durable readback;
- rejection of caller-manufactured revisions;
- lifecycle transition enforcement;
- terminal-state immutability;
- not-found and invalid-input outcomes.

The existing Slice-1 TimerIntent value regression remains in the fast test graph.

## Runtime boundary

This slice introduces no installed TimerIntent runtime path. In particular it adds no:

- TimerAssignment;
- NativeTimerBinding;
- multi-backend scheduler or failover execution;
- SearchTimer direct execution path;
- REST or frontend TimerIntent endpoint;
- Backend Agent Timer command;
- SuiteBridge/RESTfulAPI/SVDRP Timer mutation command;
- native Timer create/update/delete/toggle;
- production reconciliation loop;
- `mutations=enabled` mode.

The Phase-64 architecture guard continues to reject `TimerIntent` production wiring outside `core/timers/` while these later slices remain separately reviewable.

## Acceptance

Acceptance for this repository-only slice is:

1. focused TimerIntent domain regression passes;
2. focused TimerIntent repository regression passes against in-memory SQLite;
3. Phase-64 architecture guard passes;
4. repository-wide architecture checks pass;
5. the complete hosted VDR-Suite CI graph is green on one exact Draft-PR head.

There is no real yaVDR acceptance requirement for this slice because it adds no daemon wiring, installed service path, Backend Agent execution, SuiteBridge execution or native VDR Timer mutation.

## Deferred

The next Phase-64 slices may add TimerAssignment persistence and deterministic scheduling only after separate review. Native Timer mutation remains later and must use the protected-write, operation, job, generation-fencing and authoritative-readback contracts established before Phase 64.
