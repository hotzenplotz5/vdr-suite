# Person API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This document describes the current person metadata and person query API.

The implementation provides:

- person domain model
- person role model
- person collection model
- deterministic person resolver
- person resolution JSON contract
- person query model
- person query matcher
- person search service
- person query result JSON contract
- REST-facing person controller
- ApiRouter wiring for routed person search endpoints
- recording-person search result model
- recording-person search service
- recording-person search JSON contract
- REST-facing recording-person search controller
- snapshot-backed recording-person search routing

The routed person query endpoints expose the query contract and return a valid paged person result shape.

The routed recording-person search endpoints search recording-attached person metadata from the current VDR snapshot.

EPG, full TVScraper character export, scraper2vdr, TMDB and IMDb person metadata sources are not fully connected yet.

---

## Current Scope

Implemented:

- Person
- PersonRole
- PersonCollection
- PersonResolver
- PersonResolutionResult
- PersonResolutionJsonSerializer
- PersonQuery
- PersonQueryMatcher
- PersonSearchService
- PersonQueryResult
- PersonQueryResultJsonSerializer
- PersonController
- ApiRouter wiring
- DaemonRuntime wiring
- RecordingPersonSearchResult
- RecordingPersonSearchService
- RecordingPersonSearchResultJsonSerializer
- RecordingPersonSearchController
- snapshot-backed recording-person search routing

Implemented routed endpoints:

- GET /api/persons
- GET /api/vdr/persons
- GET /api/recordings/persons/search
- GET /api/vdr/recordings/persons/search

Not implemented yet:

- full TVScraper character and crew metadata export
- real EPG person metadata extraction
- TVScraper integration
- scraper2vdr integration
- TMDB integration
- IMDb integration
- persistent person index
- EPG person search
- real VDR metadata validation

---

## Person Object

A person object contains:

- source
- role
- originalName
- normalizedName
- characterName
- confidence
- providerReference

The original name preserves the provider supplied spelling.

The normalized name is intended for deterministic matching and future search.

The character name is optional and mainly useful for cast data.

The provider reference is optional and can later hold stable provider identifiers such as TMDB, TVDB or IMDb references.

---

## Supported Roles

The current domain supports:

- unknown
- actor
- director
- writer
- producer
- moderator
- guest
- composer
- other

The query API accepts the same lowercase role values.

---

## Supported Sources

The current query API accepts:

- epg
- dvb
- tvscraper
- scraper2vdr
- tmdb
- tvdb
- imdb
- user
- folder
- derived

The source field is designed to preserve where a person fact came from.

---

## Person Resolution JSON Contract

The serialized person resolution contains:

- resolved
- primaryPerson
- evidence

The primary person is the currently selected person representation.

The evidence list preserves all known person facts.

Example response shape:

    {
      "resolved": true,
      "primaryPerson": {
        "source": "tmdb",
        "role": "actor",
        "originalName": "Tom Hanks",
        "normalizedName": "tom-hanks",
        "characterName": "Forrest Gump",
        "confidence": 90,
        "providerReference": "tmdb:31"
      },
      "evidence": [
        {
          "source": "epg",
          "role": "actor",
          "originalName": "Tom Hanks",
          "normalizedName": "tom-hanks",
          "characterName": "",
          "confidence": 0,
          "providerReference": ""
        }
      ]
    }

---

## Resolution Rules

The current resolver uses deterministic rules:

- manual user entries override provider entries
- explicit confidence is preferred
- higher confidence wins
- provider references are preferred over anonymous entries
- source priority is used as final tie-breaker

The resolver does not yet perform external identity lookup.

---

## Person Query Endpoints

The routed query endpoints are:

    GET /api/persons
    GET /api/vdr/persons

Both endpoints currently use the same controller and JSON contract.

The /api/vdr/persons alias exists to keep person search close to the VDR-facing API namespace.

---

## Query Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| name | string | empty | Case-insensitive partial match against originalName. |
| normalizedName | string | empty | Exact match against normalizedName. |
| role | string | empty | Optional role filter. |
| source | string | empty | Optional source filter. |
| providerReference | string | empty | Exact provider reference filter. |
| limit | integer | 0 | Maximum number of returned persons. Zero means no explicit limit. |
| offset | integer | 0 | Number of matching persons to skip. |

An empty query matches all supplied person facts.

For the standalone person endpoints, the router still supplies an empty person collection.

For recording-person search endpoints, the router supplies snapshot-backed recordings from VdrSnapshotReadService.

