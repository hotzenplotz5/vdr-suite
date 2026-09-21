# VDR-Suite – Phase 11 Snapshot Read APIs

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Goal

Phase 11 introduced frontend-oriented read access to the daemon-owned VDR snapshot.

The goal is to make the snapshot usable by future frontends without forcing each API request to call VDR or RESTfulAPI directly.

Future clients include:

- web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD frontend
- third-party integrations

---

## Current Status

Phase 11 snapshot read API work is complete for the current domain set.

Implemented and verified domains:

- status
- channels
- timers
- events
- recordings

Latest verified implementation phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Verified locally with:

```text
make test-api-router
make test
```

Result:

- snapshot-backed `VdrController` read methods passed
- `ApiRouter` routes passed for all snapshot read domains
- `TestHttpServer` coverage passed
- full `make test` passed

---

## Snapshot Foundation

The snapshot foundation used by Phase 11 consists of:

- `VdrSnapshot`
- `SnapshotCache`
- `SnapshotCacheService`
- `ISnapshotAccessService`
- `SnapshotAccessService`
- `VdrSnapshotReadService`
- `VdrSnapshotReadJsonSerializer`
- `VdrController`
- `ApiRouter`
- `TestHttpServer`

The `VdrSnapshotReadService` reads from `ISnapshotAccessService` and exposes snapshot domains without owning the cache and without polling.

---

## Architecture Direction

The read path is:

```text
SnapshotCache
    ↓
SnapshotCacheService
    ↓
SnapshotAccessService
    ↓
VdrSnapshotReadService
    ↓
JSON serializer
    ↓
REST controller
    ↓
ApiRouter
    ↓
HTTP frontend/client
```

Responsibilities stay separated:

- `SnapshotCache` stores the latest snapshot.
- `SnapshotCacheService` updates complete snapshots or individual domains.
- `SnapshotAccessService` exposes cache access through `ISnapshotAccessService`.
- `VdrSnapshotReadService` provides frontend-oriented read methods.
- `VdrSnapshotReadJsonSerializer` converts snapshot domain objects into JSON.
- `VdrController` exposes read-only API responses.
- `ApiRouter` maps HTTP paths to controller methods.
- `TestHttpServer` verifies HTTP-level routing behavior.

No RESTfulAPI access should be added to Phase 11 read endpoints.

---

## Implemented Endpoints

The implemented read-only snapshot endpoints are:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/snapshot
GET /api/vdr/capabilities
GET /api/vdr/recordings
GET /api/vdr/timers
GET /api/vdr/channels
GET /api/vdr/events
```

The existing overview endpoint remains:

```text
GET /api/vdr/overview
```

Endpoint design rules:

- use snapshot-backed data
- return stable JSON structures
- do not trigger polling
- do not call RESTfulAPI directly
- keep controller logic thin
- keep JSON formatting in serializers
- keep future filtering and pagination possible

---

## Implemented Domain JSON Contracts

### Status

Status is exposed through `GET /api/vdr/status`.

The response includes the existing `VdrStatus` fields:

- `enabled`
- `mode`
- `host`
- `port`
- `state`

### Channels

Channels are exposed through `GET /api/vdr/channels`.

The response includes the existing `VdrChannel` fields:

- `id`
- `number`
- `name`
- `provider`
- `group`
- `radio`
- `encrypted`
- `enabled`

### Timers

Timers are exposed through `GET /api/vdr/timers`.

The response includes the existing `VdrTimer` fields:

- `id`
- `channelId`
- `eventId`
- `title`
- `subtitle`
- `startTime`
- `endTime`
- `priority`
- `lifetime`
- `enabled`
- `recording`

### Events

Events are exposed through `GET /api/vdr/events`.

The response includes the existing `VdrEvent` fields:

- `id`
- `channelId`
- `title`
- `subtitle`
- `description`
- `startTime`
- `endTime`
- `durationSeconds`
- `contentDescriptors`
- `parentalRating`

### Recordings

Recordings are exposed through `GET /api/vdr/recordings`.

The response includes the existing `VdrRecording` fields:

- `id`
- `title`
- `path`
- `startTime`
- `durationSeconds`
- `sizeMb`

---

## Test Strategy

Phase 11 uses layered tests:

- service-level tests for `VdrSnapshotReadService`
- serializer behavior through controller and router coverage
- controller tests for snapshot-backed responses
- router tests for REST path mapping
- HTTP server tests through `TestHttpServer`
- full `make test` validation before marking the phase complete

Optional real-backend validation remains separate from `make test` and should use explicit local targets, similar to the existing `test-local-*` pattern.

---

## Completed Implementation Order

Completed implementation order:

1. `VdrSnapshotReadService`
2. `VdrSnapshotReadService` tests
3. VDR source and test-target build integration
4. `VdrSnapshotReadJsonSerializer`
5. controller methods
6. router mappings
7. HTTP-level tests
8. channel JSON serialization
9. timer JSON serialization
10. event JSON serialization
11. recording JSON serialization
12. controller and router test updates
13. status documentation update

---

## Non-Goals

Phase 11 does not implement:

- multi-backend federation
- permissions
- write APIs
- live-update transport
- SSE/WebSocket event streaming
- new polling strategies
- direct RESTfulAPI calls from read endpoints

---

## Next Direction

The next architectural direction is a snapshot change feed.

The change feed should build on existing change-detection domain objects and should not immediately introduce SSE or WebSocket transport. Transport can be added later after the internal feed model is stable.
---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
