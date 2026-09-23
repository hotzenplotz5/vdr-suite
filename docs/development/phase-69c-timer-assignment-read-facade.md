# Phase 69.C — TimerAssignment Read Facade

## Status

**ACTIVE — bounded prerequisite for the first public durable Timer mutation.**

Baseline:

```text
main=50c51ea0fab08cbae8149ad33136dec869c6c3b0
PR #323=ACCEPTED
69.C=ACTIVE
```

## Purpose

Phase 69.C now has a stable public durable-operation item and the shared opaque
ETag/precondition helper. The next public mutation must still not expose an
internal revision token or invent a parallel Timer lifecycle.

Live repository analysis shows that native Timer CREATE is the cleanest first
mutation candidate because its accepted ADR-0042 operation targets the
Suite-owned `TimerAssignment`, uses durable `MutationOperationRepository`
idempotency and stores the immutable pre-dispatch payload with
`reserveWithPayload()`.

A protected public mutation cannot use `If-Match` correctly until the same
Suite resource has an authoritative readable revision. This slice establishes
only the read boundary needed for that later contract.

## Decision

Add a read-only `TimerAssignmentReadService` inside `core/timers`:

```text
TimerAssignmentRepository
  -> TimerAssignmentReadService
```

The repository remains the sole Phase-64 persistence, revision, epoch and
ownership authority. The facade:

- accepts one `timerAssignmentId` and one explicit `backendId`;
- returns the current repository-owned `TimerAssignment` unchanged on an exact
  backend match;
- preserves the current `assignmentRevision` for later conversion to a public
  opaque ETag;
- reports a real assignment on another backend as `notFound`;
- distinguishes invalid input and storage failure internally;
- never creates, updates, reassigns or otherwise mutates a TimerAssignment.

## Backend and authorization boundary

ADR-0048 requires authorization before current revision metadata is disclosed.
This facade deliberately does not become a second permission evaluator.

A later HTTP slice must therefore:

1. obtain an explicit backend scope from the public request;
2. authenticate the actor through the existing Phase-62 security context;
3. authorize `timers.view` for that backend through the existing
   `AuthorizationService`;
4. only then call this backend-scoped read facade;
5. derive the public strong opaque ETag from `assignmentRevision`.

The facade's backend-mismatch-as-`notFound` behavior prevents an authorized
scope from becoming a cross-backend assignment existence oracle.

## Why this precedes Timer CREATE

The accepted native Timer CREATE preparation contract already binds:

- `resourceType = TimerAssignment`;
- `resourceId = timerAssignmentId`;
- `expectedRevision = assignmentRevision`;
- `actionFamily = timer.create`;
- actor/backend/resource/action-scoped durable idempotency;
- immutable payload persistence before dispatch;
- readback-required completion and no blind retry after uncertain dispatch.

Publishing that mutation before a public TimerAssignment revision exists would
either leak/freeze an internal revision field or make `If-Match` meaningless.
This slice avoids both shortcuts.

## Validation

The focused Phase-69 test proves:

- an exact backend-scoped read succeeds;
- repository-issued `assignmentRevision` is preserved;
- a backend mismatch is hidden as `notFound`;
- a missing assignment is `notFound`;
- empty assignment/backend identity is invalid;
- a repository revision transition is immediately visible through the same
  read facade at the next revision.

The target participates in the normal `test-phase` graph.

## Non-goals

This slice does **not** add:

- a DaemonRuntime `TimerAssignmentRepository` instance;
- a public TimerAssignment HTTP route;
- `timers.view` routing changes in `SecurityHttpGate`;
- public Timer JSON or ETag emission;
- public Timer mutation submission;
- `Idempotency-Key` request handling;
- `If-Match` mutation enforcement;
- NativeTimerBinding exposure;
- any pre-v1 route change;
- Agent, SuiteBridge, VDR-native or media-plane changes.

## Next bounded slice

After this facade is accepted, compose exactly one existing
`TimerAssignmentRepository` over the DaemonRuntime shared database and expose
the read service through a narrow callback. That composition must open the
Phase-64 runtime guard only for the exact reviewed files and must not introduce
a second database or Timer lifecycle authority.
