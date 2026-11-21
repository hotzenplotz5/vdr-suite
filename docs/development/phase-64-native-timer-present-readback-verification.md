# Phase 64 — Operation-aware Native Timer Present Readback Verification

## Purpose

This slice adds the first operation-aware **present** native Timer verification
service. It combines three already-established authorities:

- the current durable `NativeTimerBinding`;
- one valid `NativeTimerReadbackExpectation`;
- one authoritative `NativeTimerObservation`.

Executor success alone is not verification. A present readback verifies the
operation only when all identity, revision, generation, time and normalized
state fences match.

## Exact verification order

`NativeTimerPresentReadbackVerificationService::verify()`:

1. validates the expectation and observation contracts;
2. loads the binding only by `nativeTimerBindingId`;
3. requires exact expectation/binding and observation/expectation native
   identity;
4. permits only `managed` or `adopted` bindings;
5. requires the observation generation to equal the operation expectation and
   rejects a binding already observed in a newer generation;
6. requires `observation.observedAt >= expectation.readbackNotBefore`;
7. requires the observed normalized fingerprint to equal the expected
   fingerprint;
8. only after those current-observation fences, recognizes an already durable
   verification of the same `operationId`;
9. for a first verification, requires exact `expectedBindingRevision`;
10. rejects an observation older than the current durable observation;
11. persists the authoritative observed state, generation/time fences and
    `lastVerifiedOperationId` through the existing optimistic repository update.

There is no hidden retry after a repository conflict.

## Successful verification

A first successful verification updates only mutable observation/reconciliation
evidence:

- `backendGeneration` becomes the verified observation generation;
- `observedState` becomes the actual authoritative readback representation;
- `observedFingerprint` becomes the matching expected normalized fingerprint;
- `lastObservedAt` advances to the readback time;
- `lastVerifiedOperationId` becomes the exact operation ID;
- `missingSince` is cleared because authoritative present evidence now exists;
- `driftState` becomes `none` because the exact expected operation result has
  been proven.

Stable binding identity, backend/native identity, assignment relationship and
ownership remain repository-immutable.

Representation differences such as expected `930` and observed `0930` are
allowed when the versioned normalized fingerprint is equal. The durable copied
state stores the actual readback representation.

## Idempotent replay and later drift

A repeated verification of the same operation may return `alreadyVerified`
only after the **current incoming observation** still satisfies:

- exact backend/native identity;
- exact operation generation;
- `readbackNotBefore`;
- expected normalized fingerprint.

This ordering is deliberate. A later changed native Timer must surface as
`reconciliationRequired`; it must not be hidden merely because the previous
operation was already verified.

## Fail-closed outcomes

The service exposes separate outcomes for:

- invalid evidence;
- binding not found;
- binding revision conflict;
- identity conflict;
- ownership conflict;
- generation conflict;
- stale observation;
- changed/unexpected present state requiring reconciliation;
- repository optimistic-concurrency conflict;
- repository error.

None of these paths writes a guessed drift classification.

## Scope boundary

This slice does **not** add:

- an operation repository or ADR-0042 lifecycle transition to `succeeded`;
- authoritative native absence/delete verification;
- external drift classification;
- adoption or ownership reassignment;
- TimerAssignment state transition;
- replacement/failover execution;
- Agent Timer commands;
- SuiteBridge, RESTfulAPI or SVDRP Timer mutation;
- daemon scheduler/reconciler wiring;
- public Timer mutation API;
- `mutations=enabled`.

A later orchestration slice may consume `verified` to advance the durable
ADR-0042 operation lifecycle. Native absence remains separately gated by
generation-fenced snapshot completeness evidence.

## Next bounded work

The next safe slice is authoritative native Timer absence evidence. It must
prove that a complete Timer snapshot for one exact backend generation was
successfully observed before the absence of one bound native Timer can be used
for delete verification, external-delete classification or controlled
replacement. Transport failure and partial/incomplete snapshots remain
non-evidence.
