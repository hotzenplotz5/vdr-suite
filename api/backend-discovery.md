# Public-v1 Backend discovery — developer contract

## Navigation

- [API developer entry point](README.md)
- [OpenAPI 3.1 partial contract](openapi.json)
- [REST implementation boundary](rest/README.md)
- [Accepted Phase 69.F Backend collection](../docs/development/phase-69f-public-backend-collection.md)
- [Public API compatibility ADR](../docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)

## Status

Source-audited against `main` commit `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`, 2026-10-11. The Backend collection is accepted in Phase 69.F (PR #373) and implemented in the current source. This document describes **source behavior, not a check of the installed daemon**. Its machine-readable counterpart is [the partial OpenAPI specification](openapi.json).

## HTTP contract

```http
GET /api/v1/backends?limit=50&sort=backendId&order=asc
```

The query may contain only `limit` (1–100, default 50), `cursor` (opaque, nonempty, up to 4096 characters), `sort=backendId`, and `order=asc`. Repeated or unknown query fields, incorrect order/sort and malformed cursors are rejected. Ordering is always `backendId ASC`.

For each item the wire format exposes only `backendId`, `name`, `enabled` and `online`. Browser-only selection/write flags, backend transport details, provider types, VDR paths and secrets are **not public**.

Example successful response (illustrative IDs; shape verified in `PublicApiRuntime.cpp`):

```json
{
  "items": [{"backendId": "backend-a", "name": "Living Room", "enabled": true, "online": true}],
  "page": {"limit": 50, "nextCursor": null, "hasMore": false},
  "meta": {"partial": false},
  "links": {
    "self": "/api/v1/backends?limit=50&sort=backendId&order=asc",
    "next": null
  }
}
```

There is no collection ETag. Success responses use `Cache-Control: no-store`. Discovery reads the Suite-owned BackendRegistry and does not poll backends; `meta.partial` is therefore always `false`.

## Authentication and authorization

An authenticated actor is required. The security gate derives visible Backend IDs from **positive operation grants** and their backend scopes, rather than requiring a made-up `backends.view` permission. A wildcard `*` scope covers all configured Backends. `role.read-only` alone does not make a Backend visible. An authenticated actor without qualifying scopes gets an **empty 200 collection**, not an implicit default Backend.

Credential acquisition and transport are governed by the deployed VDR-Suite identity/Device security contracts; **this document does not prescribe a made-up Bearer or cookie scheme**. The OpenAPI entry records actor authentication as a vendor extension until the concrete security variants are documented and reviewed.

## Pagination for TV, Kodi and desktop clients

1. Start with `GET /api/v1/backends?limit=50` after authentication.
2. Render `items`. If `page.hasMore` is true, follow `links.next` or reuse the exact `page.nextCursor`. Do not decode, manufacture or store the `be1_`-prefixed cursor as a stable ID.
3. Continue until `page.hasMore` is false. A `409 cursor_expired` means the authorization scope changed; discard the cursor and restart from page one.
4. Do not silently retry against `/api/backends`: that unversioned browser route has a different and nonpublic contract.

## Error handling

| HTTP | Public code or origin | Client response |
| --- | --- | --- |
| 400 | `invalid_request` | Correct query or discard malformed cursor; no blind alternate-route fallback |
| 401 | `unauthorized` or security-gate authentication rejection | Establish valid actor credentials |
| 409 | `cursor_expired` | Restart traversal with a fresh first page |
| 503 | `service_unavailable` or security-gate availability rejection | Surface temporary failure; do not interpret as an empty Backend list |
| 405 | `method_not_allowed`, `Allow: GET` | Use GET; HEAD and OPTIONS are not substitutes for this endpoint |

Public REST handler errors use `application/problem+json`, with stable `code` plus `type`, `title`, `status` and `requestId`; `detail`, `instance` and `correlationId` may be present. Security-gate rejections may precede REST handling.

## Verified implementation and regression anchors

- [Source route/parser/serializer](rest/src/PublicApiRuntime.cpp)
- [Security permission gate](../core/security/include/SecurityHttpGate.h)
- [REST Backend collection tests](rest/tests/test_public_backend_collection.cpp)
- [Backend security tests](../core/security/tests/test_public_backend_collection_security.cpp)
- [JavaScript reference client](../clients/reference-js/public-v1-client.js) — `getBackends({query: {limit: 50}})`
- [Accepted phase contract and CI evidence](../docs/development/phase-69f-public-backend-collection.md)

When extending the wire contract, update the matching source tests, OpenAPI path/schemas, and this guide together. This documentation-only branch has **not** introduced an automated route-to-OpenAPI checker or changed production code.

## Back

- [API entry point](README.md)
- [Project README](../README.md)
