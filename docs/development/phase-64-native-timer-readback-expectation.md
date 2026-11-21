# Phase 64: Native Timer Expected Readback Contract

## Purpose

ADR-0042 separates backend executor success from verified mutation success.
A native Timer operation may therefore be `executed_unverified` or
`outcome_unknown` while VDR-Suite waits for authoritative readback.

`NativeTimerReadbackExpectation` is the backend-neutral contract that carries
the exact evidence needed to decide whether a later **present** native Timer
observation is even eligible to verify that operation.

It does not execute, persist or classify anything.

## Exact expectation fence

One expectation binds all of these facts:

- stable `operationId`
- operation lifecycle state: `executed_unverified` or `outcome_unknown`
- stable `nativeTimerBindingId`
- exact `expectedBindingRevision`
- exact `backendId`
- non-zero `backendGeneration`
- exact `backendNativeTimerId`
- positive `readbackNotBefore`
- complete expected `NativeTimerObservedState`
- matching normalized `expectedFingerprint`

No individual field is sufficient on its own.

## Why `readbackNotBefore` is required

A state match does not prove that a mutation took effect when the observation
predates dispatch or the point at which the operation could have changed the
backend.

A future verifier must therefore require:

`observation.observedAt >= expectation.readbackNotBefore`

in addition to all identity, revision, generation and fingerprint checks.

This prevents a stale pre-operation snapshot from being accepted merely
because it happens to equal the expected result.

## Self-consistent expected state

The contract carries both the copied expected state and its fingerprint.
Validation requires:

`expectedFingerprint == nativeTimerObservedStateFingerprint(expectedState)`

This keeps expected readback on the same versioned normalized Timer-state
contract used by `NativeTimerBinding` and `NativeTimerObservation`.

Representation-only native time differences such as `930` and `0930` remain
fingerprint-equivalent.

## Allowed operation states

Only ADR-0042 states that still require authoritative verification are valid:

- `executed_unverified`
- `outcome_unknown`

A completed `succeeded`, verified failure, rejected request or pre-dispatch
failure must not create this expectation.

## Present readback only

This slice deliberately models only an expected **present native Timer**.

Delete/absence verification is not represented. A transport failure, lookup
failure or incomplete snapshot is not authoritative absence. Native absence
requires a later dedicated generation-fenced snapshot-completeness contract.

## No drift classification

A changed present observation must not automatically become
`external_field_change`. It may be the exact result of an in-flight operation.

This contract only supplies the expected-operation evidence. A later
operation-aware verifier will compare:

- the current durable binding
- this expectation
- one authoritative `NativeTimerObservation`

and will fail closed on any stale or mismatched fence.

## Scope boundary

This slice has:

- no repository
- no SQLite
- no `VdrTimer` dependency
- no VDR adapter
- no TimerAssignment transition
- no adoption
- no drift-state write
- no native Timer create/update/delete
- no Agent command
- no SuiteBridge, RESTfulAPI or SVDRP mutation wiring
- no daemon wiring
- no public Timer mutation API
- no `mutations=enabled`

## Next bounded slice

Add an operation-aware present-readback verifier that consumes current durable
`NativeTimerBinding`, one valid `NativeTimerReadbackExpectation`, and one valid
`NativeTimerObservation`.

It may prove that an observation matches the expected operation result, but it
must not yet invent authoritative absence or perform native mutation.
