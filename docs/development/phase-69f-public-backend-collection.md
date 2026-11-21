# Phase 69.F — Public Backend Collection

## Status

**ACCEPTED — fifth bounded 69.F slice via PR #373.**

Accepted evidence:

```text
PR #373
accepted head=096a4d2f1e011b876f80a38ef7e7b66ca04f0d83
merge/main=64798ee1bd51694d70a01432d6260b4f84dfa26b
CI #9335 / run 36338659080 = SUCCESS (6/6)
```

Baseline:

```text
main=ca7ddce8c1f4dcd94960c87d49c87533c5b0630b
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #372
accepted head=97739a82b59913edf268bc33beece1b074f58aec
merge/main=ca7ddce8c1f4dcd94960c87d49c87533c5b0630b
CI #9324 / run 36335165236 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0061 Actor Permissions, Federation and Client Access](../adr/ADR-0061-actor-permissions-federation-client-access.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

## Root-cause / architecture proof

The accepted client-contract matrix exposed a concrete platform dependency:

```text
GET /api/v1/channels?...        requires explicit backendId
GET /api/v1/timer-assignments  requires explicit backend
but independent clients had no stable public Backend discovery resource.
```

The repository already had `GET /api/backends`, but that route is a pre-v1
first-party presentation contract. Its serializer exposes browser-oriented
selection/write/capability state such as `frontendSelector`, write flags,
access mode and backend capability internals. Freezing that shape as public v1
would make browser implementation details a long-term compatibility promise.

Phase 69.D had therefore correctly deferred Backends until actor-safe filtering
and a stable representation were defined.

## Endpoint

```http
GET /api/v1/backends
```

Optional query members:

```text
limit   default 50, minimum 1, maximum 100
cursor  opaque keyset cursor
sort    backendId
order   asc
```

Unknown query members, duplicate query members, unsupported sort/order values or
invalid limits fail with `400 invalid_request`.

Ordering is stable:

```text
backendId ASC
```

The cursor prefix is `be1_`. It is opaque and binds both the last
`backendId` and the normalized authorization scope used to produce the page.

## Authorization — reuse existing grant scopes

This slice deliberately does **not** invent a `backends.view` permission.

ADR-0061 already defines permissions as operation grants scoped to one or more
backends. Backend discovery answers a narrower question: which configured
backend identities may this already-authenticated actor know about in order to
address resources it is otherwise allowed to consume?

The SecurityHttpGate therefore derives discovery scope from the actor's already
resolved grants:

- every positive grant with a non-empty backend scope contributes that scope;
- `role.read-only` contributes **no** scope because it is a mutation
  restriction, not a positive resource-access grant;
- exact backend scopes reveal only those configured backends;
- wildcard backend scope `*` reveals all configured backends;
- duplicate scopes are normalized away;
- an authenticated actor with no positive backend scope receives a valid empty
  collection;
- unavailable permission-grant resolution fails closed with `503`.

This reuses the existing Phase-62/ADR-0061 grant model rather than creating a
parallel Backend-discovery ACL.

The cursor is authorization-scope-bound. Reusing a cursor after the effective
grant scope changes returns:

```text
409 cursor_expired
```

A client must restart traversal from the first page under the new grant set.

## Stable representation

Each item intentionally contains only:

```json
{
  "backendId": "backend-a",
  "name": "Living Room",
  "enabled": true,
  "online": true
}
```

The public contract does **not** expose:

- backend connection host/port or transport configuration;
- `frontendSelector`;
- `accessMode` or the legacy `canWrite*` presentation flags;
- provider/backend implementation type (for example `restfulapi` or Agent transport identity);
- provider/VDR capability implementation details;
- Agent credentials, ownership internals or SuiteBridge state.

There is no Backend item route in this slice, therefore items do not invent a
`self` link.

## Collection envelope

The response uses the accepted Phase-69.D collection conventions:

```json
{
  "items": [],
  "page": {
    "limit": 50,
    "nextCursor": null,
    "hasMore": false
  },
  "meta": {
    "partial": false
  },
  "links": {
    "self": "/api/v1/backends?limit=50&sort=backendId&order=asc",
    "next": null
  }
}
```

The source is the local Suite-owned `BackendRegistry`. Discovery performs no
backend I/O and is not a federated fan-out, so partial-source semantics do not
apply and `meta.partial` is always false.

The collection intentionally has no ETag because there is no owning durable
Backend-set revision in the current runtime.

## Failure semantics

- unauthenticated -> `401 unauthorized`;
- malformed query/cursor -> `400 invalid_request`;
- cursor authorization-scope mismatch -> `409 cursor_expired`;
- permission-grant resolution unavailable -> `503 service_unavailable`;
- Backend collection runtime unavailable or invalid runtime output ->
  `503 service_unavailable`;
- POST/PUT/PATCH/DELETE/HEAD/OPTIONS -> `405 method_not_allowed`,
  `Allow: GET`.

Public problem responses retain ADR-0048 structured status/code semantics.

## Discovery links and capability

The additive public root link includes:

```text
links.backends = /api/v1/backends
```

`GET /api/v1/capabilities` advertises:

```text
public-api.backends-read / version 1
```

with runtime availability derived from Backend collection composition.

## Legacy compatibility and browser boundary

`GET /api/backends` and its related first-party routes remain retained pre-v1
transition routes.

They are **not deprecated** by this slice because the new public resource is not
a drop-in replacement for the bundled browser's richer Backend selector/default/
snapshot behavior. `deprecatedAliases` remains empty.

The bundled browser is not migrated in this slice. New TV, mobile, desktop,
Kodi and automation clients may use `GET /api/v1/backends` as the stable
Backend identity discovery entry point.

## Regression contract

Focused runtime tests prove:

- actor-scoped filtering;
- wildcard discovery;
- authenticated empty collection;
- stable `backendId ASC` keyset pagination;
- `be1_` cursor generation;
- authorization-scope changes expire an old cursor;
- malformed cursor and invalid query handling;
- unavailable runtime failure;
- method mismatch;
- no ETag;
- no legacy frontend/capability fields.

Security tests prove:

- two positive operation grants expose exactly their backend scopes;
- `role.read-only` does not create Backend visibility;
- wildcard grants remain wildcard discovery scope;
- scoped `role.admin` contributes its backend scope;
- anonymous access is denied with a public-v1 problem response.

Architecture guards prove runtime composition, shutdown reset, inventory,
matrix advancement, minimal representation and that no `backends.view`
permission or legacy deprecation is invented.

## Acceptance boundary

This slice changes the daemon/public HTTP control plane only.

It does not change native VDR state, Timer behavior, SuiteBridge/Agent
transport, Home composition, EPG lifecycle or LiveTV playback. Real-yaVDR
native-effect acceptance is therefore not required.

Acceptance requires the exact branch head to pass the complete hosted CI graph.
Merge still requires explicit user approval.
