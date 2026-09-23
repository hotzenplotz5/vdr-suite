# Phase 69 — Public API Kickoff and Runtime Progress

## Status

**ACTIVE — 69.A and 69.B are accepted on `main`; current slice: 69.C Revision/precondition/idempotency exposure.**

Start baseline:

```text
main=35c9120472502976207152bd0c13d1b73807249f
phase68=COMPLETED
phase69=ACTIVE
slice=69.A
```

Binding architecture: [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md).

## Goal

Phase 69 stabilizes the client-independent public contract below `/api/v1`.
69.A first establishes what exists today so transition routes are not accidentally
promoted into public compatibility promises.

This slice does **not** change VDR behavior, Timer ownership, MediaSession
ownership, Teletext/HbbTV transport, Legacy OSD transport/input semantics or
SuiteBridge/Agent protocols.

## Repository truth found at kickoff

The live Phase-68 merge baseline already contains one ADR-0048 prerequisite that
the older ADR context described as missing: `ApiResponse` has arbitrary response
header storage and the HTTP server propagates those headers. Phase 69 therefore
must consume current repository truth rather than reimplement that foundation.

No `/api/v1` route exists at the Phase-69 start baseline.

The current REST surface is a mixture of:

- direct `ApiRouter` routes;
- delegated first-party runtime routes;
- historical aliases, especially `/api/vdr/...`;
- frontend/support routes and operator diagnostics;
- bounded control resources for media, Broadcast Companion and Legacy OSD;
- dynamic backend-scoped transition routes under `/api/backends/<backendId>/...`.

All of them are **pre-v1** until a Phase-69 slice explicitly defines the stable
resource identity, representation, authorization, error and compatibility
contract.

## 69.A classification

### Public v1

At kickoff there were no public-v1 routes. The first bounded runtime step now
establishes exactly:

```text
GET /api/v1
GET /api/v1/capabilities
```

The contract root exposes the public API major, a truthful daemon build identity,
the supported public API majors, caller authentication state and canonical links.
The capability resource is platform-level discovery only; it does not publish
Agent, plugin, media or Legacy OSD protocol versions.

Existing behavior is not grandfathered into v1. Any additional route requires an
explicit Phase-69 inventory and compatibility update.

### Pre-v1 compatibility candidates

Existing domain-facing routes such as Channels, EPG, Recordings, people/search,
Timers/SearchTimers, Jobs, Backends, metadata and history remain usable
transition routes. Their current response shape, alias names, offset conventions
and backend-native fields are not automatically stable public contracts.

### Private / transition support

The following families are not candidates for direct public freezing in their
current form:

- `/api/runtime...` diagnostics;
- `/api/epg/cache/...` cache/refresh internals;
- native-fuzzy maintenance endpoints;
- first-party remote/overlay transition actions;
- backend settings and metadata helper shapes assembled below
  `/api/backends/<backendId>/...`.

They may later gain deliberate public resources, remain private, or be replaced.

### Separate planes and bounded control resources

MediaSession control, Broadcast Companion control and Legacy OSD session/control
resources may receive public v1 control representations, but their underlying
media bytes, Agent protocol, plugin-local schemas and OSD frame/input data plane
remain separately versioned as required by ADR-0048.

## Dynamic backend-scoped transition shapes

The current source also assembles routes dynamically:

```text
/api/backends/<backendId>/recordings/metadata/{manual,trailer,search,seasons,episodes,assign,withdraw}
/api/backends/<backendId>/recordings/series-hierarchy
/api/backends/<backendId>/settings/media-transcode
/api/backends/<backendId>/settings/series-artwork
```

These are explicitly transition shapes, not stable v1 names.

## Inventory guard

`tools/check_phase69_public_api_inventory.py` pins the route literals and the
delegated `*ApiRuntime` owners present at the kickoff baseline.

The guard fails when:

- an inventoried route literal is added or removed without updating the Phase-69
  inventory;
- a new delegated API runtime owner is attached to `ApiRouter` without being
  brought under the inventory;
- a `/api/v1` route appears without an explicit Phase-69 inventory update.

This is a migration guard, not a promise that the pre-v1 routes are permanent.

## First bounded runtime step

The first stable v1 resource set is deliberately small: the contract root and
platform capability discovery only. No existing domain route is aliased into v1.

`serverVersion` comes from `VDR_SUITE_SERVER_VERSION`. Repository builds default
to `git-<short-commit>` from the exact source tree; packaging may override that
value with its truthful package release identity. This value is explicitly
separate from `apiVersion`, Agent protocol/software versions and SuiteBridge
plugin/schema versions.

69.A is accepted on `main`:

