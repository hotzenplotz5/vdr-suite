# Phase 69.C — MutationOperation Read Facade

## Status

**ACTIVE — bounded 69.C slice after the accepted public precondition foundation.**

Baseline:

```text
main=beb98f6edab39d962bd6415db7be21cf145e05cb
PR #321 / CI #9091=ACCEPTED
69.C=ACTIVE
```

## Purpose

The public API cannot safely expose an operation resource by opening a second
database, creating a parallel operation store, or bypassing the durable
ADR-0042 lifecycle authority.

The repository truth at this baseline is:

- `MutationOperationRepository` is the durable ADR-0042 authority;
- it already owns `operationRevision`, idempotency scope, request fingerprint
  and state transitions;
- no production DaemonRuntime instance of that repository existed yet;
- PublicApiRuntime therefore had no legitimate read dependency for an
  `/api/v1/operations/{id}` resource.

## Decision

This slice establishes exactly one DaemonRuntime repository instance over the
existing shared `database_`:

```text
DaemonRuntime::database_
  -> MutationOperationRepository
  -> MutationOperationReadService
```

The read service is deliberately read-only. It does not:

- reserve operations;
- transition operation state;
- interpret mutation payloads;
- create another lifecycle state;
- cache an independent copy of operation truth.

## Actor-scoped read semantics

`MutationOperationReadService::findForActor(operationId, actorId)` returns an
operation only when the durable record belongs to that exact actor.

An operation owned by another actor is returned as `notFound`, not as a
distinct authorization/existence response. This prevents operation identifiers
from becoming a cross-actor existence oracle before the public route is added.

Storage and input failures remain distinguishable internally so the later REST
owner can map them onto ADR-0048 Problem Details without parsing messages.

## Runtime composition

Daemon startup now:

1. opens the existing VDR-Suite database;
2. constructs one `MutationOperationRepository` over that database;
3. runs its existing idempotent `ensureSchema()`;
4. constructs one `MutationOperationReadService` over that repository.

No second database connection or public-only operation table is introduced.

## Validation

The focused Phase-69 test verifies:

- owner read succeeds;
- current `operationRevision` is preserved;
- idempotency key remains durable read data;
- another actor receives `notFound`;
- a missing operation receives `notFound`;
- invalid empty identity input is rejected;
- a repository state transition is visible through the same read facade at the
  next revision.

The target is included in the normal `test-phase` graph, and the daemon build
must link the same repository/read-service sources.

## Non-goals

This slice does **not** yet add:

- `GET /api/v1/operations/{id}`;
- operation JSON serialization;
- ETag response emission;
- `If-None-Match` handling on an operation resource;
- public mutation submission;
- `Idempotency-Key` request handling;
- any pre-v1 route change.

## Accepted checkpoint

This read-facade/runtime-composition slice was accepted in PR #322:

```text
merge=377f59e7b2d8e5ec3c7cbf22522c3c1789fd9849
CI=35876551406 / #9094 / SUCCESS (6/6)
```

## Next bounded slice

The next 69.C slice is [Public Operation Resource](phase-69c-public-operation-resource.md).
It binds this read service into `PublicApiRuntime` and exposes the first
read-only durable operation item with authenticated ownership, opaque ETag and
conditional GET semantics.
