# Phase 69.C — Public Operation Resource

## Status

**ACTIVE — bounded 69.C slice after the accepted operation read facade.**

Baseline:

```text
main=377f59e7b2d8e5ec3c7cbf22522c3c1789fd9849
PR #321 / CI #9091=ACCEPTED
PR #322 / CI #9094=ACCEPTED
69.C=ACTIVE
```

Binding decisions:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Public precondition foundation](phase-69c-public-preconditions.md)
- [MutationOperation read facade](phase-69c-operation-read-facade.md)

## Purpose

This slice exposes the first durable operation resource through the stable v1
client API:

```text
GET /api/v1/operations/{operationId}
```

There is still no public `/api/v1/operations` collection in this slice. The
literal `/api/v1/operations/` is the inventoried item-resource prefix.

The route uses the already accepted actor-scoped
`MutationOperationReadService`. It does not open another database, create
another operation store or bypass the ADR-0042 lifecycle authority.

## Authentication and visibility

A concrete operation item read requires an authenticated actor before the
request reaches the Public API runtime.

The read facade then applies exact actor ownership:

```text
authenticated actor owns operation
  -> visible

missing operation
OR operation belongs to another actor
  -> 404 not_found
```

The 404 equality is intentional. A caller cannot distinguish another actor's
operation ID from a nonexistent operation ID.

The contract root and capability resource keep their existing authentication
semantics; this slice does not make every `/api/v1` GET actor-owned.

## Public representation

A successful item read returns only reviewed public fields:

```json
{
  "operationId": "op_...",
  "state": "queued",
  "backendId": "backend_...",
  "links": {
    "self": "/api/v1/operations/op_..."
  }
}
```

The first representation deliberately does **not** expose:

- idempotency key;
- normalized request fingerprint;
- expected resource fingerprint;
- immutable mutation payload;
- verification policy;
- deadline;
- internal result reference;
- repository-owned `operationRevision` as a JSON field;
- internal resource type / resource ID names whose public taxonomy has not yet
  been reviewed.

Those fields remain available to the owning Control Plane services but are not
silently frozen into v1.

Compatible additive public fields may be introduced later under ADR-0048 after
their semantics are reviewed.

## Operation state vocabulary

The public `state` uses the shared ADR-0042 mutation lifecycle vocabulary:

```text
accepted
rejected
conflict
queued
dispatching
executed_unverified
succeeded
failed_before_dispatch
failed_verified
outcome_unknown
cancelled
```

HTTP `200` means that the durable operation representation was retrieved. It
does not mean the mutation succeeded.

In particular, a resource may return `200` while state is
`dispatching`, `failed_verified`, `outcome_unknown` or `cancelled`.

## Revision and ETag

The repository-owned `operationRevision` remains the authoritative revision.

The public response converts that revision through the accepted shared
precondition helper into a strong opaque ETag:

```text
operationRevision
  -> PublicResourcePreconditions
  -> ETag: "opaque-token"
```

Clients must not decode or manufacture the ETag.

A normal successful response is:

```text
200 OK
ETag: "opaque-token"
Cache-Control: no-store
```

The JSON body does not repeat the revision.

## Conditional safe read

Clients may send:

```text
If-None-Match: "opaque-token"
```

Semantics:

- missing condition -> normal `200`;
- non-matching condition -> normal `200` with the current ETag;
- matching strong or weak condition -> `304` with no response body;
- wildcard `*` on an existing visible operation -> `304`;
- malformed entity-tag condition -> `400 invalid_request`.

The ownership lookup happens before conditional comparison. A caller therefore
cannot use an ETag probe to discover another actor's operation.

## Error and method contract

The item route uses the common v1 Problem Details representation.

Relevant responses are:

```text
400 invalid_request
401 unauthorized
404 not_found
405 method_not_allowed + Allow: GET
503 service_unavailable
```

`POST`, `PUT`, `PATCH`, `DELETE`, `HEAD` and `OPTIONS` are not
operation-item mutations in this slice. The concrete item route reports
`405 method_not_allowed` with `Allow: GET`.

No cancellation API is implied by exposing `state=cancelled`.

## Capability discovery

The platform capability resource now includes:

```json
{
  "id": "public-api.durable-operations-read",
  "version": 1,
  "availability": "available"
}
```

Availability is truthful to runtime composition:

- registered operation lookup -> `available`;
- missing operation lookup -> `unavailable`.

The daemon registers the lookup before starting the HTTP listener and resets it
during shutdown before destroying the read service.

## HTTP header propagation

`If-None-Match` is propagated only through the safe GET path:

```text
HttpServerRequest
  -> TestHttpServer
  -> ApiRouter::handleClientGet
  -> PublicApiRuntime
```

The server performs case-insensitive header-name matching. The focused HTTP
regression proves a lowercase `if-none-match` request reaches the resource and
returns `304`.

No mutation header behavior is changed by this slice.

## Validation contract

The slice is covered at three levels:

1. Public resource unit contract:
   - truthful capability state;
   - reviewed JSON fields;
   - ETag emission;
   - strong, weak, wildcard and list `If-None-Match`;
   - `304`, malformed-condition `400`, hidden/missing `404`,
     unavailable `503`, and method `405`.

2. Security gate:
   - anonymous concrete operation GET rejected with public `401`;
   - authenticated concrete operation GET reaches routing.

3. HTTP integration:
   - authenticated GET returns JSON + ETag;
   - lowercase `if-none-match` round-trips through the real HTTP/router path
     and returns bodyless `304`.

The existing Phase-69 route inventory is updated so the new v1 item prefix
cannot appear or disappear without explicit review.

## Non-goals

This slice does **not** add:

- operation collection/listing;
- operation mutation submission;
- operation cancellation;
- public `Idempotency-Key` request handling;
- `If-Match` mutation enforcement;
- Timer/Recording public mutation routes;
- retry cadence or `Retry-After`;
- public mutation payloads or internal fingerprints;
- changes to pre-v1 routes;
- changes to Agent, SuiteBridge, media or VDR-native protocols.

## Next bounded 69.C work

After this item resource is accepted, 69.C can move to the first public mutation
submission contract only when one existing domain path can reuse the same
durable operation/idempotency authority without speculative retry or compatibility
fallback.

Until then, the operation item resource is a read/reconciliation surface only.
