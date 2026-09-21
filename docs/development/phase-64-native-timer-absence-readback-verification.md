# Phase 64 — Operation-aware Native Timer Absence Readback Verification

## Purpose

This slice combines the three authorities required to verify the **postcondition**
of one Suite-managed native Timer delete:

- current durable `NativeTimerBinding` state;
- one valid Slice-18 `NativeTimerAbsenceReadbackExpectation`;
- one valid complete Slice-15 `NativeTimerInventoryEvidence`.

A complete inventory proves absence. The operation expectation proves which
unresolved Suite operation expected that absence. The current binding supplies
the durable identity/revision/ownership fence. None is sufficient alone.

The service verifies the expected postcondition; it does not claim historical
causality when the Timer was already missing before the operation.

## Verification order

`NativeTimerAbsenceReadbackVerificationService::verify()`:

1. validates expectation and complete inventory evidence;
2. reloads the binding by exact `nativeTimerBindingId`;
3. requires exact binding/expectation backend and backend-native identity;
4. permits only `managed` or `adopted` ownership for Suite delete verification;
5. rejects a binding already observed in a generation newer than the expected operation generation;
6. asks the Slice-15 assessor to require exact backend, exact generation,
   `evidence.observedAt >= readbackNotBefore`, and actual absence of the native ID;
7. returns `reconciliationRequired` when the complete inventory still contains the Timer;
8. only after current evidence proves absence, recognizes an idempotent durable
   verification of the same operation;
9. for a first verification, requires exact `expectedBindingRevision`;
10. rejects evidence older than the current durable observation;
11. persists through the existing optimistic `NativeTimerBindingRepository::update()` fence.

There is no hidden retry after a repository conflict.

## Successful verification

The service advances only mutable observation/verification evidence:

- `backendGeneration` to the exact expected/evidence generation;
- `lastObservedAt` to the complete inventory observation time;
- `lastVerifiedOperationId` to the exact operation ID.

If the binding was not previously missing, it also sets:

- `missingSince` to this first authoritative post-operation absence time;
- `driftState = expected_transition`.

The last known present state/fingerprint remain untouched.

## Existing missing evidence is not rewritten

A Suite delete may be issued when a Timer is already missing. A later complete
inventory can prove that the desired delete postcondition is satisfied, but it
cannot retroactively prove that the Suite operation caused the original
absence.

Therefore, when `missingSince` is already non-zero, verification preserves:

- the original `missingSince`;
- existing valid missing cause classification such as `ambiguous`,
  `external_delete`, or `expected_transition`;
- last known present state/fingerprint.

Only `lastVerifiedOperationId`, generation and observation time advance. This
keeps operation verification separate from drift/cause classification.

## Idempotent replay does not hide reappearance

The current complete inventory is assessed **before** `alreadyVerified`.
Therefore a Timer that reappears after a previously verified delete returns
`reconciliationRequired`; it cannot be hidden by the durable operation ID.

An idempotent replay is admitted only when the incoming complete inventory still
proves absence in the exact expected backend generation and after
`readbackNotBefore`.

## Ordering relative to generic absence application

When a matching expected-delete context exists, this verifier must run **before**
Slice-17 generic absence application. First verification requires the exact
operation-time binding revision. Letting the generic recorder write first would
correctly invalidate that revision and force `bindingRevisionConflict`.

Without a matching operation expectation, Slice 17 remains the conservative
fallback that records missing evidence without claiming a Suite operation.

## Scope boundary

This slice adds no:

- operation repository or ADR-0042 lifecycle transition to `succeeded`;
- external-delete classifier;
- ownership or assignment reassignment;
- automatic deletion/archive of NativeTimerBinding history;
- TimerAssignment transition;
- replacement/failover execution;
- VDR/RESTfulAPI/Agent/SuiteBridge/SVDRP Timer mutation;
- daemon wiring;
- public Timer mutation API;
- `mutations=enabled`.

## Next bounded work

After this verifier is accepted, the next safe step is to define how a verified
native Timer operation is correlated with the durable ADR-0042 operation record
without creating a second lifecycle authority. Only after that operation-state
handoff is exact should Phase 64 introduce the native Timer command boundary for
create/update/delete and real runtime acceptance.