---

## Validation

Invalid requests return HTTP 400 with a JSON error response.

Validation rules:

- limit must not be negative
- offset must not be negative
- role must be empty or one of the supported role values
- source must be empty or one of the supported source values

Example validation responses:

    {"error":"invalid person role"}

    {"error":"invalid person source"}

    {"error":"limit must not be negative"}

    {"error":"offset must not be negative"}

---

## Successful Person Query Response

Successful requests return HTTP 200 and application/json.

Response shape:

    {
      "totalCount": 0,
      "returnedCount": 0,
      "limit": 10,
      "offset": 0,
      "persons": []
    }

When person data is available in later phases, the persons array will contain entries shaped like:

    {
      "source": "tmdb",
      "role": "actor",
      "originalName": "Tom Hanks",
      "normalizedName": "tom-hanks",
      "characterName": "Forrest Gump",
      "confidence": 95,
      "providerReference": "tmdb:31"
    }

---

## Matching

The current query matcher supports:

- name partial match against originalName, case-insensitive
- normalizedName exact match
- role exact match
- source exact match
- providerReference exact match

All configured filters must match.

An empty query matches every person in the supplied collection.

---

## Pagination

The person query result contains:

- totalCount
- returnedCount
- limit
- offset

totalCount is the number of matches before pagination.

returnedCount is the number of returned persons after offset and limit are applied.

A limit of zero means no explicit limit.

---

## Current Source of Truth

The standalone person API path is:

    ApiRouter
    -> PersonController
    -> PersonSearchService
    -> PersonQueryMatcher
    -> PersonQueryResultJsonSerializer

The router still supplies an empty PersonCollection for the standalone person API.

The recording-person search API path is:

    ApiRouter
    -> VdrSnapshotReadService
    -> RecordingPersonSearchController
    -> RecordingPersonSearchService
    -> PersonQueryMatcher
    -> RecordingPersonSearchResultJsonSerializer

For recording-person search, the router supplies snapshot-backed recordings from the current VDR snapshot.

If the backend parameter is empty, all default snapshot recordings are used.

If the backend parameter is set, recordings are read from the matching backend snapshot.

---

## Recording Person Search Endpoints

Recording-person search is routed through:

    GET /api/recordings/persons/search
    GET /api/vdr/recordings/persons/search

These endpoints search person metadata attached to recordings in the current VDR snapshot.

Supported parameters:

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| name | string | empty | Case-insensitive partial match against originalName. |
| normalizedName | string | empty | Exact match against normalizedName. |
| role | string | empty | Optional role filter. |
| source | string | empty | Optional source filter. |
| providerReference | string | empty | Exact provider reference filter. |
| backend | string | empty | Optional backend filter. Empty uses default snapshot recordings. |
| limit | integer | 0 | Maximum number of returned matches. Zero means no explicit limit. |
| offset | integer | 0 | Number of matching entries to skip. |

Successful response shape:

    {
      "totalCount": 1,
      "returnedCount": 1,
      "limit": 10,
      "offset": 0,
      "matches": [
        {
          "recording": {
            "id": "router-recording-1",
            "backendId": "default",
            "title": "Router Recording",
            "path": "/srv/vdr/video/Router_Recording/2026-06-04.20.00.1-0.rec"
          },
          "person": {
            "source": "tvscraper",
            "role": "actor",
            "originalName": "Router Actor",
            "normalizedName": "router-actor",
            "characterName": "Router Character",
            "confidence": 95,
            "providerReference": "tvscraper:router-actor"
          }
        }
      ]
    }

The recording-person search does not inspect recording titles, paths or descriptions as fallback person sources.

It only searches structured Person entries attached to VdrRecording.persons.

---

## Out of Scope

The following are intentionally out of scope for the current person query API:

- real VDR person metadata extraction
- additional recording person metadata extraction
- EPG person metadata extraction
- persistent person index
- external provider lookup
- TMDB identity resolution
- IMDb identity resolution
- TVScraper integration
- scraper2vdr integration
- SearchTimer integration

---

## Future Direction

Future phases may add:

- real VDR person metadata validation
- recording person metadata extraction
- EPG person metadata extraction
- TVScraper person import
- scraper2vdr person import
- TMDB and IMDb provider references
- additional actor and director search validation over real recordings
- cast and crew filters
- persistent person index
- multi-backend person search
- person search result entries that reference recordings and EPG events

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
