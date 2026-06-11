# Phase 21.0 - Real VDR Runtime Polling Findings

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This document records the Phase 21.0 architecture finding from real VDR runtime validation.

The purpose of this phase is deliberately architectural, not an implementation step.

No daemon runtime poll loop should be introduced before the real VDR polling and EPG performance implications are understood and documented.

---

## Real VDR Runtime Observation

A real VDR runtime test was performed on 2026-06-11 against a VDR backend exposed through RESTfulAPI.

The daemon successfully started against the real backend and the RESTfulAPI endpoints were reachable.

Observed backend state:

```text
backendId: default
snapshotAvailable: true
mode: restfulapi
host: 127.0.0.1
port: 8002
channelCount: 342
eventCount: 36136
timerCount: 0
recordingCount: 973
```

The daemon served VDR-Suite HTTP on:

```text
127.0.0.1:18080
```

The following VDR-Suite endpoints returned successfully after startup:

```text
GET /api/vdr/health
GET /api/vdr/status
GET /api/vdr/snapshot
GET /api/vdr/changes
GET /api/vdr/live
```

After startup, `GET /api/vdr/changes` and `GET /api/vdr/live` were empty.

A real SVDRP channel switch was then performed:

```text
svdrpsend CHAN 1
svdrpsend CHAN 2
```

The channel switch succeeded on the VDR side, but the VDR-Suite change and live endpoints remained empty.

---

## Runtime Polling Finding

The current daemon performs an initial poll during runtime initialization.

It does not yet run a cyclic runtime poll loop after startup.

This is an important implementation finding, but it must not be solved by blindly adding a fixed interval full snapshot refresh.

---

## EPG Performance Finding

The real VDR test exposed the cost of full EPG retrieval through RESTfulAPI:

```text
/events.json size: approximately 31.6 MB
/events.json item count: 36,136 EPG events
HTTP GET /events.json: approximately 26.7 seconds
VdrSnapshotBuilder event build: approximately 32.3 seconds
```

This makes `/events.json` the dominant cost in the current real snapshot path.

An initial EPG snapshot is acceptable.

Regular full EPG polling is not acceptable for real systems.

---

## Domain Cost Classification

VDR-Suite must not treat all data domains as equally cheap to refresh.

The real VDR result establishes the following domain classification:

```text
Lightweight domains
-------------------
Status
Timers
Recordings

Medium domains
--------------
Channels

Heavy domains
-------------
Events / EPG
```

Future data domains must use the same classification rule instead of being added to a generic poll-everything loop.

---

## Metadata Implication

The EPG finding is the first visible real-world case of a heavy domain.

The same architectural rule will later affect the metadata layer.

Future metadata-related domains may also be expensive:

```text
Recording metadata
Scraper results
Series metadata
Movie metadata
Poster images
Fanart images
Preview images
Preview streams
Media stream metadata
```

These domains must not be refreshed by regularly reloading or re-scraping everything.

The correct direction is:

```text
Recording or source changed
  -> mark affected metadata as dirty
  -> refresh only affected metadata
  -> update cache or database
  -> publish change feed entry
  -> publish live update event
```

The incorrect direction is:

```text
Periodically re-scrape all recordings and all media metadata
```

---

## Architecture Rule

VDR-Suite must separate change detection from expensive data refresh.

A cheap change signal may be polled regularly.

A heavy data domain must only be refreshed when there is a concrete reason, and preferably only in the affected subset.

Architecture rule:

```text
Do not build a daemon loop that periodically rebuilds a full VDR snapshot.

Do not regularly poll /events.json as part of a fixed runtime interval.

Use cheap change-state checks, dirty flags, push signals or backend-specific hints to decide whether a heavy domain needs refresh work.
```

---

## Correct Runtime Direction

The future daemon runtime loop should be built around cheap change-state polling first:

```text
Runtime loop
  -> getChangeState()
  -> ChangeDetection
  -> SnapshotRefreshPlanner
  -> refresh only affected domains
  -> append SnapshotChangeFeed entries
  -> publish LiveUpdateEvent entries
```

For heavy domains, the refresh planner must be conservative.

Events / EPG should be handled as a special heavy domain until a better strategy exists.

---

## RestfulAPI Event Stream Direction

RESTfulAPI SSE or event stream integration should be considered as a future optimization, not as a core architectural dependency.

The VDR-Suite backend boundary must remain backend-neutral.

Correct boundary:

```text
IVdrAdapter / backend boundary
  -> polling fallback
  -> future push sources
       -> RESTfulAPI event stream
       -> SVDRP-related notifications
       -> plugin bridge events
```

SSE may provide dirty hints or change hints.

SSE must not replace:

```text
SnapshotCache
VdrSnapshotBuilder
ChangeDetection
SnapshotRefreshPlanner
SnapshotChangeFeed
LiveTransport
```

Live transport remains above the snapshot change feed.

Backend-specific event streams remain below or beside the adapter boundary as optimization sources.

---

## Phase 21.0 Decision

Phase 21.0 is an architecture validation phase.

The next implementation work must not be a naive poll loop.

Before implementing cyclic runtime polling, VDR-Suite must document and preserve these constraints:

- initial full snapshot remains valid
- cheap change-state polling is acceptable
- full EPG refresh is expensive
- Events / EPG is a heavy domain
- metadata, images and previews may become heavy domains later
- future polling must be domain-aware
- future push integration must remain backend-neutral
- live transport must consume snapshot change feed entries, not own backend polling

---

## Recommended Next Phases

```text
Phase 21.1 - RESTfulAPI Event Stream Strategy
Phase 21.2 - Domain-aware Runtime Poll Loop Foundation
Phase 21.3 - Heavy Domain Refresh Policy
```

Phase 21.1 should document push-based change hints as backend-specific optimizations.

Phase 21.2 should add a daemon loop only after the domain-aware strategy is clear.

Phase 21.3 should define how Events / EPG, metadata, images and preview data are protected from expensive full refresh behavior.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
