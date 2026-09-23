# Phase 69.C — TimerAssignment Runtime Composition

## Status

**ACCEPTED — bounded runtime-composition prerequisite for the first public durable Timer resource/mutation.**

Baseline:

```text
main=b971b074725351004d676c001ee976f574587390
PR #324 / CI #9098=ACCEPTED
69.C=ACTIVE
```

## Purpose

PR #324 established a read-only `TimerAssignmentReadService` inside the Timer
domain. The next safe step is to make that accepted read authority available to
the public-v1 owner without opening a public Timer route or introducing a second
database/lifecycle authority.

## Decision

DaemonRuntime now owns exactly one TimerAssignment repository/read pair over the
already-open shared Suite database:

```text
DaemonRuntime::database_
  -> TimerAssignmentRepository
  -> TimerAssignmentReadService
```

Startup uses the repository's existing idempotent `ensureSchema()`. No second
SQLite connection, Timer store, scheduler, assignment planner or mutation owner
is created.

## Narrow PublicApiRuntime callback

The public runtime receives a dormant internal lookup callback:

```text
(timerAssignmentId, backendId)
  -> { status, timerAssignmentId, backendId, resourceRevision }
```

The callback intentionally exposes only the data required to build the later
stable read/precondition contract:

- stable Suite `timerAssignmentId`;
- explicit `backendId`;
- repository-owned `assignmentRevision` mapped internally as
  `resourceRevision`.

It does **not** expose TimerAssignment state, role, decision evidence,
NativeTimerBinding identity, backend generation, internal intent revision or
other Phase-64 persistence fields.

This callback is an internal C++ dependency, not a public HTTP resource.

## Authorization boundary

The callback itself is not a permission evaluator. The following contract
remains mandatory for the later HTTP slice:

1. authenticate through the existing Phase-62 request context;
2. derive and validate the explicit backend scope;
3. authorize `timers.view@<backendId>` through the existing
   `AuthorizationService`;
4. only after authorization invoke the TimerAssignment lookup;
5. convert `resourceRevision` into the accepted strong opaque public ETag.

Therefore this slice does not disclose revision metadata over HTTP.

## Lifecycle safety

The callback is registered at the same late DaemonRuntime composition point as
the accepted operation lookup, after fallible domain/runtime initialization and
immediately before router/HTTP construction.

Shutdown resets the TimerAssignment callback before destroying the read service
and repository. This prevents the singleton PublicApiRuntime from retaining a
dangling DaemonRuntime capture.

## Phase-64 guard opening

Two existing Phase-64 guards previously prohibited every TimerAssignment runtime
crossing outside `core/timers`.

This slice does not weaken those guards generally. It adds only the exact
reviewed files required for:

- PublicApiRuntime callback declaration/implementation and focused test;
- DaemonRuntime ownership/composition/shutdown;
- the two daemon files that instantiate the repository authority.

Any other TimerAssignment or TimerAssignmentRepository runtime crossing still
fails the existing architecture checks.

## Build boundary

The daemon source list now links only:

```text
core/timers/src/TimerAssignment.cpp
core/timers/src/TimerAssignmentRepository.cpp
core/timers/src/TimerAssignmentReadService.cpp
```

The daemon compile target receives the narrow `-Icore/timers/include` include
path. Global compiler include scope is not widened.

## Validation

The focused Phase-69 test verifies:

- callback starts unconfigured;
- registration and reset lifecycle;
- exact ID/backend lookup result;
- revision propagation;
- unavailable/invalid/notFound statuses;
- the public TimerAssignment HTTP route remains closed with `404 not_found`.

A dedicated architecture guard verifies exact DaemonRuntime composition, source
linkage, shutdown order and the narrow Phase-64 allow-lists.

## Non-goals

This slice does **not** add:

- `GET /api/v1/timer-assignments/{id}`;
- a TimerAssignment collection;
- public Timer JSON;
- public ETag emission for TimerAssignment;
- `timers.view` route classification;
- Timer CREATE submission;
- `Idempotency-Key` request handling;
- mutation `If-Match`;
- NativeTimerBinding exposure;
- Timer planner/scheduler activation;
- any pre-v1 route change;
- Agent, SuiteBridge, VDR-native or media-plane behavior.

## Accepted checkpoint

This runtime-composition slice was accepted in PR #325:

```text
merge=92c127fbe40d8d20ac2294e1b4b28fd3ea3a6560
CI=35892930603 / #9100 / SUCCESS (6/6)
```

## Next bounded slice

The next step is [Public TimerAssignment Resource](phase-69c-public-timer-assignment-resource.md):
one authenticated backend-scoped item read with a deliberately reviewed public
representation, strong opaque ETag and `If-None-Match` handling.
`timers.view` authorization must occur before revision or existence
information is returned.
