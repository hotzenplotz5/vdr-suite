# Phase 69.F — Public-v1 Channel Reference Client

## Status

**CANDIDATE — ninth bounded 69.F slice.**

Baseline:

```text
main=88b7e01d5fb327fea3f54d43f31202dc7e303d98
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #376
accepted head=67d103443ec247eb7c7e3d395386510ae33b8fe4
merge/main=88b7e01d5fb327fea3f54d43f31202dc7e303d98
CI #9341 / run 36344690345 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.D Federated Public Channel Collection](phase-69d-public-channel-federation.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)
- [Phase 69.F Public-v1 Discovery Reference Client](phase-69f-public-v1-discovery-reference-client.md)

## Fresh post-#376 selection audit

The accepted reference seam now consumes public root, capabilities and Backend
discovery. The remaining already-stable public-v1 reads are Channel collection,
TimerAssignment collection/item and durable Operation item.

Channel is the smallest coherent successor because:

- Backend discovery directly yields the `backendId` source identities it needs;
- the contract is read-only and provider-neutral;
- public `(backendId, channelId)` identity is already accepted;
- pagination and partial-result semantics are already stabilized by Phase 69.D;
- it requires no ETag/If-None-Match handling;
- it requires no mutation, idempotency or prior operation identifier.

TimerAssignment and Operation client methods therefore remain separate slices.

## Bounded client extension

The existing JavaScript reference client gains:

```text
getChannels({
  query: {
    backendIds: [...],  // required, 1..16
    limit,              // optional, 1..100
    cursor,             // optional, opaque
    sort,               // optional, "backendId,channelId"
    order               // optional, "asc"
  }
})
```

It emits one request to:

```http
GET /api/v1/channels?backendId=...
```

Repeated `backendId` members preserve explicit source selection. The client
does not invent default/all-backend behavior.

## Partial results and failures

A successful federated response is returned unchanged, including
`meta.partial` and per-source `state` / `code` evidence. A partial 200 is
not converted into failure.

Public Problem Details remain structured `VdrSuitePublicClientError` values.
In particular, `409 cursor_expired` is surfaced to the caller and does not
cause a retry with a changed source set or a legacy route.

## Hard boundaries

This slice does not:

- call or deprecate `/api/vdr/channels`;
- migrate `fetchClientChannels()` in the bundled browser;
- create a default-backend shortcut;
- hide unavailable Channel sources;
- retry after arbitrary failures;
- add TimerAssignment or Operation client methods;
- add a new public resource or server route;
- change Home, LiveTV, VDR, SuiteBridge or Agent behavior.

## Regression contract

The focused Node regression proves:

- repeated explicit backend source encoding;
- fixed Channel sort/order validation;
- limit and 1–16 source bounds;
- caller-owned auth/cache options survive;
- partial-source metadata is preserved;
- `cursor_expired` remains a structured 409 error;
- invalid client query shapes dispatch zero HTTP requests;
- a failed request is not retried.

Architecture guards bind the method to the accepted Phase-69.D Channel contract,
preserve the browser/public Channel non-equivalence classification, keep the
stable public-v1 contract count at eight, and wire the slice into fast CI,
architecture and Phase-69 validation.

No real yaVDR acceptance is required because this is a client-only consumer of
an already accepted read contract. Exact-head Hosted CI is the Ready-for-Review
gate.
