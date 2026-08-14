# Phase 64 Slice 22 — Native Timer Delete Operation Preparation

## Status

Bounded pre-dispatch slice, now retargeted to `main` after Slice 21 / PR #174 merged.

Slice 20 supplies the single durable ADR-0042 mutation operation authority.
Slice 21 closes a delete operation only after Slice 19 has verified the native
absence readback. This slice adds the opposite end of that chain: safely reserve
one delete operation against one exact current managed/adopted
`NativeTimerBinding` and produce the immutable target context a later dispatcher
must revalidate before any backend write.

## Pre-dispatch only

`NativeTimerDeleteOperationPreparationService` is **Pre-dispatch only**.
It performs no backend action and grants no execution authority by itself.

The request carries:

- stable `operationId`;
- durable `idempotencyKey`;
- actor identity;
- normalized request fingerprint;
- exact `nativeTimerBindingId`;
- exact expected binding revision;
- exact expected backend generation;
- request time and optional deadline.

The service reloads the binding from `NativeTimerBindingRepository`; caller
supplied native identity or backend identity is never trusted.

## Binding eligibility fence

Preparation requires the current binding to be:

- present in the durable repository;
- `managed` or `adopted`;
- on the exact expected binding revision;
- on the exact expected backend generation;
- not already durably missing;
- free of unresolved drift (`driftState == none`).

A missing or drifted binding requires reconciliation instead of blind delete
preparation.

The operation captures the exact current binding revision as ADR-0042
`expectedRevision`, but a later dispatcher must re-read that revision before
actual execution. Operation reservation and execution authority remain distinct.

## Shared operation reservation

The prepared shared operation is fixed to:

```text
resourceType = NativeTimerBinding
resourceId = <nativeTimerBindingId>
expectedRevision = <current binding revision>
actionFamily = timer.delete
verificationPolicy = readback_required
state = accepted
```

The backend ID comes from the durable binding, and the requested backend
generation must match that binding exactly.

The Slice-20 repository owns the stable idempotency scope and first
`operationRevision`. Exact replay is returned as `alreadyPrepared` only while
the durable operation is still `accepted`.

If the operation has already advanced to `dispatching` or any later state,
preparation replay fails with `operationStateConflict`. This prevents a caller
from treating an old preparation response as permission to dispatch the same
logical operation again.

## NativeTimerDeleteDispatchHandoff

A successful preparation returns one immutable
`NativeTimerDeleteDispatchHandoff` containing:

- operation ID and exact current operation revision;
- binding ID and exact expected binding revision;
- TimerAssignment ID carried by the managed/adopted binding;
- backend ID and exact backend generation;
- exact backend-native Timer identity.

This is target context, not a native command and not a readback expectation.

## No premature readback fence

Slice 18 requires a positive `readbackNotBefore` because an observation captured
before dispatch must never prove that a later mutation took effect.

Therefore Slice 22 deliberately does **not** create a
`NativeTimerAbsenceReadbackExpectation`, does not invent
`executed_unverified`/`outcome_unknown`, and does not carry `readbackNotBefore`.
Those facts do not exist before an actual dispatch attempt.

A later slice must establish the real dispatch boundary first and only then mint
the Slice-18 expectation.

## Concurrency and idempotency

Preparation can race with another binding writer after the binding read. That is
safe because:

1. the operation records the exact binding revision;
2. the handoff carries that same revision;
3. actual dispatch is a later fenced step that must re-read and revalidate it.

No hidden retry rewrites the operation onto a newer binding revision.

The shared repository also prevents a reused idempotency scope with a changed
logical operation or fingerprint and prevents an operation ID from being reused
for another logical request.

## Scope boundary

This slice adds only the preparation service, focused regression, architecture
guard, isolated Make fragment and documentation.

It adds:

- no native Timer mutation;
- no RESTfulAPI/SVDRP/SuiteBridge/Agent command construction;
- no dispatch state transition;
- no post-dispatch outcome state;
- no `NativeTimerAbsenceReadbackExpectation` creation;
- no `readbackNotBefore` value;
- no TimerAssignment transition;
- no replacement/failover;
- no daemon/runtime wiring;
- no public Timer mutation API;
- no broad Timer UI;
- no `mutations=enabled` switch.

Because no installed runtime path changes, real yaVDR runtime acceptance is not
required for Slice 22.

## Next bounded work

**Slice 23** should define the native Timer delete dispatch claim/outcome
contract around this handoff. It must re-read the exact operation and binding,
claim `accepted -> dispatching` through the shared operation revision fence,
and define how one real executor outcome produces either
`executed_unverified`, `outcome_unknown`, or a verified/pre-dispatch failure.
Only after a real dispatch boundary exists may it create the Slice-18 expected
absence with a trustworthy `readbackNotBefore`.

Actual Agent/VDR transport wiring should remain a separate bounded runtime slice
unless the dispatch contract can be connected without weakening provider,
generation, authorization, or acceptance boundaries.
