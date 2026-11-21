# Phase 69.F — Public-v1 Discovery Reference Client

## Status

**ACCEPTED — eighth bounded 69.F slice via PR #376.**

Accepted evidence:

```text
PR #376
accepted head=67d103443ec247eb7c7e3d395386510ae33b8fe4
merge/main=88b7e01d5fb327fea3f54d43f31202dc7e303d98
CI #9341 / run 36344690345 = SUCCESS (6/6)
```

Baseline:

```text
main=19077e7110fdb8111aead43e15b6962767ac6234
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #375
accepted head=dfba6b0459c3521fe534900c03b383f539fa55ce
merge/main=19077e7110fdb8111aead43e15b6962767ac6234
CI #9339 / run 36343460569 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

## Fresh live audit after PR #375

The fallback-hardening sequence is complete: the current matrix records no
deferred browser route fallback.

The remaining 69.F problem is different. The repository exposes eight stable
public-v1 method/resource contracts, but the live tree contains:

- no `clients/` client layer;
- no OpenAPI/Swagger description;
- no public-v1 SDK/reference implementation;
- no browser wrapper that can be safely switched wholesale to v1.

The last point is intentional. The matrix proves the existing browser Backend,
Channel and Timer surfaces are richer or non-equivalent. Migrating those
wrappers merely to make them say `/api/v1` would change product semantics.

## Decision

Establish the smallest executable external-client seam using only discovery
contracts whose public semantics are already stable:

```text
GET /api/v1
GET /api/v1/capabilities
GET /api/v1/backends
```

The implementation lives at
`clients/reference-js/public-v1-client.js`.

This is a **reference client**, not a package-release decision. JavaScript is
used because the repository already validates JavaScript with Node without
introducing a package manager. Android, TV, desktop and Kodi implementations may
use different languages while preserving the same HTTP contracts.

## Client boundary

The reference client:

- accepts an injected `fetch` implementation;
- accepts caller-owned headers and credentials instead of inventing auth;
- preserves public Problem Details fields including stable `code`, request ID
  and correlation ID;
- supports only the Backend collection's documented `limit`, `cursor`,
  `sort=backendId` and `order=asc` query contract;
- performs exactly one request per operation;
- contains no browser-session, EPG-cache, Recording-cache, provider or plugin
  route knowledge;
- contains no legacy `/api/vdr/...` route;
- performs no automatic retry or alternate-route fallback.

## Explicit non-scope

- No npm or other package publication.
- No OpenAPI generation.
- No browser Web Client API migration.
- No new authentication or credential-provisioning scheme.
- No Channel or TimerAssignment reference-client methods yet.
- No new public-v1 resource.
- No EPG, Recording, Metadata, SearchTimer or Genre promotion.
- No daemon/native VDR behavior change.

## Regression contract

The Node regression proves:

- exact public discovery URLs;
- Backend keyset query encoding;
- caller-owned authorization/correlation headers are preserved;
- unsupported Backend query parameters are rejected before dispatch;
- public 503 Problem Details remain structured client errors;
- failed requests are not retried;
- no legacy/private route literal enters the reference client.

Architecture guards tie the implementation to the accepted public-v1 inventory,
keep the bundled browser on its non-equivalent transition routes, preserve the
eight-resource stable-v1 count, and wire the slice into fast CI, architecture
and Phase-69 validation.

No real yaVDR acceptance is required because this slice adds client-side
reference code and documentation only. Hosted CI on the exact head is the
Ready-for-Review gate.
