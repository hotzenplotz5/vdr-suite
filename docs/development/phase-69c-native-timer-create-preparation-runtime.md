# Phase 69.C — Native Timer CREATE Preparation Runtime Composition

## Status

**ACTIVE — bounded daemon composition prerequisite for the first public Timer mutation.**

Baseline:

```text
main=61f933d1ae23aaa26d0ff3715e68e9686450ee7f
PR #326 / CI #9103=ACCEPTED
69.C=ACTIVE
```

## Purpose

Phase 64 already established the durable native Timer CREATE preparation
contract:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerCreateOperationPreparationService
  -> MutationOperationRepository::reserveWithPayload()
```

That accepted service validates the current durable intent and assignment,
binds the mutation to the exact assignment revision/backend generation and
atomically persists the durable operation plus immutable CREATE payload.

Until this slice, the service existed only as a tested Control-Plane component
and had no DaemonRuntime owner.

This slice composes that accepted authority into the daemon without opening any
public mutation or native dispatch path.

## Runtime ownership

DaemonRuntime now owns exactly one additional Timer repository:

```text
DaemonRuntime::database_
  -> MutationOperationRepository
  -> TimerIntentRepository
  -> TimerAssignmentRepository
```

and exactly one preparation service:

```text
NativeTimerCreateOperationPreparationService(
    TimerIntentRepository,
    TimerAssignmentRepository,
    MutationOperationRepository)
```

No second database, repository lifecycle or mutation store is introduced.

The initialization order intentionally creates the TimerIntent schema before the
TimerAssignment schema because TimerAssignment persistence refers to
`timer_intents(timer_intent_id)`.

## Linked daemon sources

The daemon links only the sources needed by the accepted preparation boundary:

- `TimerIntent.cpp`;
- `TimerIntentRepository.cpp`;
- `NativeTimerBinding.cpp` for the observation-state validation dependency used by
  `NativeTimerSpecification.cpp`;
- `NativeTimerSpecification.cpp`;
- `NativeTimerCreateOperationPayload.cpp`;
- `NativeTimerCreateOperationPreparationService.cpp`.

The existing TimerAssignment and MutationOperation sources remain the same
authorities.

This slice deliberately does not link a new CREATE dispatch/execution owner.

## Dormant boundary

The preparation service is constructed but not invoked.

There is no call to:

```text
nativeTimerCreateOperationPreparationService_->prepare(...)
```

outside the already accepted Timer-domain tests.

Therefore this slice cannot:

- reserve a new public mutation;
- create a native Timer;
- dispatch to an Agent;
- call SuiteBridge;
- call VDR/SVDRP;
- change TimerAssignment state;
- advertise a new write capability.

It only makes the already accepted preparation authority available for a later
reviewed admission boundary.

## Lifecycle

The service depends on all three repositories, so shutdown destroys it first:

```text
NativeTimerCreateOperationPreparationService
  -> TimerAssignment read service
  -> TimerAssignmentRepository
  -> TimerIntentRepository
  -> MutationOperation read service
  -> MutationOperationRepository
```

The shared database closes only after those owners have been destroyed.

## Phase-64 guard evolution

The original Phase-64 preparation guard correctly prohibited all runtime
consumers when the preparation contract was first introduced.

Phase 69.C now opens only the exact reviewed DaemonRuntime ownership/lifecycle
files:

- `core/daemon/include/DaemonRuntime.h`;
- `core/daemon/src/DaemonRuntimeInitialization.cpp`;
- `core/daemon/src/DaemonRuntimeShutdown.cpp`.

Any other use of `NativeTimerCreateOperationPreparationService` outside the
Timer domain remains rejected by the Phase-64 architecture guard.

This is a narrow evolution of the historical boundary, not removal of it.

## Phase-69 architecture guard

The focused Phase-69 guard verifies:

- exactly one DaemonRuntime `TimerIntentRepository`;
- exactly one DaemonRuntime
  `NativeTimerCreateOperationPreparationService`;
- intent repository initialization precedes assignment repository composition;
- preparation service construction uses the existing intent, assignment and
  mutation-operation repositories;
- service shutdown precedes repository destruction;
- required daemon sources are linked exactly once;
- CREATE dispatch/native execution sources are not pulled into this slice;
- no `prepare()` call exists in API/daemon/http/security runtime code;
- no PublicApiRuntime or SecurityHttpGate mutation handling is introduced;
- the Phase-64 runtime allow-list remains exact and narrow.

## Public API boundary

The accepted public TimerAssignment read remains unchanged:

```text
GET /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
```

This slice adds no POST/PATCH/DELETE route, no request JSON and no mutation
headers.

In particular it does not yet consume:

- `Idempotency-Key`;
- mutation `If-Match`;
- a public Timer CREATE body;
- a public operation submission response.

## Native/runtime acceptance

No repeated real-yaVDR acceptance is required for this slice because there is
still no preparation invocation, dispatch, Agent command, SuiteBridge command or
VDR-native effect.

CI must prove daemon compilation and the architecture/test guards. Real-device
acceptance becomes relevant when a later slice activates the real mutation
execution path.

## Non-goals

This slice does **not** add:

- public Timer CREATE submission;
- a public TimerIntent resource;
- a public TimerAssignment collection;
- `Idempotency-Key` handling;
- mutation `If-Match`;
- operation `202 Accepted` / `Location` semantics;
- NativeTimerBinding exposure;
- CREATE dispatch/start;
- Agent activation;
- SuiteBridge/VDR mutation;
- retry/fallback behavior.

## Next bounded slice

After this composition is accepted, the remaining 69.C implementation can open
the reviewed public Timer CREATE admission/submission boundary.

That slice must translate public inputs into the internal preparation contract
without exposing internal TimerIntent revision, assignment epoch, backend
generation or NativeTimerBinding identity.

Before dispatch is allowed it must enforce the accepted public mutation rules:

- explicit backend-scoped `timers.create` authorization;
- closed request JSON;
- required `Idempotency-Key`;
- required strong `If-Match` against the public TimerAssignment ETag;
- durable actor/backend/resource/action idempotency;
- stable operation response and `Location`;
- `409`, `412` and `428` semantics from ADR-0048;
- no speculative retry and no pre-v1 mutation fallback.
