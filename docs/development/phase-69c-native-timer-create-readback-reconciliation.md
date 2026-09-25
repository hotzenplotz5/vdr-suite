# Phase 69.C — Native Timer CREATE Readback Reconciliation

## Status

**ACTIVE — bounded dormant reconciliation candidate after accepted PR #342.**

Live accepted baseline used for this slice:

```text
main=d1e543b977da45f732c768f4efdf8877910dd90e
PR #342 merge=d1e543b977da45f732c768f4efdf8877910dd90e
PR #342 accepted head=78880a00e0cc330b7375ee24086162a89e6cd5ac
PR #342 CI=36132407130 / #9210 / SUCCESS (6/6)
69.C=ACTIVE
```

Binding architecture:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0063](../adr/ADR-0063-mutation-complexity-proportionality-reuse.md)
- [Native Timer CREATE Reconciliation Runtime Composition](phase-69c-native-timer-create-reconciliation-runtime.md)
- [Native Timer CREATE Outcome Evidence](phase-69c-native-timer-create-outcome-evidence.md)
- [Native Timer CREATE Outcome Application](phase-69c-native-timer-create-outcome-application.md)

## Proven predecessor boundary

PR #342 connected the existing durable typed Agent result to the accepted
`NativeTimerCreateDispatchService::applyOutcome()` transition without adding a
productive runtime callsite:

```text
BackendAgentCommandDeliveryService::result()
-> BackendAgentCommandRepository::acceptResult()
-> backend_agent_command_results
-> BackendAgentCommandRepository::resultForCommand()
-> applyDurableNativeTimerCreateResult()
-> NativeTimerCreateDispatchService::applyOutcome()
-> NativeTimerCreateReadbackExpectation
```

For `acceptedUnverified` and `outcomeUnknown`, `applyOutcome()` already
constructs the exact CREATE readback expectation from the immutable durable
operation payload plus the exact Agent outcome evidence. In particular,
`readbackNotBefore` is the real `dispatchStartedAt` from the Agent evidence.

Authoritative Timer readback remains a different evidence source. It must not be
fabricated from the Agent result, inferred from the operation state or replaced
by a retry of the CREATE mutation.

## Bounded change

This slice adds one stateless daemon-composition adapter:

```text
reconcileNativeTimerCreateReadback(...)
```

It consumes only:

- an existing `NativeTimerCreateReadbackExpectation`;
- one existing authoritative complete `NativeTimerCreateReadbackEvidence`;
- the already durable `native.timer.create` operation payload;
- the already composed accepted verification, fulfillment and completion owners.

The adapter performs:

```text
NativeTimerCreateReadbackExpectation
+ authoritative NativeTimerCreateReadbackEvidence
+ durable NativeTimerCreateOperationPayload
-> NativeTimerCreateReadbackVerificationService::verify()
-> TimerAssignmentFulfillmentService::bindVerified()
-> NativeTimerCreateOperationCompletionService::complete()
```

It owns no persistence, repository, native read, polling loop, retry loop or
lifecycle state.

## Existing authorities remain authoritative

No new authority is introduced.

### MutationOperation authority

`MutationOperationRepository` remains the sole durable operation/payload and
operation-revision authority.

The reconciliation adapter reads the existing immutable
`NativeTimerCreateOperationPayload`. It does not issue operation revisions.
Final `succeeded` transition ownership remains in
`NativeTimerCreateOperationCompletionService::complete()`, which delegates the
revision-fenced transition to `MutationOperationRepository`.

### NativeTimerBinding authority

`NativeTimerBindingRepository` remains the sole durable binding and
`bindingRevision` authority.

The adapter never writes a binding directly. Only
`NativeTimerCreateReadbackVerificationService::verify()` may create the
verified managed binding from authoritative readback evidence.

### TimerAssignment authority

`TimerAssignmentRepository` remains the durable assignment-revision authority.

The adapter never writes an assignment directly. Only
`TimerAssignmentFulfillmentService::bindVerified()` may transition the
assignment from `provisioning` or `reconciling` to `bound`.

## Fences preserved by this slice

