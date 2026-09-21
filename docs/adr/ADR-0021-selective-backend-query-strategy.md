# ADR-0021: Selective Backend Query Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current Project Status](../development/current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Status

Accepted

---

## Context

Real VDR validation showed that full-domain transfers are too expensive for normal runtime behavior.

The most visible example is EPG data. A full RESTfulAPI `/events.json` response on the tested real VDR produced a very large payload and long processing time.

Established VDR frontends such as `live` usually request or display only the information needed for the current view.

VDR-Suite must therefore avoid treating full-domain transfers as the default runtime model for heavy domains.

The same rule will later apply to metadata, posters, fanart, preview images, preview streams and scraper-derived data.

---

## Decision

VDR-Suite will prefer selective backend queries over full-domain transfers whenever possible.

The performance target is that requesting equivalent user-visible information should require backend work comparable to established VDR frontends such as `live`.

Full-domain transfers are allowed for explicit diagnostics, explicit full refresh operations, initial bootstrap where no cheaper alternative exists, and controlled background work with a clear heavy-domain policy.

Full-domain transfers must not become the default runtime refresh strategy for heavy domains.

---

## Heavy Domains

The following domains are heavy or potentially heavy:

```text
EPG
metadata
posters
fanart
preview images
preview streams
scraper-derived data
```

A change hint for a heavy domain must not automatically trigger a full-domain reload.

---

## Preferred Runtime Access Patterns

Preferred access patterns are:

- channel-scoped queries
- time-window queries
- object-specific queries
- pagination or result limits
- count-only or cheap validation queries where supported
- change-hint driven refresh decisions

For EPG, preferred query shapes include current or near-future windows, single channel schedule windows, limited events per channel, single event lookup and count-only validation.

---

## Adapter Boundary

Selective queries must remain behind backend-neutral adapter boundaries.

VDR-Suite must not hard-code RESTfulAPI-specific query strings into higher runtime layers.

Correct direction:

```text
Runtime layer
  -> backend-neutral query object
  -> IVdrAdapter
  -> backend-specific adapter mapping
  -> backend-local API or plugin access
```

Incorrect direction:

```text
Runtime layer
  -> RESTfulAPI URL construction
```

The first implementation of this rule is `VdrEventQuery` through the VDR adapter boundary.

---

## RESTfulAPI Direction

RESTfulAPI already provides selective event filtering capabilities such as time windows, channel paths, event IDs, limits and count-style options.

VDR-Suite should measure and use existing RESTfulAPI selective filters before proposing new RESTfulAPI endpoints.

New RESTfulAPI endpoints may still be justified later if the existing filters cannot achieve live-like performance for common frontend views.

---

## Relationship to Snapshots

Snapshots remain authoritative for snapshot-backed API reads.

Selective queries do not remove the snapshot architecture.

Selective queries define how snapshots and runtime views should be refreshed without unnecessary full-domain transfers.

Heavy-domain snapshots may be partial, windowed, lazily refreshed, refreshed only after explicit change hints or refreshed through background policy.

---

## Relationship to Change Hints and Live Transport

Change hints identify that something may have changed.

Change hints do not imply that the entire affected domain must be reloaded.

Live transport remains above the snapshot change feed.

Live transport must not become the owner of backend-specific query selection, heavy-domain refresh policy, snapshot generation, change detection or backend polling.

---

## Consequences

Benefits:

- avoids repeated full EPG transfers during runtime
- prepares VDR-Suite for large metadata and image domains
- keeps backend-specific optimizations inside adapters
- supports future multi-backend and permission-aware federation
- aligns backend workload with VDR frontend behavior

Costs:

- requires query objects and refresh policies per heavy domain
- requires real VDR validation of backend-specific query behavior
- requires care to keep snapshots consistent when using partial refreshes

---

## Validation Direction

Future validation should measure selective backend queries against real VDR systems.

For RESTfulAPI EPG this includes:

```text
/events.json?from=<unix-time>&timespan=<seconds>
/events.json?from=<unix-time>&timespan=<seconds>&chevents=<count>
/events/<channel-id>.json?from=<unix-time>&timespan=<seconds>
/events/<channel-id>/<event-id>.json
/events.json?from=<unix-time>&timespan=<seconds>&only_count=true
```

Validation must compare response size, HTTP time, returned event count, mapper behavior, snapshot build impact and usefulness for common frontend views.

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
