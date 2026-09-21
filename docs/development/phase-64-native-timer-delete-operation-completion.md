# Phase 64 Slice 21 — Native Timer Delete Operation Completion

## Status

Bounded correlation slice stacked on Phase 64 Slice 20 / Draft PR #173.

Slice 19 can prove the expected native Timer delete postcondition and durably
record that proof on `NativeTimerBinding`. Slice 20 introduces the one shared
ADR-0042 `MutationOperationRepository`. This slice connects those two durable
facts without adding another lifecycle authority and without executing a native
Timer mutation.

## Single lifecycle authority

The ADR-0042 `MutationOperationRepository` from Slice 20 remains the **Single
lifecycle authority**. This service never stores operation state on a Timer
object and never introduces a Timer-specific operation table.

The service may only move an existing shared operation from one unresolved
readback state:

```text
executed_unverified
outcome_unknown
```

to:

```text
succeeded
```

The transition uses the exact repository-owned `operationRevision`. A stale or
concurrently changed operation therefore fails closed through the existing
revision fence.

## Exact correlation contract

Completion consumes the Slice-18
`NativeTimerAbsenceReadbackExpectation` as the bounded correlation context, then
reloads both durable records itself.

The shared operation must match all of:

- exact `operationId`;
- exact backend ID and backend generation;
- `resourceType == NativeTimerBinding`;
- resource ID equal to the exact `nativeTimerBindingId`;
- operation `expectedRevision` equal to the expectation's exact
  `expectedBindingRevision`;
- `actionFamily == timer.delete`;
- verification policy `readback_required`;
- current unresolved operation state equal to the expectation state.

The durable binding must independently match all of:

- exact binding ID;
- exact backend ID/generation;
- exact backend-native Timer identity;
- managed or adopted ownership;
- `lastVerifiedOperationId` equal to the exact operation ID;
- durable missing evidence (`missingSince > 0`);
- `lastObservedAt >= readbackNotBefore`.

Only that conjunction is sufficient to complete the operation.

## Why the expectation is still checked

`lastVerifiedOperationId` proves which operation Slice 19 verified, but the
shared operation also owns the mutation's expected resource revision. The
Slice-18 expectation carries the exact operation-time binding revision used by
Slice 19. Matching it against `MutationOperation.expectedRevision` prevents an
unrelated expectation carrying the same operation ID from being treated as the
shared operation's verification context.

No arithmetic is performed on opaque resource revisions.

## Durable completion result and replay

The successful transition stores one deterministic bounded result reference:

```text
native-timer-delete-readback:<nativeTimerBindingId>:operation:<operationId>
```

If the shared operation is already `succeeded` with that exact result reference,
the service returns `alreadyCompleted` without requiring the Timer binding to
still have the same current observation. This preserves ADR-0042 idempotency:
later resource changes do not erase the durable result of a previously completed
logical operation.

A `succeeded` operation with another result reference is not reclassified and
fails as an operation-state conflict.

## Cause classification remains separate

The service does not require `expected_transition` drift classification. Slice
19 intentionally distinguishes postcondition verification from historical
causality and can preserve earlier `ambiguous` or `external_delete` evidence.

For operation completion, the authoritative fact is the exact operation-bound
absence proof represented by `lastVerifiedOperationId` plus the identity,
generation, revision and readback fences above.

## Scope boundary

This slice adds only the correlation service, focused regression, architecture
guard, Make test fragment and documentation.

It adds:

- no native Timer mutation;
- no Timer create/update/delete command dispatch;
- no Agent/SuiteBridge/SVDRP/RESTfulAPI write path;
- no TimerAssignment transition;
- no replacement or failover;
- no external-change classification;
- no daemon/runtime wiring;
- no public Timer mutation API;
- no broad Timer UI;
- no `mutations=enabled` switch.

Because no installed runtime path changes, real yaVDR runtime acceptance is not
required for Slice 21.

## Next bounded work

**Slice 22** should define the pre-dispatch Native Timer delete operation
preparation/handoff contract: create or load the shared ADR-0042 operation,
validate the managed/adopted binding and exact revision/generation, and produce
the bounded expected-absence context required by Slices 18/19/21. It must still
stop before real Agent/VDR Timer mutation until command construction, provider
authority and runtime wiring are separately fenced.
