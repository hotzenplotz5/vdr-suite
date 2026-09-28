# Phase 69.F — Public-v1 TimerAssignment Item Reference Client

## Status

**ACCEPTED — eleventh bounded 69.F slice via PR #379.**

Accepted evidence:

```text
PR #379
accepted head=0762ea868d1c9ba93378064107731af9660ac705
merge/main=28eb3301215bea17958c3c1b2f49f1e8c7705445
CI #9348 / run 36366447485 = SUCCESS (6/6)
```

Baseline:

```text
main=7a2c8f651ed895148d5520f2ac5820781b917e49
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #378
accepted head=c996f4c057f0d5b912c00f65d11630cf606cf8c4
merge/main=7a2c8f651ed895148d5520f2ac5820781b917e49
CI #9346 / run 36364060778 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.C Public TimerAssignment Resource](phase-69c-public-timer-assignment-resource.md)
- [Phase 69.F TimerAssignment Collection Reference Client](phase-69f-public-v1-timer-assignment-collection-reference-client.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

## Fresh post-#378 selection audit

The reference client can now discover Backends and enumerate public
TimerAssignments for one authorized backend. The collection items directly
supply:

```text
timerAssignmentId
backendId
```

Those are exactly the inputs required by the accepted TimerAssignment item
resource. The durable Operation item remains less immediately reachable because
its `operationId` originates from a mutation submission path that the
reference client does not yet implement.

The TimerAssignment item is therefore the next bounded read.

## Bounded client extension

The client gains:

```text
getTimerAssignment({
  timerAssignmentId,
  backendId,
  ifNoneMatch,  // optional opaque entity tag
  ...request options
})
```

It performs exactly one request to:

```http
GET /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
```

The Suite-owned item identity is kept as the path identity and the Backend is
the explicit authorization/native-state scope already defined by the public
contract.

## Revision and conditional read

The accepted item resource maps its private repository revision to a strong
opaque ETag. The client:

- returns the response ETag without decoding or manufacturing it;
- sends caller-provided `ifNoneMatch` as `If-None-Match`;
- treats `304 Not Modified` as successful conditional-read state;
- returns `{status: 304, etag, data: null}` for a bodyless 304;
- returns `{status: 200, etag, data}` for a normal item read;
- leaves weak/list/wildcard/malformed entity-tag semantics to the server-owned
  PublicResourcePreconditions contract.

A malformed condition that the server rejects remains structured
`400 invalid_request`.

## Security/error boundary

The client does not reinterpret the accepted visibility rules:

- wrong/missing public resource remains `404 not_found`;
- authorization/backend-scope failures remain public Problem Details;
- unavailable/inconsistent read remains server-owned failure evidence;
- no error triggers fallback or retry.

## Hard boundaries

This slice does not:

- add mutation `If-Match`;
- submit Timer CREATE or any other mutation;
- read durable Operations;
- expose raw `assignmentRevision`;
- expose TimerAssignment state/role/epoch or NativeTimerBinding;
- switch the browser native-Timer surface to TimerAssignment;
- call `/api/vdr/timers` or `/api/vdr/timers/live`;
- change server, VDR, SuiteBridge, Agent, Home or LiveTV behavior.

## Regression contract

The focused Node regression proves:

- exact item URL and backend mapping;
- normal `200` returns public JSON plus opaque ETag;
- caller-provided ETag is emitted as `If-None-Match`;
- bodyless `304` is represented as a successful conditional result;
- invalid local item inputs dispatch zero requests;
- malformed server-side ETag condition remains structured
  `400 invalid_request`;
- hidden/wrong-backend item remains `404 not_found`;
- no retry/fallback occurs.

Architecture guards bind the client to the accepted 69.C item contract, preserve
the native/browser Timer non-equivalence, keep all eight public-v1
method/resource contracts unchanged, and keep Operation/mutation work outside
this slice.

No real yaVDR acceptance is required because this is a client-only consumer of
an already accepted revisioned HTTP read contract. Exact-head Hosted CI is the
Ready-for-Review gate.
