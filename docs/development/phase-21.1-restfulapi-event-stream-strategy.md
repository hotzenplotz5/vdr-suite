# Phase 21.1 - RESTfulAPI Event Stream Strategy

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

This document defines the Phase 21.1 architecture work for RESTfulAPI event stream strategy and selective EPG access.

Phase 21.1 is not an implementation phase yet.

The purpose is to define how backend-specific event streams and existing RESTfulAPI event filters can provide precise change hints and selective data access without replacing the snapshot architecture.

---

## Core Rule

Event streams provide change hints.

Event streams do not provide authoritative domain data.

Authoritative data remains snapshot-based or selectively queried through backend adapter boundaries.

Correct direction:

```text
Backend-specific event stream
  -> change hint
  -> refresh planning
  -> selective snapshot or domain refresh
  -> snapshot change feed
  -> live transport
```

Incorrect direction:

```text
Backend-specific event stream
  -> live transport directly
```

---

## Backend Neutrality

VDR-Suite must not become directly dependent on RESTfulAPI SSE.

RESTfulAPI event streams are a possible optimization for RESTfulAPI-backed adapters.

They are not the core architecture.

Possible future hint sources:

```text
PollingChangeHintSource
RestfulApiEventStreamSource
SvdrpNotificationSource
PluginBridgeChangeHintSource
```

All hint sources must feed the same domain-aware refresh strategy.

---

## Change Hint Semantics

A change hint describes that something may have changed.

A change hint does not carry the complete domain payload.

Examples:

```text
StatusChanged
ChannelsChanged
TimersChanged
RecordingsChanged
EpgChanged
```

The refresh planner decides what to refresh.

---

## RESTfulAPI EPG Reality Check

RESTfulAPI already has selective EPG access capabilities.

The current expensive behavior comes from requesting the full endpoint without constraints:

```text
GET /events.json
```

On the tested real VDR this produced:

```text
about 31.6 MB JSON
about 36,136 EPG events
about 26.7 seconds HTTP time
about 32.3 seconds snapshot build time
```

This does not mean that RESTfulAPI cannot provide more selective EPG data.

The existing `events.cpp` implementation reads query options such as:

```text
timespan
from
start
limit
chevents
chfrom
chto
only_count
```

It also accepts a channel identifier through the `/events/<channel_id>` path form and an event identifier through the second path parameter.

The implementation uses VDR-internal schedule access:

```text
LOCK_CHANNELS_READ
LOCK_SCHEDULES_READ
Schedules->GetSchedule(...)
Schedule->Events()->First()
Schedule->Events()->Next(...)
```

Therefore RESTfulAPI is already able to perform backend-local filtering before JSON is emitted.

---

## Selective RESTfulAPI EPG Access Strategy

VDR-Suite should not treat `/events.json` as the default runtime EPG source.

Runtime EPG access should prefer selective RESTfulAPI queries.

Candidate access patterns:

```text
Current or near-future EPG window:
GET /events.json?from=<unix-time>&timespan=<seconds>
```

```text
Limited number of events per channel:
GET /events.json?from=<unix-time>&timespan=<seconds>&chevents=<count>
```

```text
Single channel EPG window:
GET /events/<channel-id>.json?from=<unix-time>&timespan=<seconds>
```

```text
Single channel with limited events:
GET /events/<channel-id>.json?from=<unix-time>&timespan=<seconds>&chevents=<count>
```

```text
Counting or cheap validation:
GET /events.json?from=<unix-time>&timespan=<seconds>&only_count=true
```

The exact supported URL forms must be validated against the installed RESTfulAPI version before implementation.

---

## Direct VDR Access Boundary

Live-like behavior should happen inside the VDR plugin process, not inside VDR-Suite.

Correct architecture:

```text
VDR-Suite
  -> RESTfulAPI adapter
  -> RESTfulAPI HTTP endpoint with selective query
  -> RESTfulAPI plugin code
  -> VDR schedules and channels in memory
```

Incorrect architecture:

```text
VDR-Suite
  -> direct VDR memory access
```

VDR-Suite remains an external daemon.

RESTfulAPI remains the backend-local access layer for RESTfulAPI-backed VDR nodes.

---

## Heavy Domain Rule

Events / EPG remains a heavy domain.

A hint such as `EpgChanged` must not automatically trigger a full `/events.json` refresh.

Instead, the hint should mark the EPG domain as dirty and defer refresh decisions to a heavy-domain policy.

Future heavy domains follow the same rule:

```text
Metadata
Posters
Fanart
Preview images
Preview streams
Scraper-derived data
```

---

## Relationship to Live Transport

Live transport remains above the snapshot change feed.

Live transport publishes already validated snapshot change feed entries.

It must not own:

- backend polling
- backend event stream parsing
- snapshot generation
- refresh planning
- heavy-domain policy
- RESTfulAPI query selection

---

## Implementation Implications

The next implementation phase should not begin with a naive daemon poll loop.

It should first define the adapter-facing EPG access model.

Important implementation candidates:

```text
RestfulApiVdrAdapter selective EPG requests
DomainAwareRefreshPolicy
EpgRefreshWindow
HeavyDomainRefreshDecision
SelectiveSnapshotRefresh
```

Required validation before code:

```text
curl /events.json?from=<now>&timespan=3600
curl /events.json?from=<now>&timespan=7200&chevents=2
curl /events/<channel-id>.json?from=<now>&timespan=86400
curl /events.json?from=<now>&timespan=3600&only_count=true
```

Validation must record:

```text
response size
HTTP time
number of events returned
whether channel and event filters behave as expected
whether only_count avoids payload generation sufficiently
```

---

## Current Phase 21.1 Status

Phase 21.1 is currently the active architecture focus.

It is not yet marked as completed.

Current completed phase remains:

```text
Phase 21.0 - Real VDR Runtime Polling Findings
```

Current implementation focus remains:

```text
Phase 21.1 - RESTfulAPI Event Stream Strategy
```

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
