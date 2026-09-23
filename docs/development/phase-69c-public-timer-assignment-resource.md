# Phase 69.C — Public TimerAssignment Resource

## Status

**ACCEPTED — first stable public-v1 Timer domain item read.**

Baseline:

```text
main=92c127fbe40d8d20ac2294e1b4b28fd3ea3a6560
PR #325 / CI #9100=ACCEPTED
69.C=ACTIVE
```

## Purpose

The accepted Phase-69.C prerequisites now provide:

- the shared public opaque ETag / precondition helper;
- the durable public operation resource;
- a backend-scoped read-only `TimerAssignmentReadService`;
- exactly one DaemonRuntime `TimerAssignmentRepository` over the shared Suite
  database;
- a lifecycle-safe dormant TimerAssignment lookup in `PublicApiRuntime`.

This slice opens exactly one stable Timer domain resource so later protected
Timer mutation can use a public revision contract rather than leaking an
internal revision field.

## Stable route

```text
GET /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
```

The TimerAssignment identity is the stable Suite-owned
`timerAssignmentId`. The required `backend` query value is authorization and
native-state scope; it is not a replacement for the resource identity.

This slice does not expose a TimerAssignment collection.

```text
GET /api/v1/timer-assignments
-> 404 not_found
```

## Backend scope and authorization

The backend scope is interpreted exactly once by `SecurityHttpGate`.

For an item GET the gate:

1. authenticates the caller;
2. extracts `backend` from the query;
3. accepts only a non-empty backend ID of at most 128 characters containing
   alphanumeric characters, `.`, `_` or `-`;
4. authorizes the existing `timers.view` permission for that exact backend;
5. records the authorization decision through the existing accountability path;
6. passes only the authorized backend ID to the router/public runtime.

The `PublicApiRuntime` does not reparse `backend=`. This prevents the
authorization layer and resource layer from interpreting different backend
scopes.

Expected security outcomes include:

- unauthenticated caller -> `401`;
- missing or syntactically unsafe backend scope -> `400 invalid_backend_scope`;
- missing `timers.view` -> `403 permission_denied`;
- grant for another backend -> `403 backend_scope_denied`;
- authorized backend plus missing/foreign assignment -> `404 not_found`.

Authorization therefore precedes resource lookup and revision disclosure.

## Public representation

A successful `200` response deliberately exposes only:

```json
{
  "timerAssignmentId": "...",
  "backendId": "...",
  "links": {
    "self": "/api/v1/timer-assignments/...?backend=..."
  }
}
```

The representation intentionally does not freeze or disclose:

- raw `assignmentRevision`;
- TimerAssignment state;
- role;
- intent revision;
- assignment epoch;
- backend generation;
- decision evidence;
- channel mapping internals;
- NativeTimerBinding identity.

Those Phase-64 fields remain internal until separately reviewed public
semantics exist.

## Revision and conditional GET

The authoritative repository-owned `assignmentRevision` is mapped internally
to `resourceRevision` and converted by the accepted
`PublicResourcePreconditions` helper into a strong opaque ETag.

The raw revision is not returned in JSON.

Supported semantics:

- ordinary authorized GET -> `200` plus current `ETag`;
- matching `If-None-Match` -> `304`, empty body, current `ETag`;
- malformed `If-None-Match` -> `400 invalid_request`;
- unavailable or internally inconsistent lookup data -> `503`.

The accepted strong/weak/list/wildcard matching rules remain owned by the shared
precondition helper rather than being reimplemented here.

## Method contract

The item resource is read-only in this slice.

```text
GET -> supported
POST/PUT/PATCH/DELETE/HEAD/OPTIONS -> 405 method_not_allowed
Allow: GET
```

No action-style mutation alias is introduced under this resource.

## Capability discovery

`GET /api/v1/capabilities` now reports:

```text
public-api.timer-assignments-read / version 1
```

Its availability follows whether the accepted TimerAssignment lookup is
registered.

## Compatibility and inventory

The Phase-69 public route inventory now explicitly includes the stable item
prefix:

```text
/api/v1/timer-assignments/
```

No pre-v1 Timer route is changed or declared stable by this slice.

## Validation

Focused Phase-69 regression covers:

- authentication;
- exact backend-scoped `timers.view` authorization;
- wrong-scope and missing-permission denial;
- invalid/missing backend scope;
- authorized backend propagation from SecurityHttpGate through TestHttpServer
  and ApiRouter;
- minimal public representation;
- strong opaque ETag;
- conditional `304`;
- malformed conditional request;
- missing/foreign resource `404`;
- closed collection;
- read-only method contract;
- capability discovery;
- architecture guard proving PublicApiRuntime does not reparse the backend query.

## Non-goals

This slice does **not** add:

- a TimerAssignment collection;
- TimerIntent public resources;
- Timer CREATE submission;
- Timer modify/delete submission;
- `Idempotency-Key` request handling;
- mutation `If-Match`;
- public NativeTimerBinding;
- scheduler/planner activation;
- a second repository or database;
- any pre-v1 route change;
- Agent, SuiteBridge, VDR-native or media-plane behavior.

No repeated real-yaVDR acceptance is required for this slice because it changes
only Control-Plane HTTP read/security semantics and does not alter native VDR
mutation or media behavior.

## Accepted checkpoint

This public TimerAssignment resource slice was accepted in PR #326:

```text
merge=61f933d1ae23aaa26d0ff3715e68e9686450ee7f
CI=35895657815 / #9103 / SUCCESS (6/6)
```

## Next bounded slice

The next step is [Native Timer CREATE Preparation Runtime Composition](phase-69c-native-timer-create-preparation-runtime.md):
one DaemonRuntime `TimerIntentRepository` plus the already accepted
`NativeTimerCreateOperationPreparationService` over the existing assignment
and mutation-operation authorities.

That composition remains dormant: no public mutation, no `prepare()` call,
no dispatch and no native execution.
