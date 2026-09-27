# Phase 69.D — Federated Public Channel Collection

## Status

**IMPLEMENTED CANDIDATE — second bounded 69.D collection slice.**

Baseline:

```text
main = 45ef8e3665082c2b1bb4e3e7f54c67e1e18833d6
previous accepted slice = GET /api/v1/timer-assignments?backend={backendId}
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0020 Multi-Source Federation Architecture](../adr/ADR-0020-multi-source-federation-architecture.md)
- [ADR-0013 Permission Model](../adr/ADR-0013-permission-model.md)
- [Phase 63 Channel Observation Ingestion](phase-63-channel-observation-ingestion.md)

## Why this is the next 69.D slice

The first 69.D collection proved the generic collection envelope, bounded
response limit, keyset pagination and explicit failure for one Suite-owned
repository source. It cannot prove ADR-0048 partial multi-backend reads because
all TimerAssignment rows come from one SQLite authority; inventing independent
per-backend TimerAssignment failures would create false federation semantics.

The live inventory leaves Channels as the narrowest existing domain that
already has stable `(backendId, channelId)` identity,
`VdrSnapshotReadService`, `BackendRegistryService` and the accepted
`channels.view` permission without coupling Phase 69.D to Home, Recording
metadata fanout or LiveTV.

This slice does not promote `/api/vdr/channels`. It defines a new public-v1
collection deliberately.

## Endpoint and authorization

```http
GET /api/v1/channels?backendId=backend-a&backendId=backend-b
```

At least one explicit `backendId` is required. Repeating it selects the
aggregate source set. Omission is invalid; it does not mean a default backend
or all discoverable backends. At most 16 sources may be requested.

Each requested backend is authorized independently with
`channels.view@backend`. Any authorization failure rejects the whole request
before collection evaluation. Unauthorized backend identities never appear in
`meta.sources`.

## Identity and representation

The public identity is `(backendId, channelId)`. Channel number, name,
provider and group are attributes, not identity. Items expose:
`backendId`, `channelId`, `channelNumber`, `name`, `provider`,
`groupName`, `radio`, `encrypted`, and `enabled`.

No adapter URL, host/IP, plugin identity, pointer, channel-list position or
transport identity is exposed.

## Ordering and pagination

Canonical ordering is:

```text
backendId ASC, channelId ASC
```

Pagination is keyset-only: default 50, minimum 1, maximum 100, no OFFSET. The
opaque cursor uses prefix/version `ch1_` / `channels/1`, binds the normalized
requested backend scope and last `(backendId, channelId)`, and is re-authorized
on every page. Malformed cursors return `400 invalid_request`; a cursor whose
backend scope no longer matches returns `409 cursor_expired`.

The source snapshots already exist in memory. The public response is bounded to
`limit`; this slice does not invent a second Channel repository just to impose
source-level SQL pagination. It is not a snapshot-isolation contract.

## Partial results

A source is useful only when the Backend exists, is enabled and online, and
`VdrSnapshotReadService` has a valid Channel snapshot. A successful source,
including an authoritative empty set, has state `ok`. An unavailable source
has state `unavailable` and code `backend_unavailable`.

If at least one source succeeds and another fails, the response is `200` with
`meta.partial=true` and explicit `meta.sources`. A failed source is never
silently omitted. If all requested sources fail, the endpoint returns
`503 backend_unavailable`. If all succeed, `meta.partial=false`.

Responses use `Cache-Control: no-store`. There is no collection ETag because
there is no durable federation-wide Channel collection revision owner.

## Existing authorities and non-scope

The runtime only composes `BackendRegistryService` and
`VdrSnapshotReadService` into the existing `PublicApiRuntime` callback
boundary. It adds no repository, polling loop, cache, provider selector,
lifecycle owner or fallback route.

This slice changes none of Home, Home Rails, Recording discovery/metadata
fanout, LiveTV, Live Preview, EPG aggregation, Timer mutation, SuiteBridge,
Agent transport, snapshot polling cadence or legacy `/api/vdr/channels`.

No real-yaVDR native-effect acceptance is required for this read-only contract.

## 69.D remainder

After this slice the two ADR-0048 runtime obligations are represented: the
TimerAssignment collection proves single-source keyset semantics, and Channels
prove federated partial-result semantics. Current repository evidence therefore
leaves one bounded 69.D closeout audit/guard slice. Additional domain promotion
is not required merely to repeat the same collection mechanics.
