# EPG Search API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Phase 45 EPG Search Architecture](phase-45-epg-search-architecture.md)

---

## Purpose

This document describes the implemented EPG search REST contract.

The search API is additive.

It does not replace the existing selective EPG endpoints:

- GET /api/epg/now-next
- GET /api/epg/time-window
- GET /api/epg/channel-window

---

## Endpoint

GET /api/epg/search

The endpoint searches over a selective EPG time window.

VDR remains the source of truth.

VDR-Suite does not create a persistent full EPG mirror.

---

## Query Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| query | string | empty | Text query matched against title, subtitle and description. |
| backend | string | empty | Backend identity carried into the result metadata. |
| channelId | string | empty | Optional channel filter. |
| from | integer | -1 | Optional start timestamp passed to the selective EPG query. |
| timespan | integer | 7200 | Search window size in seconds. Must be greater than zero. |
| limit | integer | 0 | Maximum number of returned matches. Zero means no explicit limit. |
| offset | integer | 0 | Number of matching entries to skip. |
| sort | string | empty | Optional sort field. Allowed values: title, startTime, duration. |
| order | string | empty | Optional sort order. Allowed values: asc, desc. Empty means ascending. |

---

## Validation

Invalid requests return HTTP 400 with a JSON error response.

Validation rules:

- timespan must be greater than zero
- limit must not be negative
- offset must not be negative
- sort must be empty or one of:
  - title
  - startTime
  - duration
- order must be empty or one of:
  - asc
  - desc

Example validation response:

    {"error":"invalid sort field"}

---

## Successful Response

Successful requests return HTTP 200 and application/json.

Response shape:

    {
      "totalCount": 2,
      "returnedCount": 2,
      "limit": 10,
      "offset": 0,
      "results": [
        {
          "eventId": "event-1",
          "backendId": "default",
          "channelId": "S19.2E-1-1011-11110",
          "title": "Tatort",
          "subtitle": "Borowski",
          "description": "Krimi",
          "startTime": "1780000000",
          "endTime": "1780003600",
          "durationSeconds": 3600,
          "matchedFields": [
            "title"
          ]
        }
      ]
    }

---

## Matching

The first implementation matches the query text case-insensitively against:

- title
- subtitle
- description

The response includes matchedFields for result transparency.

Future phases may extend this with genre, actor, director, rating, metadata-provider and SearchTimer-related matching.

---

## Sorting

Implemented sort fields:

- title
- startTime
- duration

Implemented order values:

- asc
- desc

Sorting is applied after matching and before pagination.

---

## Pagination

The search result contains:

- totalCount
- returnedCount
- limit
- offset

totalCount is the number of matches before pagination.

returnedCount is the number of returned results after offset and limit are applied.

---

## Source of Truth

The EPG search API searches events provided through the existing selective EPG query path.

The current path is:

    ApiRouter
    -> EpgController
    -> IEpgQueryService
    -> EpgQueryService
    -> VdrService
    -> IVdrAdapter
    -> VDR / RESTfulAPI

Search filtering is applied only to explicitly requested selective windows.

There is no full EPG mirror.

---

## Out of Scope

The following are intentionally out of scope for the current EPG search API:

- genre filtering
- genre sorting
- SearchTimer execution
- saved searches
- persistent EPG indexing
- metadata provider integration
- user profile policy
- content rating policy

---

## Future Direction

Future phases may add:

- multi-source genre matching
- tvscraper, TMDb, TVDb and IMDb enrichment
- user-defined genres
- folder/path-derived genres
- SearchTimer foundations
- multi-backend EPG search
- user profile and content-rating policy

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
