# SearchTimer Preview EPG Cache Strategy

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](../adr/ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)

---

## Purpose

This document records the SearchTimer preview EPG input strategy after the Phase 54.1 real yaVDR validation.

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

Phase 54.3 should implement the VDR-Suite cache side first.

It should not require an immediate RESTfulAPI plugin patch.

RESTfulAPI SSE should remain a follow-up phase after the cache contract exists.

---

## Back

- [Back to Development Index](index.md)
