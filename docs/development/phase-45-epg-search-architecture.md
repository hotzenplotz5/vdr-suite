# Phase 45.0 - EPG Search Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

Phase 45.0 defines the architecture for EPG search before implementation starts.

This phase is intentionally an architecture phase.

It does not introduce C++ implementation, REST route changes, SearchTimer execution or persistent EPG mirroring.

---

## Source of Truth Rule

VDR remains the source of truth for:

- channels
- EPG events
- timers
- running events
- recording state

VDR-Suite complements VDR.

VDR-Suite must not create a competing persistent full EPG mirror.

---

## Existing EPG Foundation

The current EPG read path is:

EPG REST API
-> EpgController
-> IEpgQueryService
-> EpgQueryService
-> VdrService
-> IVdrAdapter
-> backend-specific adapter
-> VDR / RESTfulAPI

Existing selective read scopes are:

- NowNext
- TimeWindow
- ChannelWindow

The current RESTfulAPI adapter maps selective event queries to RESTfulAPI-compatible event endpoints and query parameters.

---

## Phase 45 Architecture Goal

Phase 45 introduces the architecture for searchable, filterable and later multi-backend-capable EPG access.

The architecture must support:

- text search
- EPG field selection
- genre filters
- genre sorting
- grouping
- backend-aware result identity
- future SearchTimer construction
- RESTfulAPI compatibility
- non-RESTfulAPI backend adapters later

---

## Genre Architecture Requirement

Genres must be modeled as a multi-source classification layer.

The design must not assume that DVB EPG content descriptors are the only genre source.

Supported future genre sources are:

1. DVB / EPG genre
2. user-defined genre
3. folder- or path-based genre
4. tvscraper genre
5. IMDb genre
6. TVDb genre
7. TMDb genre

A future EPG genre object should preserve at least:

- source
- original value
- normalized value
- optional confidence
- optional provider-specific reference

The first implementation steps may expose only available VDR / RESTfulAPI data.

The architecture must still keep the multi-source shape open from the beginning.

---

## Search Request Shape

A future EPG search request should be backend-neutral.

Candidate fields:

- query text
- search fields:
  - title
  - subtitle
  - description
- channelId
- backendId
- from
- timespan
- limit
- genre filters
- sort mode
- grouping mode

The request must remain independent from RESTfulAPI URL syntax.

RESTfulAPI-specific query construction belongs in the adapter layer.

---

## Search Result Shape

A future EPG search result should carry enough identity for later actions.

Candidate fields:

- backendId
- eventId
- channelId
- title
- subtitle
- description
- startTime
- endTime
- durationSeconds
- genres
- match metadata
- timer state, if available later

Search results must not require persistent full EPG storage.

---

## Live Plugin Comparison

The VDR Live plugin is the functional reference level for EPG usability.

Relevant Live capabilities include:

- fast VDR-internal access to EPG data
- EPG event presentation with title, short text, description, channel and time data
- SearchTimer concepts
- title, subtitle and description matching
- time, channel, duration and weekday constraints
- blacklist and repeat handling
- extended EPG information
- timer-related search results

VDR-Suite should target this usability level over time.

Unlike Live, VDR-Suite must remain backend-neutral, REST-capable and multi-backend-ready.

---

## RESTfulAPI Compatibility

RESTfulAPI compatibility remains mandatory.

Existing EPG endpoints must not be broken:

- GET /api/epg/now-next
- GET /api/epg/time-window
- GET /api/epg/channel-window

A future search endpoint should be additive.

Candidate future endpoint:

- GET /api/epg/search

Search implementation must prefer selective backend access.

If local filtering is needed, it must operate only on explicitly requested selective windows.

---

## Out of Scope for Phase 45.0

The following are out of scope for this architecture document:

- C++ implementation
- SearchTimer execution
- timer creation
- persistent full EPG mirror tables
- scraper integration
- frontend UI
- authentication and authorization
- destructive actions
- RESTfulAPI semantic changes

---

## Proposed Phase 45 Sequence

Recommended implementation sequence after this architecture step:

1. Phase 45.1 - EPG search request domain
2. Phase 45.2 - EPG genre source model
3. Phase 45.3 - EPG search result model
4. Phase 45.4 - EPG search matcher for selective event windows
5. Phase 45.5 - EPG search JSON contract
6. Phase 45.6 - EPG search REST controller boundary
7. Phase 45.7 - RESTfulAPI-backed search validation
8. Phase 45.8 - backend-aware EPG search extension
9. Phase 45.9 - SearchTimer architecture preparation

---

## Verification Strategy

Local verification for architecture-only work:

- make test-docs
- make test-phase
- make daemon

Full regression remains the responsibility of GitHub Actions for normal changes.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