```text
PR #314 -> d34058220653496a35fb7e1c073cc6981f4c72db
PR #315 -> bca8ff3bfd1216bda39f29105911f64853bd263c
PR #315 CI -> 35818928861 / run #9074 -> SUCCESS (6/6)
```

No repeated real-yaVDR acceptance was required because 69.A introduced only
read-only contract discovery and repository guards.

## 69.B accepted implementation

Phase 62 already generates and validates the HTTP request/correlation context and
decorates final HTTP responses with `X-Request-ID` and, when present,
`X-Correlation-ID`. Phase 69.B reuses that authority; it does not create a
second request-ID generator.

The first 69.B runtime step is deliberately limited to the new public-v1 owner:

- propagate the accepted Phase-62 request/correlation identity into
  `PublicApiRuntime`;
- include the same identifiers on direct v1 responses;
- claim unknown `/api/v1/...` GET routes inside the public owner and return
  Problem Details-compatible `404 not_found` instead of falling through into
  pre-v1 routing;
- return Problem Details-compatible `405 method_not_allowed` plus `Allow: GET`
  for POST to the two current GET-only v1 resources;
- allow that method rejection through Phase-62 security without treating it as a
  business mutation;
- keep unknown/future v1 POST routes fail-closed at the Phase-62
  `security_policy_not_migrated` boundary until each real mutation receives an
  explicit authorization contract;
- leave every pre-v1 `/api/...` route unchanged.

PR #316 merged the first 69.B runtime step:

```text
PR #316 -> 72a637cc6f18fc1c1c1a77859abb3ab1f15e2b03
CI -> 35843808669 / run #9081 -> SUCCESS (6/6)
```

PR #317 merged the v1 security-error normalization step:

```text
PR #317 -> 5ff36d635072c63e1c46ccb377c830ec99988108
CI -> 35846333459 / run #9083 -> SUCCESS (6/6)
```

PR #318 merged the public unsupported-method boundary:

```text
PR #318 -> e9d59b87349d7e1be9356efc5cd656f41e11ea12
CI -> 35861961215 / run #9085 -> SUCCESS (6/6)
```

The remaining 69.B foundation work now centralizes the public Problem Details
representation itself:

- one lower-level `PublicProblemDetails` model/serializer is shared by the
  public REST runtime and the SecurityHttpGate;
- public security responses map internal Phase-62 reason codes onto stable
  ADR-0048 public categories instead of freezing transition/internal names such
  as `security_policy_not_migrated` into the client contract;
- audit/accountability retains the original internal reason code;
- pre-v1 error bodies remain unchanged;
- route-specific public details such as `instance` remain optional and are set
  only when the public owner has that route context.

This preserves the dependency direction: `core/security` does not depend on
`api/rest`; both consume a transport-level public error model from
`core/http`.

69.B is closed by [Phase 69.B Closeout](phase-69b-closeout.md). PR #319 merged the
shared public Problem Details model and stable public security-code mapping:

```text
PR #319 -> 0248db3d63626a87d391f7649a984adc97b45232
CI -> 35863518112 / run #9087 -> SUCCESS (6/6)
```

The active bounded slice is now 69.C. Retry intervals are not invented without
an owning operation/resource semantic; deprecation/sunset policy remains 69.E.

## 69.C accepted foundation and active resource

69.C has accepted two prerequisite checkpoints:

```text
PR #321 -> beb98f6edab39d962bd6415db7be21cf145e05cb
CI #9091 -> 35873370127 -> SUCCESS (6/6)

PR #322 -> 377f59e7b2d8e5ec3c7cbf22522c3c1789fd9849
CI #9094 -> 35876551406 -> SUCCESS (6/6)
```

PR #321 established the shared opaque ETag / conditional-request helper. PR #322
established one actor-scoped `MutationOperationReadService` over the single
DaemonRuntime-owned durable operation repository.

The active 69.C resource step is
[Public Operation Resource](phase-69c-public-operation-resource.md):

```text
GET /api/v1/operations/{operationId}
```

It is an authenticated actor-owned item read, not an operation collection or
mutation endpoint. It exposes a reviewed representation, derives a strong opaque
ETag from the durable operation revision and supports `If-None-Match -> 304`.
Cross-actor lookup preserves the same `404 not_found` result as a missing
operation. Internal idempotency keys, request/resource fingerprints, mutation
payloads and result references remain private.

## 69.A acceptance

- Phase 69 is active and 69.A is accepted;
- the pre-v1 route baseline and declared public-v1 route set are machine checked;
- only the read-only `GET /api/v1` and `GET /api/v1/capabilities` resource set
  was added in 69.A;
- no existing client path was removed or redirected;
- Phase-62 through Phase-68 ownership and safety contracts remain unchanged.
