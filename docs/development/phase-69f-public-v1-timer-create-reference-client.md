# Phase 69.F — Public-v1 Timer CREATE Admission Reference Client

## Status

**ACCEPTED — twelfth bounded 69.F slice via PR #380.**

Accepted evidence:

```text
PR #380
accepted head=1b033576fab3c5322c14edc6405d5c6948dc50f3
merge/main=afd0e04708622d99a3ee82532e7d8608495d3f6b
CI #9352 / run 36367430072 = SUCCESS (6/6)
```

Baseline:

```text
main=28eb3301215bea17958c3c1b2f49f1e8c7705445
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #379
accepted head=0762ea868d1c9ba93378064107731af9660ac705
merge/main=28eb3301215bea17958c3c1b2f49f1e8c7705445
CI #9348 / run 36366447485 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.C Public Timer CREATE Admission](phase-69c-public-timer-create-admission.md)
- [Phase 69.F TimerAssignment Item Reference Client](phase-69f-public-v1-timer-assignment-item-reference-client.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

## Fresh post-#379 selection audit

The public-v1 inventory still contains exactly eight stable method/resource
contracts. The independent reference client now consumes six of them. The two
remaining contracts form one dependency chain:

```text
POST TimerAssignment CREATE admission
  -> returns operationId / Location
  -> GET durable Operation item
```

The mutation must come first because its input is already available from the
accepted TimerAssignment item read:

- `timerAssignmentId`;
- `backendId`;
- opaque strong TimerAssignment ETag.

The durable Operation read remains the bounded successor because this POST
creates the Operation identity that it consumes.

## Bounded client extension

The client gains:

```text
submitTimerCreate({
  timerAssignmentId,
  backendId,
  ifMatch,
  idempotencyKey,
  ...request options
})
```

It emits exactly one request:

```http
POST /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
Content-Type: application/json
If-Match: <caller-owned opaque ETag>
Idempotency-Key: <caller-owned opaque key>

{}
```

The client does not accept or expose a native Timer specification. The durable
TimerAssignment already owns that state.

## Caller-owned safety inputs

The reference client deliberately does not generate or transform:

- `If-Match`;
- `Idempotency-Key`;
- operation identity.

It only requires non-empty caller inputs and lets the server own canonical ETag
and Idempotency-Key syntax/semantics.

Exact replay after response loss remains an explicit caller action using the
same logical request, same If-Match and same Idempotency-Key. The client never
automatically retries a mutation.

## Accepted response

A successful/replayed admission is returned as:

```text
{
  status: 202,
  location: "/api/v1/operations/{operationId}",
  etag: "<opaque Operation ETag>",
  data: {
    operationId,
    state,
    backendId,
    links
  }
}
```

The client does not interpret `state=accepted` as native Timer success. It
also does not automatically poll `location`; Operation reconciliation remains
a separate client slice.

## Error boundary

Public Problem Details remain structured `VdrSuitePublicClientError` values.
In particular:

- `409 idempotency_conflict` remains visible;
- `412 revision_conflict` remains visible;
- `428 precondition_required` remains server-owned for requests that reach the
  server without a precondition;
- backend/security/service errors are not converted into retries or legacy
  Timer calls.

## Hard boundaries

This slice does not:

- generate Idempotency-Keys;
- decode or manufacture ETags;
- automatically retry Timer CREATE;
- automatically poll/read Operations;
- call `/api/vdr/timers/actions/create`;
- switch the bundled browser Timer mutation path;
- expose NativeTimerSpecification or NativeTimerBinding;
- change Timer CREATE runtime, Agent, SuiteBridge, VDR, Home or LiveTV behavior.

## Regression contract

The focused Node regression proves:

- exact public POST URL and empty-object body;
- caller-owned Authorization/CSRF headers survive;
- caller-owned If-Match and Idempotency-Key are sent unchanged;
- `202` returns Location, opaque Operation ETag and Operation representation;
- an exact replay happens only because the caller invokes the client again;
- invalid local required inputs dispatch zero requests;
- `412 revision_conflict` and `409 idempotency_conflict` remain structured;
- no automatic retry/polling/fallback occurs.

Architecture guards bind the method to the accepted Phase-69.C admission
contract, keep the eight stable public-v1 method/resource contracts unchanged,
preserve browser/native Timer non-equivalence, and keep Operation reconciliation
outside this slice.

No real yaVDR acceptance is required because this slice changes only the
reference client consuming an already accepted public mutation contract; it
does not alter productive mutation/runtime behavior. Exact-head Hosted CI is the
Ready-for-Review gate.
