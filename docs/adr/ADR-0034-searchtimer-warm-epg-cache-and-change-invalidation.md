# ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation

## Navigation

- [ADR Index](index.md)
- [Development Index](../development/index.md)
- [SearchTimer Preview EPG Cache Strategy](../development/searchtimer-preview-epg-cache-strategy.md)
- [Roadmap](../planning/roadmap.md)

---

## Status

Accepted.

---

## Context

SearchTimer preview must be usable as an operator-facing truth source before creating, updating or deleting backend SearchTimers.

Phase 54.1 verified that the VDR-Suite SearchTimer preview matcher can correctly preserve SearchTimer comparison options such as title-only matching.

Real yaVDR validation showed:

- live VDR EPG input was available
- a 14-day RESTfulAPI `/events.json` query returned about 35,000 events
- the raw RESTfulAPI transfer was about 30 MB
- the raw RESTfulAPI download took about 28 seconds
- the complete local probe took about 34 seconds
- the matcher itself produced correct title-only SearchTimer results

This means the correctness problem is solved, but the interactive data path is not acceptable.

The problem is not that VDR-Suite cannot match SearchTimers.

The problem is that SearchTimer preview must not fetch the full EPG dump during every interactive preview request.

---

## Decision

SearchTimer preview must use a warm EPG input source instead of performing a full live RESTfulAPI EPG dump per request.

The standard interactive preview path must be:

```text
SearchTimer preview request
-> use warm EPG cache
-> apply VDR-Suite SearchTimer matcher
-> return preview result
```

The standard interactive preview path must not be:

```text
SearchTimer preview request
-> fetch /events.json for 14 days
-> transfer tens of megabytes
-> parse all events
-> then search
```

The warm EPG cache must be treated as a backend-scoped runtime data source.

The cache must expose explicit readiness metadata.

A preview must distinguish between:

- cache ready and searched
- cache empty or not ready
- cache stale
- slow diagnostic live fallback, if explicitly enabled later

A zero-match result must not be used to hide missing EPG input.

---

## Change Detection Strategy

VDR-Suite already has a change-state concept for VDR runtime state.

The short-term implementation should use the existing change-state polling path to detect EPG changes.

The preferred model is:

```text
change-state indicates eventsUpdate changed
-> mark backend EPG cache stale
-> refresh EPG cache in daemon/runtime path
-> SearchTimer preview searches warm cache
```

The long-term implementation may use RESTfulAPI SSE as a push-based change signal.

SSE should carry invalidation/change metadata, not the complete EPG payload.

Preferred future SSE shape:

```text
event: change-state
data: {
  "bootId": "...",
  "eventsUpdate": 123,
  "timersUpdate": 456,
  "recordingsUpdate": 789,
  "channelsUpdate": 111
}
```

The SSE stream should be used as a refresh trigger.

It must not stream the full EPG as normal preview input.

---

## RESTfulAPI Strategy

No immediate RESTfulAPI patch is required for the first VDR-Suite implementation.

Phase 54.3 should work with the existing RESTfulAPI and change-state polling model first.

A later phase may patch RESTfulAPI to expose a general change stream, preferably:

```text
GET /changes/stream
```

The stream should report changes for:

- boot id
- events
- timers
- recordings
- channels

A narrower EPG-only stream can be considered later, but a general change-state stream is more useful for VDR-Suite runtime synchronization.

---

## Consequences

Positive consequences:

- SearchTimer preview can become interactive and frontend-safe.
- VDR-Suite keeps its own tested SearchTimer matching semantics.
- Preview no longer depends on downloading large EPG JSON bodies per request.
- Missing EPG input becomes visible as a cache readiness problem instead of a false zero-result preview.
- Polling and future SSE can share the same invalidation model.

Trade-offs:

- The daemon must manage backend-scoped EPG cache freshness.
- Startup may need a cache warm-up phase before SearchTimer preview is ready.
- The cache may consume memory proportional to the configured EPG window.
- Native epgsearch may still be faster for some backends and should remain a future capability-specific path.

---

## Implementation Guidance

Phase 54.3 should add the first implementation step:

- define a backend-scoped EPG cache service
- expose cache readiness and freshness metadata
- populate the cache outside the interactive preview request path
- wire SearchTimer preview to the warm cache
- return explicit diagnostics when the cache is not ready
- preserve the Phase 54.1 comparison-option behavior
- keep real SearchTimer mutation out of this cache phase

A later phase may add:

- cache refresh scheduling
- eventsUpdate-driven invalidation
- SSE change stream client support
- RESTfulAPI `/changes/stream` patch
- backend-native epgsearch fast path

---

## Back

- [Back to ADR Index](index.md)
- [Back to Development Index](../development/index.md)
