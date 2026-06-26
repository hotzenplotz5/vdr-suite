# SearchTimer Preview EPG Cache Strategy

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](../adr/ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)

---

## Purpose

This document records the SearchTimer preview EPG input strategy after the Phase 54.1 real yaVDR validation and the Phase 54.3 warm-cache runtime foundation.

It explains why VDR-Suite must not fetch the full RESTfulAPI EPG dump during each interactive SearchTimer preview request.

---

## Verified Findings

Real yaVDR validation showed:

```text
inputEventCount: about 35,500
raw RESTfulAPI /events.json size: about 30 MB
raw /events.json download time: about 28 seconds
complete live probe time: about 34 seconds
SearchTimer title-only result count for "Amerika": 86
```

The important conclusion:

```text
SearchTimer matching correctness is not the bottleneck.
The EPG input path is the bottleneck.
```

The Phase 54.1 matcher fix verified that SearchTimer preview now preserves:

- compareTitle
- compareSubtitle
- compareSummary
- limit
- offset

---

## Problem

The slow path is:

```text
SearchTimer preview request
-> GET /events.json?from=now&timespan=14d
-> transfer about 30 MB
-> parse about 35,000 events
-> run local matcher
-> return result
```

This is acceptable for a diagnostic probe.

It is not acceptable for normal UI usage.

---

## Target Runtime Model

The target path is:

```text
Daemon/runtime
-> keeps backend-scoped EPG cache warm

SearchTimer preview request
-> reads warm EPG cache
-> applies SearchTimerPreviewService
-> returns result immediately
```

The preview path must not perform a full EPG dump fetch by default.

---

## Implemented Runtime Foundation

Phase 54.3 adds the first runtime-side cache refresh foundation.

Implemented components:

```text
SearchTimerPreviewEpgCache
SearchTimerPreviewEpgCacheRefreshService
SearchTimerPreviewEpgCacheRefreshServiceRegistry
SearchTimerPreviewEpgCacheRefreshController
ApiRouter refresh route
DaemonRuntime refresh controller wiring
```

The refresh service intentionally calls:

```text
VdrSnapshotBuilder::buildEvents(VdrEventQuery)
```

It must not call:

```text
VdrSnapshotBuilder::buildEvents()
```

This keeps the refresh path on the selective EPG-query contract instead of the full EPG dump path.

Default bounded refresh input:

```text
channelEventLimit: 50
```

---

## Manual Refresh Endpoint

Phase 54.3 exposes a read-only runtime refresh endpoint for explicit cache warming:

```text
POST /api/searchtimers/preview/cache/refresh?backend=default
POST /api/vdr/searchtimers/preview/cache/refresh?backend=default
```

Supported query parameters:

```text
backend=default
from=-1
timespan=7200
start=-1
limit=0
channelEventLimit=50
chevents=50
```

Successful response shape:

```json
{
  "backendId": "default",
  "status": "ready",
  "available": true,
  "eventCount": 50
}
```

Unavailable response shape:

```json
{
  "backendId": "default",
  "status": "unavailable",
  "available": false,
  "eventCount": 0
}
```

Unknown backend response shape:

```json
{
  "backendId": "missing",
  "status": "backend-not-found",
  "available": false,
  "eventCount": 0
}
```

This endpoint is intentionally not a SearchTimer mutation endpoint.

It does not create, update or delete SearchTimers.

---

## Cache Readiness Rules

SearchTimer preview must distinguish:

```text
cache ready
cache warming
cache stale
cache unavailable
```

A missing or empty EPG cache must not be rendered as a normal zero-match result.

A zero-match result is valid only when:

```text
EPG input was available
and
the matcher searched it successfully
and
no events matched the SearchTimer query
```

---

## Change Detection

Initial implementation should use the existing change-state polling model.

Expected behavior:

```text
eventsUpdate unchanged
-> keep current cache

eventsUpdate changed
-> mark cache stale
-> refresh cache outside the interactive preview request path
```

Future SSE support should use the same model.

SSE should signal changes.

SSE should not stream full EPG payloads.

---

## RESTfulAPI Extension Direction

A later RESTfulAPI patch should prefer a general change stream:

```text
GET /changes/stream
```

Preferred payload class:

```text
bootId
eventsUpdate
timersUpdate
recordingsUpdate
channelsUpdate
```

This keeps the stream useful for more than SearchTimer preview.

---

## Implementation Boundary

Phase 54.3 implements the VDR-Suite cache side first.

It does not require an immediate RESTfulAPI plugin patch.

RESTfulAPI SSE remains a follow-up phase after the cache contract exists.

The current refresh endpoint is an explicit runtime trigger. It is not yet a scheduler, not yet a worker loop and not yet an SSE-driven invalidation handler.

---

## Back

- [Back to Development Index](index.md)
