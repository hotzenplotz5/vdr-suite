# Phase 69.A — Public API Route Inventory Kickoff

## Status

**ACTIVE — Phase 69 has explicitly started. Current slice: 69.A Public resource and route inventory.**

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

None at kickoff.

A route becomes public v1 only through an explicit `/api/v1/...` contract and
compatibility test. Existing behavior is not grandfathered into v1.

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

## Next bounded implementation step

After this inventory is green in CI, the next 69.A step is to define the first
stable resource set and the `GET /api/v1` contract root.

The contract root must not fabricate `serverVersion`. Phase 69 must first bind
that field to a truthful runtime/build identity rather than hard-code a temporary
value merely to satisfy the ADR example.

## Acceptance for this kickoff slice

- Phase 69/69.A is the canonical active status;
- the pre-v1 route baseline is machine checked;
- no runtime endpoint behavior changes;
- no existing client path is removed or redirected;
- Phase-62 through Phase-68 ownership and safety contracts remain unchanged.
