# Phase 69.C — Native Timer CREATE Fulfillment Runtime Composition

## Status

**ACTIVE — bounded prerequisite after accepted PR #329.**

Baseline:

```text
main=b5ef507a94e4c36a2472d1697c4c973efefe01f0
PR #329 / CI #9119=ACCEPTED
69.C=ACTIVE
```

Binding decisions and accepted contracts:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [TimerAssignment Fulfillment State](phase-64-timer-assignment-fulfillment.md)
- [Native Timer CREATE Preparation Runtime](phase-69c-native-timer-create-preparation-runtime.md)
- [Native Timer CREATE Dispatch Runtime](phase-69c-native-timer-create-dispatch-runtime.md)
- [Durable TimerAssignment Native Specification](phase-69c-timer-assignment-native-specification.md)

## Why this prerequisite exists

Live review after PR #329 showed that the accepted
`NativeTimerCreateOperationPreparationService::prepare()` does not own the
TimerAssignment lifecycle transition. It requires the current durable
TimerAssignment to already be in `provisioning`.

Phase 64 already owns that transition in exactly one service:

```text
TimerAssignmentFulfillmentService::beginProvisioning()
selected
  -> revision / intent / backend-generation fences
  -> provisioning
```

The production DaemonRuntime did not yet compose that owner or the
`NativeTimerBindingRepository` dependency it requires. Opening a public Timer
CREATE handler before composing the accepted owner would force the HTTP/runtime
layer either to write TimerAssignment state directly or to duplicate lifecycle
logic. Both are rejected.

## Runtime composition

This slice adds exactly one production-daemon instance of:

- `NativeTimerBindingRepository` over the existing shared daemon `Database`;
- `TimerAssignmentFulfillmentService` over the existing
  `TimerAssignmentRepository` and the new runtime-owned binding repository.

The binding repository schema is initialized before the fulfillment service is
constructed.

The existing CREATE preparation, command reservation, dispatch and activation
owners remain unchanged.

## Dormant boundary

Composition is not invocation.

This slice deliberately does **not** call:

```text
timerAssignmentFulfillmentService_->beginProvisioning(...)
timerAssignmentFulfillmentService_->bindVerified(...)
nativeTimerCreateOperationPreparationService_->prepare(...)
backendAgentNativeTimerCreateReservationService_->reserve(...)
nativeTimerCreateDispatchService_->claimAfterReservation(...)
backendAgentNativeTimerCreateActivationService_->activateDispatching(...)
```

No Agent command becomes pollable and no SuiteBridge or VDR mutation can be
caused by this slice.

## Public API boundary

The stable TimerAssignment resource remains:

```text
GET /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
```

The item remains GET-only. This slice does not add:

- `POST`;
- `timers.create`;
- mutation `If-Match`;
- `Idempotency-Key`;
- public native Timer fields;
- operation submission;
- legacy or RESTfulAPI mutation fallback.

## Lifecycle and shutdown

The runtime dependency order is:

```text
TimerAssignmentRepository
  -> NativeTimerBindingRepository
  -> TimerAssignmentFulfillmentService
  -> NativeTimerCreateOperationPreparationService
```

Shutdown destroys the fulfillment service before either repository.

The binding repository source is linked exactly once together with its existing
read/write storage implementation.

## Acceptance

CI/build/architecture validation is sufficient because the new service remains
dormant and no native effect is activated.

The original Phase-64 NativeTimerBinding and NativeTimerBindingRepository
guards are retained as safety boundaries. Their blanket runtime prohibitions are
narrowed only for `DaemonRuntime.h` and
`DaemonRuntimeInitialization.cpp`, and only when this Phase-69 successor guard
is present with the exact dormant-fulfillment and repository-source markers.
All other runtime surfaces remain forbidden.

The focused Phase-69 guard proves:

- one binding repository owner;
- one fulfillment service owner;
- exact dependency and shutdown order;
- binding repository sources linked exactly once;
- no fulfillment invocation from API/daemon/HTTP/security integration;
- PublicApiRuntime and SecurityHttpGate remain mutation-closed.

Real yaVDR acceptance remains required only once a later candidate can actually
activate the native CREATE path.

## Next bounded prerequisite

Public CREATE admission still must not open until Suite-owned identity
allocation is proven.

The accepted CREATE preparation request requires both:

- stable `operationId`;
- pre-reserved `nativeTimerBindingId`.

The next bounded slice must locate and reuse an accepted Control-Plane identity
authority, or establish a separately reviewed Suite-owned identity authority if
the repository proves none exists. It must not use an Agent-local identity
generator as accidental public/API authority and must not invent identities
inside retry/dispatch code.

Only after that identity prerequisite is accepted can the public admission
safely sequence:

```text
authorize + If-Match + idempotency
-> beginProvisioning
-> prepare
-> reserve
-> claimAfterReservation
-> activateDispatching
```
