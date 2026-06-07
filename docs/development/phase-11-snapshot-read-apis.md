# VDR-Suite – Phase 11 Snapshot Read APIs

## Goal

Phase 11 introduces frontend-oriented read access to the daemon-owned VDR snapshot.

The goal is to make the snapshot usable by future frontends without forcing each API request to call VDR or RESTfulAPI directly.

Future clients include:

- web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD frontend
- third-party integrations

---

## Current Foundation

The current snapshot foundation already exists:

- `VdrSnapshot`
- `SnapshotCache`
- `SnapshotCacheService`
- `ISnapshotAccessService`
- `SnapshotAccessService`
- `VdrOverviewService` snapshot-backed overview path
- `VdrController` snapshot-backed overview route

Phase 11.0 adds the first dedicated frontend-read service:

- `VdrSnapshotReadService`

This service reads from `ISnapshotAccessService` and exposes snapshot domains without owning the cache and without polling.

---

## Architecture Direction

The intended read path is:

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
- serializers convert domain objects into JSON.
- controllers expose read-only API responses.
- `ApiRouter` maps HTTP paths to controller methods.

No RESTfulAPI access should be added to Phase 11 read endpoints.

---

## Planned Endpoints

The planned read-only snapshot endpoints are:

```text
GET /api/vdr/status
GET /api/vdr/recordings
GET /api/vdr/timers
GET /api/vdr/channels
GET /api/vdr/events
```

The existing endpoint remains:

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

## Test Strategy

Phase 11 starts with service-level tests:

- `test-vdr-snapshot-read-service`

API phases should add:

- serializer tests
- controller tests
- router tests
- HTTP server tests through `TestHttpServer`

Later optional real-backend validation should remain separate from `make test` and use explicit local targets, similar to the existing `test-local-*` pattern.

---

## Implementation Order

Recommended implementation order:

1. `VdrSnapshotReadService`
2. `VdrSnapshotReadService` tests
3. VDR source and test-target build integration
4. snapshot read JSON serializer
5. serializer tests
6. controller methods
7. router mappings
8. HTTP-level tests
9. status documentation update

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
