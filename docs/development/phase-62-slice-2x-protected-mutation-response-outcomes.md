# Phase 62 Slice 2X — Protected Mutation Response Outcomes

## Status

Selected documentation contract only. No Slice-2X production code, schema,
packaging, installation or runtime mutation exists yet.

PR #117 remains open, Draft and unmerged.

## Why this work is necessary

The selection is based on a direct requirement-to-code proof, not on a desire to
add another security feature.

### Binding Phase-62 requirement

The Phase-62 exit criteria require:

```text
every privileged mutation has actor, decision and outcome evidence
```

Actor and authorization-decision evidence already exist. Outcome evidence for
ordinary protected business mutations does not.

### Current code evidence

`SecurityHttpGate::appendDecisionEvent()` writes only the pre-dispatch result:

```text
dispatch_authorized
dispatch_denied
```

For an allowed protected mutation, `TestHttpServer::handleRequest()` then calls
`ApiRouter::handleClientPost()` and returns its response. No post-dispatch event
is written for that router result.

Browser-session issue and logout are different: Slice 2S already records their
actual `operation.succeeded` or `operation.failed` result. That implementation
explicitly excludes business mutations outside browser-session lifecycle.

### Concrete unresolved failure

With the accepted code, these two requests produce indistinguishable
accountability evidence before dispatch:

1. an authorized mutation that completes successfully;
2. the same authorized mutation whose router, backend or domain owner returns an
   error.

Both have actor and `dispatch_authorized`, but neither has persisted evidence of
the returned mutation result. The explicit Phase-62 outcome criterion is
therefore not met.

## Why the rejected audit-read slice is not necessary

No Phase-62 exit criterion requires an HTTP endpoint for reading accountability
events. Existing evidence can be inspected through repository/runtime acceptance
without adding a new production route.

The following are therefore not justified by the demonstrated gap:

- `GET /api/security/accountability/events`;
- a new `security.audit.read` permission;
- audit-reader role semantics;
- an audit JSON serializer or frontend;
- pagination, filtering, export, redaction or retention work;
- audit-of-audit read transactions.

They may only be reconsidered after a separate concrete requirement or failure
case is established.

## Exact Slice-2X scope

Slice 2X adds outcome evidence only for the protected mutation dispatches already
classified by `SecurityHttpGate`.

It does not add or reclassify any HTTP route.

For every authorized protected mutation that reaches
`ApiRouter::handleClientPost()`, the HTTP owner appends exactly one event after
the router returns:

```text
HTTP 200..299  -> event_type=operation.succeeded, outcome=succeeded
all other HTTP -> event_type=operation.failed,    outcome=failed
```

The event reuses the already-authorized context:

- actor, actor type, device and session;
- authentication state;
- permission, backend scope and action;
- operation ID when supplied by the existing request contract;
- request ID and correlation ID.

The reason code is the observed HTTP status:

```text
http_status_<decimal status>
```

Examples:

```text
http_status_200
http_status_400
http_status_503
```

No request body, response body, header, cookie, credential or secret is copied
into the event.

## Ownership boundary

The smallest coherent owner set is:

- `SecurityGateDecision`: retains the already-calculated protected-mutation
  accountability context needed after dispatch;
- `SecurityHttpGate`: constructs and appends the exact outcome event;
- `TestHttpServer`: invokes the outcome append only after
  `ApiRouter::handleClientPost()` returns;
- `AccountabilityEventRepository`: unchanged append-only persistence.

No new repository, service, database table, index, permission, role, route,
configuration variable or frontend owner is selected.

## Failure semantics

The existing pre-dispatch event remains mandatory. If it cannot be persisted,
dispatch does not occur.

If the router has returned but the outcome event cannot be persisted:

- the original router response is not delivered as a successful accountable
  result;
- the server returns HTTP 503 `accountability_unavailable` with the existing
  request/correlation headers;
- the response contains no request or response body copied from the mutation;
- the implementation does not claim that an already executed external/domain
  side effect was rolled back;
- clients must not be told that automatic replay is safe.

This is intentionally an HTTP-observed result contract. It does not claim
cross-system crash atomicity, compensation, a transactional Outbox or an
idempotent operation framework.

Those larger mechanisms are not selected merely because this bounded outcome
gap exists. They require their own concrete failure proof before implementation.

## Focused test contract

The implementation tests must prove:

1. one `operation.succeeded` event after an authorized protected mutation returns
   2xx;
2. one `operation.failed` event after an authorized protected mutation returns a
   non-2xx status;
3. exact actor, permission, backend, action, operation, request and correlation
   continuity from the pre-dispatch event;
4. exact `http_status_<status>` reason and `succeeded`/`failed` outcome;
5. no outcome event for authorization denial, authentication denial, CSRF
   denial, Safe POST, GET or unsupported methods;
6. no duplicate outcome event for one dispatch;
7. forced post-dispatch append failure returns
   `503 accountability_unavailable`;
8. pre-dispatch append failure still prevents router dispatch;
9. no request body, response body, Authorization value, cookie, CSRF token,
   password, verifier hash or other secret reaches accountability persistence.

## Architecture guard

The implementation must add or extend a static guard that proves:

- no new HTTP route or permission is introduced by Slice 2X;
- outcome append occurs after `handleClientPost()` and before the final response;
- only `gate.protectedMutation` can reach the new outcome path;
- `AccountabilityEventRepository` remains append-only;
- no schema/configuration/frontend/packaging owner is added;
- response bodies and request headers are not used to build the event.

## CI and runtime boundary

Before implementation, the documentation-selection head must pass all five jobs:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`;
- `packaging-regression-test`.

After implementation and green source CI, a later bounded yaVDR acceptance must
prove one successful and one deterministic failed already-protected mutation,
matching pre-dispatch and outcome events, secret-free persistence, unchanged
security configuration, database integrity and rollback of all test-owned
runtime changes.

The runtime plan must use an existing low-impact/reversible protected owner. It
must not add a production route or mutate unrelated VDR data merely to test the
audit event.

## Explicit exclusions

Slice 2X does not implement:

- a protected audit read API or audit frontend;
- audit export, filtering, pagination, redaction, deletion or retention;
- new permissions, roles, grants or security administration;
- transactional Outbox or generic cross-system commit atomicity;
- revisions, `If-Match`, idempotency keys or durable operation replay;
- native/service credential enrollment, rotation or revocation;
- compatibility retirement;
- Android, Android TV or Phase 63–67 runtime;
- PR Ready transition, merge or review-metadata changes.

## Selection gate

Implementation may begin only after this contract and all canonical current-state
files are mutually consistent and the final documentation-selection head has all
five required CI jobs green.
