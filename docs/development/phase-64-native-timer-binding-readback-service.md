# Phase 64 Slice 12 — NativeTimerBinding Present Readback Application

## Scope

Slice 12 adds the first backend-neutral application service that combines a
validated `NativeTimerObservation` with current durable
`NativeTimerBindingRepository` state.

The service is deliberately conservative. It applies only semantically
unchanged **present** observations. It does not classify a changed native Timer,
clear missing evidence, adopt an external Timer, transition a TimerAssignment to
`bound`, perform replacement/failover or mutate native state.

## Input authority

The service accepts `NativeTimerObservation`, the Slice-11 backend-neutral
present-read evidence value. It therefore never depends on `VdrTimer`, a VDR
snapshot implementation or provider-specific transport state.

Every observation must already carry:

- bounded backend identity;
- non-zero backend generation;
- backend-scoped native Timer identity;
- positive authoritative observation time;
- valid copied native state;
- exact normalized observed fingerprint.

Invalid evidence fails before any repository access or write.

## Exact binding lookup

The service resolves only by the exact stable tuple:

```text
backendId + backendNativeTimerId
```

If no binding exists, the result is `unboundObservation` and nothing is
persisted. Native discovery/import/adoption remains a separate explicit flow.

There is no title/channel/time similarity fallback.

## Monotonic observation fences

Before comparing native state, the service fails closed on stale evidence:

- lower backend generation -> `staleGeneration`;
- lower observation time -> `staleObservation`.

The service does not retry or reinterpret stale evidence.

## Changed state is reconciliation evidence

A different normalized fingerprint is not overwritten.

The service returns `reconciliationRequired` with the current durable binding
and performs no repository update. This preserves the old binding evidence for
a later reconciler that can classify external field change, disable, native
identity mismatch or expected operation transition with the appropriate policy
and operation context.

Similarly, a present observation received while the durable binding has
non-zero `missingSince` returns `reconciliationRequired`. The service does not
silently clear durable absence evidence.

## Safe unchanged refresh

When the normalized fingerprint is identical and no missing evidence exists,
the service may advance only:

- `backendGeneration`;
- `lastObservedAt`.

It deliberately preserves:

- `observedState` copied representation;
- `observedFingerprint`;
- ownership;
- assignment relationship;
- drift classification;
- verified operation identity;
- missing state (which must already be zero on this path).

Preserving the durable copied state prevents representation-only churn such as
alternating `930` and `0930` from issuing semantically meaningless state
changes. The normalized fingerprint already proves equivalence.

If generation and observation time are both already current, the result is
`alreadyCurrent` and no new repository revision is issued.

Otherwise the repository performs one exact optimistic-concurrency update and
issues the successor binding revision.

## Repository conflict

A concurrent writer may advance the binding after the service reads it. The
service does not retry using the previously evaluated observation.

A repository revision conflict becomes `repositoryConflict` with the current
durable binding. The caller must reload/re-evaluate from fresh evidence.

This preserves the Slice-10 exact concurrency boundary and prevents a hidden
retry from applying a plan against stale ownership/drift state.

## Existing drift is preserved

A binding may already carry a non-`none` drift classification while the current
observed fingerprint remains unchanged. An unchanged present read may still
advance generation/time evidence, but the service preserves the existing drift
classification exactly.

This slice does not decide that a later identical read resolves drift.
Reconciliation policy owns that decision.

## Regression contract

The focused SQLite-backed service test proves:

- a semantically identical newer observation refreshes generation/time and
  increments repository revision;
- `930` durable state is preserved when a later `0930` read has the same
  normalized fingerprint;
- exact replay is an `alreadyCurrent` no-op with no new revision;
- lower generation is rejected;
- lower observation time is rejected;
- a material fingerprint change returns `reconciliationRequired` and leaves the
  durable binding untouched;
- present-after-missing returns `reconciliationRequired` and does not clear
  `missingSince`;
- existing drift classification survives an unchanged observation refresh;
- an unbound native observation is not imported or adopted;
- invalid observation evidence fails closed.

The underlying Slice-10 repository regression already proves cross-connection
stale-writer rejection. Slice 12 consumes that conflict result without hidden
retry.

## Architecture boundary

The service lives under `core/timers` and depends only on:

- `NativeTimerObservation`;
- `NativeTimerBinding`;
- `NativeTimerBindingRepository`.

It has no VDR adapter, SQLite, TimerAssignment planner/scheduler, Agent command,
SuiteBridge, SVDRP or mutation dependency.

No runtime wiring is added in this slice.

## Runtime boundary

This slice changes no installed runtime path. The service is exercised only by
the focused test graph and is not added to daemon/Agent runtime source lists.

Real yaVDR runtime acceptance is not required. A focused real yaVDR build/test
is useful portability evidence.

## Next bounded work

The next safe slice is explicit **changed-state reconciliation classification**.
It must compare the current binding, the changed `NativeTimerObservation`,
assignment/operation evidence where required, and produce a durable drift
classification without yet performing native mutation.

Missing-native evidence requires a separate explicit absence-observation
contract; a present-read service must not invent absence from transport failure.
Assignment transition to `bound`, replacement/failover and native writes remain
later controlled slices.
