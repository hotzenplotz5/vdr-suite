# Phase 64 — Native Timer Expected Absence Readback Contract

## Purpose

A complete Timer inventory can prove that a native Timer is absent, but absence
alone cannot prove that a Suite-managed delete operation succeeded. The same
fact may also result from an external delete, backend restart, provider state
change, or another concurrent action.

This slice therefore introduces a separate backend-neutral
`NativeTimerAbsenceReadbackExpectation` for ADR-0042 operation context.

The expectation is **not absence evidence** and performs no verification by
itself. A later verifier must combine it with the current durable
`NativeTimerBinding` and one valid complete `NativeTimerInventoryEvidence`.

## Exact operation fence

One expectation binds:

- `operationId`;
- ADR-0042 readback state `executed_unverified` or `outcome_unknown`;
- exact `nativeTimerBindingId`;
- exact `expectedBindingRevision` captured for the operation;
- exact `backendId`;
- non-zero exact `backendGeneration`;
- exact `backendNativeTimerId`;
- positive `readbackNotBefore`.

All identity/revision strings are bounded to 160 characters.

`readbackNotBefore` prevents a complete inventory captured before dispatch from
verifying the later delete merely because the Timer was already absent at that
time.

## Reuse of ADR-0042 operation state

The contract reuses the existing typed `NativeTimerReadbackOperationState` from
the present-readback expectation. Both present and absent verification admit
only the same unresolved ADR-0042 states:

- `executed_unverified`;
- `outcome_unknown`.

No new operation lifecycle vocabulary is invented in this slice.

## No expected present state

Unlike `NativeTimerReadbackExpectation`, the absence contract deliberately has
no `NativeTimerObservedState` or fingerprint. The expected result is absence of
one exact backend-native identity from a **complete** inventory, not a special
synthetic Timer state.

## Scope boundary

This slice adds no:

- inventory read or absence assessment;
- NativeTimerBinding repository write;
- delete verification service;
- ADR-0042 operation lifecycle transition to success;
- external-delete classification;
- TimerAssignment transition;
- replacement/failover execution;
- VDR/Agent/SuiteBridge/SVDRP Timer mutation;
- daemon wiring;
- public Timer mutation API;
- `mutations=enabled`.

## Next bounded work

Add an operation-aware native Timer **absence verifier** that consumes the exact
expectation, current durable managed/adopted binding and one complete inventory.
It may verify only when identity, binding revision, generation and
`readbackNotBefore` all match and the Slice-15 assessor proves the native Timer
absent. It may then record the operation ID on the binding while preserving
absence evidence. Operation-record lifecycle orchestration remains separate.
