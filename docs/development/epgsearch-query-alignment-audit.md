# EPGSearch Query Alignment Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
- [EPGSearch Result Model Audit](epgsearch-result-model-audit.md)
- [Current Status](current-status.md)

---

## Purpose

Phase 48.8 audits the relationship between the existing `EpgSearchRequest` and the newer `EpgSearchQuery`.

GitHub source inspection showed that both controller and route already exist:

- `EpgController::search(...)`
- `GET /api/epg/search` in `ApiRouter`

The remaining architectural issue is therefore query model alignment.

## Existing Route

`GET /api/epg/search` is already routed through `ApiRouter`.

Supported HTTP parameters include:

- query
- backend
- channelId
- from
- timespan
- limit
- offset
- sort
- order

## Existing Controller

``EpgController::search(...)` already:

- validates `timespan`
- validates `limit`
- validates `offset`
- validates sort field
- validates sort order
- reads events from `IEpgQueryService`
- builds an `EpgSearchRequest`
- invokes `EpgSearchService`
- serializes `EpgSearchResult`

## Model Boundary Finding

``EpgSearchRequest` and `EpgSearchQuery` should not be treated as duplicates.

They represent different layers.

### EpgSearchRequest

Layer:

- API/controller request

Responsibilities:

- HTTP-facing request shape
- backend id
- query text
- channel id
- time-window request
- limit
- offset
- sort field
- sort order
- legacy field selection

### EpgSearchQuery

Layer:

- backend-neutral domain query

Responsibilities:

- search text
- search mode
- fuzzy tolerance
- title/subtitle/description selection
- match-case
- channel scope
- time scope
- duration scope
- day-of-week scope
- extended EPG info
- content descriptors
- favorites-only scope

## Decision

Do not remove `EpgSearchRequest`.

Instead, introduce an explicit mapping step in a later phase:

- `EpgSearchRequest`
- maps to
- `EpgSearchQuery`

This keeps HTTP paging, sorting and window concerns separate from backend-neutral search semantics.

## Recommended Next Phase

Phase 48.9 - EPGSearch request-to-query mapper

Expected result:

- `EpgSearchRequestMapper`
- focused unit test
- maps HTTP/controller request fields into `EpgSearchQuery`
- keeps sort and paging in `EpgSearchRequest`

No new route is needed.

## Back

- [Back to Development Index](index.md)
- [Back to EPGSearch Capability Matrix](epgsearch-capability-matrix.md)

## Phase 48.9 Mapper Follow-up

The planned explicit mapper was implemented after this audit.

``EpgSearchRequestMapper` now separates API/controller request concerns from backend-neutral domain query semantics.