The existing readback verifier requires:

- exact backend identity;
- exact `backendGeneration`;
- `evidence.observedAt >= expectation.readbackNotBefore`;
- exactly one managed correlation matching both reserved
  `timerAssignmentId` and `nativeTimerBindingId`;
- exact expected native Timer specification;
- no conflicting existing binding/native identity/assignment binding.

The immutable operation payload supplies the already accepted fulfillment
fences:

- `expectedAssignmentRevision`;
- `expectedIntentRevision`;
- `assignmentEpoch` as retained operation evidence;
- reserved `nativeTimerBindingId`;
- `backendId`;
- `backendGeneration`;
- expected native Timer specification.

`TimerAssignmentFulfillmentService::bindVerified()` additionally requires the
exact repository-issued `bindingRevision`, managed ownership and verified
binding state.

`NativeTimerCreateOperationCompletionService::complete()` independently
reloads the authoritative operation, assignment and binding and requires:

- the operation identity/backend/generation/resource/action/fingerprint match;
- `readbackRequired` verification policy;
- operation state matching the expectation;
- a bound assignment using the reserved binding;
- a managed verified binding for the same operation;
- native observation freshness at or after `readbackNotBefore`;
- exact specification match.

Only then may the operation become `succeeded`.

## Revision and replay behavior

The predecessor lifecycle remains:

```text
reservation-time operation revision N
-> claimAfterReservation(): dispatching revision N+1
-> applyOutcome(): executed_unverified/outcome_unknown revision N+2
```

The new adapter does not reinterpret any of these revisions.

The focused regression proves the concrete sequence:

```text
reserved revision 1
-> dispatching revision 2
-> applyOutcome revision 3
-> verified binding + assignment bound
-> completion revision 4
```

A readback observation older than `dispatchStartedAt` is rejected as stale.
That failure leaves the operation at revision 3, the assignment provisioning and
no binding created.

Exact downstream replay is idempotent:

```text
verify()       -> alreadyVerified
bindVerified() -> alreadyBound
complete()     -> alreadyCompleted
operation revision remains 4
```

After an operation is already terminal `succeeded`, a later productive runtime
must gate that terminal state before attempting to re-apply the durable Agent
outcome. While the operation remains in the accepted unverified outcome state,
the PR-#342 outcome adapter may reconstruct the same readback expectation through
its exact replay path.

## Boundary that remains closed

This candidate is linked and directly tested but is intentionally not called by
a productive runtime surface.

It does **not** invoke or enable:

```text
BackendAgentNativeTimerCreateReservationService::reserve()
NativeTimerCreateDispatchService::claimAfterReservation()
BackendAgentNativeTimerCreateActivationService::activateDispatching()
applyDurableNativeTimerCreateResult()
NativeTimerCreateDispatchService::applyOutcome()
Agent polling
SuiteBridge/VDR Timer CREATE
authoritative native Timer inventory acquisition/trigger
```

It receives already supplied readback evidence only. Therefore it cannot by
itself reach VDR or make an Agent command pollable.

```text
NATIVE_EFFECT_REACHABLE=NO
YAVDR_ACCEPTANCE_REQUIRED=NO
```

Hosted repository CI is sufficient for this dormant slice.

## Next bounded candidate

After this prerequisite is accepted, the remaining meaningful Timer CREATE
boundary is the productive native-effect/runtime slice.

That future slice must review the live runtime again and connect, as one bounded
lifecycle:

```text
public durable CREATE admission
-> exact reservation
-> dispatch claim
-> activation / Agent pollability
-> actual SuiteBridge/VDR CREATE
-> durable typed Agent result
-> outcome application
-> authoritative complete Timer readback acquisition
-> reconcileNativeTimerCreateReadback()
-> verified NativeTimerBinding
-> TimerAssignment bound
-> MutationOperation succeeded
```

Because that candidate will make `vdr.timer.create` pollable and a real native
effect reachable, hosted CI alone is insufficient. Exact-head real yaVDR
acceptance is required before merge, including duplicate/replay, generation,
revision, evidence and no-blind-retry proofs.
