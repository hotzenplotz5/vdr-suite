# Phase 69.C — Native Timer CREATE Reconciliation Runtime Composition

## Status

**ACTIVE — bounded composition prerequisite before the first native-effect Timer CREATE slice.**

Baseline:

```text
main=0d69e40afa2eb2fa666856d7e16317b9b82c71e2
PR #333 merge=f3363f0ec88db2cbeef6dea24ab4349738094ade
ADR-0063=ACCEPTED
69.C=ACTIVE
```

Binding architecture:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0063](../adr/ADR-0063-mutation-complexity-proportionality-reuse.md)
- [Public Timer CREATE Admission](phase-69c-public-timer-create-admission.md)
- [Native Timer CREATE Dispatch Runtime Composition](phase-69c-native-timer-create-dispatch-runtime.md)

## Live architecture finding

After public Timer CREATE admission became accepted, the next obvious step looked
like directly invoking:

```text
reserve()
-> claimAfterReservation()
-> activateDispatching()
```

Live review showed that doing so would be premature.

The Agent/SuiteBridge/native CREATE executor already exists. Once
`activateDispatching()` makes the exact command pollable, a real VDR Timer can
therefore be created.

However, DaemonRuntime did not yet compose the already accepted Phase-64 owners
needed to bring the resulting operation back through durable readback and
completion:

- `NativeTimerCreateReadbackVerificationService`;
- `NativeTimerCreateOperationCompletionService`.

The dispatch service already owns `applyOutcome()`, but no productive runtime
surface invokes it yet.

Opening activation before these owners are available would permit a native
effect while leaving the Control Plane without its complete reviewed
reconciliation path.

This slice closes only that composition gap.

## ADR-0063 proportionality

No new lifecycle, table, repository, binding model or orchestration service is
introduced.

This is deliberately consistent with ADR-0063:

- reuse existing guarantees;
- do not manufacture a new HandoffService merely to wrap three existing owners;
- compose only the already accepted services required by the proven Timer CREATE
  failure model.

Timer CREATE remains the high-complexity managed-resource case. This slice does
not turn its machinery into a generic mutation template.

## Runtime composition

DaemonRuntime now owns exactly one:

```text
NativeTimerCreateReadbackVerificationService
NativeTimerCreateOperationCompletionService
```

They reuse existing authorities:

```text
NativeTimerBindingRepository
TimerAssignmentRepository
MutationOperationRepository
```

Construction is dependency-safe:

```text
repositories
-> NativeTimerCreateDispatchService
-> NativeTimerCreateReadbackVerificationService
-> NativeTimerCreateOperationCompletionService
```

Shutdown reverses the relevant dependency boundary.

## Linked sources

The daemon links the existing Phase-64 implementations and their canonical
present-observation validator dependency:

- `core/timers/src/NativeTimerObservation.cpp`;
- `core/timers/src/NativeTimerCreateReadbackEvidence.cpp`;
- `core/timers/src/NativeTimerCreateReadbackVerificationService.cpp`;
- `core/timers/src/NativeTimerCreateOperationCompletionService.cpp`.

No new implementation source is added for reconciliation ownership.

## Dormant boundary

This slice does not invoke any of the native-effect/reconciliation methods.

The Phase-69 guard rejects productive calls from API/daemon/HTTP/security
integration surfaces to:

```text
backendAgentNativeTimerCreateReservationService_->reserve(...)
nativeTimerCreateDispatchService_->claimAfterReservation(...)
backendAgentNativeTimerCreateActivationService_->activateDispatching(...)
nativeTimerCreateDispatchService_->applyOutcome(...)
nativeTimerCreateReadbackVerificationService_->verify(...)
timerAssignmentFulfillmentService_->bindVerified(...)
nativeTimerCreateOperationCompletionService_->complete(...)
```

Therefore:

- no Agent command becomes pollable;
- no SuiteBridge CREATE is sent;
- no VDR Timer is created;
- no operation state changes beyond the already accepted public admission path;
- no readback/binding/completion is claimed;
- no speculative retry exists.

## Why this slice is required before activation

The accepted final managed CREATE lifecycle is:

```text
public admission
-> durable accepted operation + immutable payload
-> exact Agent reservation
-> operation dispatch claim
-> exact reservation activation / Agent pollability
-> native executor outcome
-> MutationOperation applyOutcome
-> authoritative managed Timer readback
-> NativeTimerBinding verification
-> TimerAssignment bindVerified
-> operation completion
```

The exact implementation of the later readback trigger still has to be reviewed
against the live runtime before activation is opened. This composition slice
only ensures the accepted owners are available without inventing new authority.

## Acceptance for this slice

CI/build/architecture validation is sufficient because no native effect is
reachable.

The focused guard proves:

- one verification owner;
- one completion owner;
- existing repository reuse;
- exact source linkage once;
- dependency-safe construction and shutdown;
- no authority leakage into PublicApiRuntime or SecurityHttpGate;
- no reserve/claim/activate/outcome/readback/bind/complete invocation.

Real yaVDR acceptance is not required for this composition-only candidate.

## Next bounded slice

After this composition prerequisite is accepted, live review must derive the
smallest safe native-effect slice.

That slice may invoke the existing reservation/claim/activation chain only if it
also proves how Agent executor outcomes and authoritative readback reach the
already composed reconciliation owners.

The first candidate that makes `vdr.timer.create` pollable requires exact-head
real yaVDR acceptance before merge.
