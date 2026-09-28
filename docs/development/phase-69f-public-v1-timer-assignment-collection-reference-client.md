# Phase 69.F — Public-v1 TimerAssignment Collection Reference Client

## Status

**ACCEPTED — tenth bounded 69.F slice via PR #378.**

Accepted evidence:

```text
PR #378
accepted head=c996f4c057f0d5b912c00f65d11630cf606cf8c4
merge/main=7a2c8f651ed895148d5520f2ac5820781b917e49
CI #9346 / run 36364060778 = SUCCESS (6/6)
```

Baseline:

```text
main=7704d3826aa426a4cb5e48c4ba07e372614b86fe
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #377
accepted head=959ac28a5f88f34282a7644c31c1982ff6beface
merge/main=7704d3826aa426a4cb5e48c4ba07e372614b86fe
CI #9344 / run 36346772858 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.D First Public Collection: TimerAssignment](phase-69d-timer-assignment-collection.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)
- [Phase 69.F Public-v1 Discovery Reference Client](phase-69f-public-v1-discovery-reference-client.md)

## Fresh post-#377 selection audit

The accepted reference client now covers public discovery and federated Channel
reads. The remaining stable reads are:

- TimerAssignment collection;
- TimerAssignment item;
- durable Operation item.

The TimerAssignment collection is the smallest coherent successor. It requires
only one authorized Backend identity already obtainable from public Backend
discovery. Unlike the TimerAssignment item and Operation item, the collection
has no resource ETag, no conditional GET contract and no dependency on a
mutation-produced operation identifier.

This slice therefore does not add item reads or mutations.

## Bounded client extension

The JavaScript reference client gains:

```text
getTimerAssignments({
  query: {
    backendId,  // required
    limit,      // optional, 1..100
    cursor,     // optional, opaque
    sort,       // optional, "timerAssignmentId"
    order       // optional, "asc"
  }
})
```

It emits exactly one request to:

```http
GET /api/v1/timer-assignments?backend={backendId}
```

The client field is named `backendId` to match the stable public identity used
by Backend discovery; it maps to the already accepted HTTP query member
`backend`.

## Collection semantics preserved

The client preserves the accepted Phase-69.D contract:

- one explicit backend only;
- `timerAssignmentId ASC` keyset order;
- default server limit 50, client-validated explicit limits 1..100;
- opaque cursor round-trip;
- no offset pagination;
- `meta.partial=false` on success;
- no collection ETag;
- no total-count guarantee;
- storage/read failure remains structured `503 service_unavailable`.

An offline native VDR backend is not converted to partial or empty data because
this read uses the Suite-owned repository and performs no backend I/O.

## Hard boundaries

This slice does not:

- read `/api/vdr/timers` or `/api/vdr/timers/live`;
- migrate the bundled browser Timer list;
- expose NativeTimerBinding or native Timer identity;
- add TimerAssignment item/ETag/conditional GET;
- add Timer CREATE or any mutation;
- add Operation reads;
- add retry, fallback, offset pagination or multi-backend aggregation;
- change server, VDR, SuiteBridge, Agent, Home or LiveTV behavior.

## Regression contract

The focused Node regression proves:

- exact `backend` query mapping from public `backendId`;
- bounded limit, opaque cursor and fixed sort/order validation;
- caller-owned auth/cache options survive;
- successful `meta.partial=false` is preserved;
- invalid client query shapes dispatch zero requests;
- `503 service_unavailable` remains a structured public client error;
- failed reads are not retried.

Architecture guards bind the client method to the accepted 69.D collection
contract, preserve native Timer/browser non-equivalence, keep the stable
public-v1 method/resource count at eight, and keep item/Operation work outside
this slice.

No real yaVDR acceptance is required because this is a client-only consumer of
an already accepted Suite-owned read contract. Exact-head Hosted CI is the
Ready-for-Review gate.
