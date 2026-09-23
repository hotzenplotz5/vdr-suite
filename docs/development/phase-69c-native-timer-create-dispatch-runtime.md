# Phase 69.C — Native Timer CREATE Dispatch Runtime Composition

## Status

**ACTIVE — bounded dispatch-chain composition prerequisite for the first public Timer mutation.**

Baseline:

```text
main=b80003de33127827fc121d7d92d7f11c6f4451e6
PR #327 / CI #9105=ACCEPTED
69.C=ACTIVE
```

## Purpose

The accepted predecessor composes the durable Timer CREATE preparation owner:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerCreateOperationPreparationService
  -> MutationOperationRepository::reserveWithPayload()
```

That produces an accepted durable operation and immutable CREATE payload, but
does not make the operation dispatchable.

Phase 64 already contains the next accepted boundaries:

```text
BackendAgentNativeTimerCreateReservationService
  -> durable BackendAgentCommandReservation

NativeTimerCreateDispatchService::claimAfterReservation()
  -> accepted MutationOperation becomes dispatching
  -> exact reservation reference becomes operation resultReference

BackendAgentNativeTimerCreateActivationService::activateDispatching()
  -> exact reserved command becomes visible to normal Agent command delivery
```

This slice composes those owners into DaemonRuntime without invoking them.

## Runtime ownership

DaemonRuntime now owns exactly one:

- `BackendAgentCommandReservationRepository`;
- `BackendAgentNativeTimerCreateReservationService`;
- `NativeTimerCreateDispatchService`;
- `BackendAgentNativeTimerCreateActivationService`.

They reuse existing authorities:

```text
MutationOperationRepository
BackendAgentRepository
BackendAgentCommandRepository
shared Database
```

No second command store, operation store or backend registry is introduced.

## Reservation repository

The reservation repository is constructed over the same shared DaemonRuntime
database and its schema is ensured alongside the existing Backend Agent
control-plane schemas.

A durable reservation remains non-pollable until the accepted activation path
makes exactly that reservation active. This preserves the Phase-64 rule that a
crash cannot cause a fresh command/job/attempt identity to be invented.

## Reservation service

The accepted `BackendAgentNativeTimerCreateReservationService` derives and
persists a fenced command reservation using:

- the current active Backend Agent identity/lease;
- exact backend generation;
- exact operation identity and revision;
- exact TimerAssignment identity/revision/epoch;
- pre-reserved NativeTimerBinding identity;
- the immutable desired native Timer specification;
- current local-provider selection and capability evidence.

This slice only constructs the service. It does not call `reserve()`.

## Dispatch-state service

The accepted `NativeTimerCreateDispatchService` is composed over the existing
`MutationOperationRepository`.

Its future `claimAfterReservation()` transition is the only reviewed boundary
that may move the durable operation from `accepted` to `dispatching` while
persisting the exact reservation reference.

This slice does not call `claimAfterReservation()`.

## Activation service

The accepted `BackendAgentNativeTimerCreateActivationService` is composed over:

- `MutationOperationRepository`;
- `BackendAgentCommandReservationRepository`;
- `BackendAgentCommandRepository`.

Its future `activateDispatching()` call verifies the dispatching operation,
reservation reference, immutable payload, provider fence and operation-revision
successor before activating the exact reserved command.

This slice does not call `activateDispatching()`.

## Linked daemon sources

The daemon adds only the accepted sources required for this boundary:

- `core/timers/src/NativeTimerCreateReadbackExpectation.cpp`;
- `core/timers/src/NativeTimerCreateDispatchService.cpp`;
- `core/agent/src/BackendAgentCommandReservation.cpp`;
- `core/agent/src/BackendAgentNativeTimerCreateReservation.cpp`;
- `core/agent/src/BackendAgentNativeTimerCreateActivation.cpp`.

The existing Agent command repository/delivery and Timer CREATE domain payload
sources remain canonical.

No public controller, SuiteBridge transport or native executor is added by this
slice.

## Dormant boundary

The complete preparation-to-activation chain remains non-invoked.

There are no runtime calls from API/daemon/http/security code to:

```text
nativeTimerCreateOperationPreparationService_->prepare(...)
backendAgentNativeTimerCreateReservationService_->reserve(...)
nativeTimerCreateDispatchService_->claimAfterReservation(...)
backendAgentNativeTimerCreateActivationService_->activateDispatching(...)
```

Therefore this slice cannot create a durable public submission, make a command
pollable, call SuiteBridge or mutate VDR.

## Lifecycle

The CREATE-specific Agent services are destroyed before their repositories and
the Backend Agent repository.

The dispatch-state service is destroyed before the MutationOperation repository.

This preserves dependency lifetime while leaving the existing generic
`BackendAgentCommandRepository` lifecycle unchanged.

## Phase-69 architecture guard

The focused guard proves:

- one reservation repository;
- one CREATE reservation service;
- one CREATE dispatch-state service;
- one CREATE activation service;
- exact source linkage once each;
- reservation schema initialization;
- dependency-safe construction and shutdown;
- no public API/security mutation markers;
- no orchestration method invocation from API/daemon/http/security code.

The predecessor preparation guard continues to prove the single TimerIntent /
preparation ownership boundary and that public mutation remains closed.

## Public API boundary

The accepted public item read remains:

```text
GET /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
```

Its POST method remains closed in this slice.

No `Idempotency-Key`, mutation `If-Match`, request body, `202` response or
write capability is introduced here.

## Acceptance

This is still runtime composition without native effect, so CI/build/architecture
validation is sufficient.

Real yaVDR acceptance becomes mandatory when a later slice invokes the chain and
makes the reserved command visible to Agent delivery / SuiteBridge execution.

## Non-goals

This slice does **not**:

- open public Timer CREATE;
- invoke preparation;
- reserve a command;
- claim a dispatch;
- activate a command;
- change an operation state;
- create a NativeTimerBinding;
- call Agent/SuiteBridge/VDR;
- expose internal TimerIntent revision, assignment epoch, backend generation or
  NativeTimerBinding identity;
- add retry or compatibility fallback.

## Next bounded slice

After acceptance, the next slice can implement the canonical public admission:

```text
POST /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
```

That slice must execute one fenced transaction-like orchestration sequence:

```text
authorize timers.create
-> lookup TimerAssignment after authorization
-> validate required strong If-Match
-> validate closed public request + Idempotency-Key
-> derive all internal revisions/generation/specification/identities
-> prepare durable operation + immutable payload
-> reserve exact Agent command
-> claim exact reservation into dispatching operation
-> activate exact command
-> return 202 + durable operation representation
   Location: /api/v1/operations/{operationId}
```

Failure before activation must map precisely to the ADR-0048 public error model.
Once dispatch/activation becomes ambiguous, the server must never invent a
replacement operation or retry through a legacy route.
