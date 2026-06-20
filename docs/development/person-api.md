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

The routed person query endpoints currently expose the query contract and return a valid paged person result shape.

Real VDR, recording, EPG, TVScraper, scraper2vdr, TMDB and IMDb person metadata sources are not connected yet.

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

Implemented routed endpoints:

- GET /api/persons
- GET /api/vdr/persons

Not implemented yet:

- real recording person metadata extraction
- real EPG person metadata extraction
- TVScraper integration
- scraper2vdr integration
- TMDB integration
- IMDb integration
- persistent person index
- full cast and crew search over recordings
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

At the current routing stage, no real metadata source is connected yet, so the router supplies an empty person collection.

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

The current routed API path is:

    ApiRouter
    -> PersonController
    -> PersonSearchService
    -> PersonQueryMatcher
    -> PersonQueryResultJsonSerializer

At this stage, the router supplies an empty PersonCollection.

This means the API contract, validation, routing and serialization are implemented, but real data sources are intentionally still out of scope.

---

## Recording Search Status

Recordings are not included in person search yet.

The current person search does not inspect:

- recording titles
- recording descriptions
- recording paths
- recording metadata
- EPG events behind recordings
- TVScraper metadata
- scraper2vdr metadata
- TMDB metadata
- IMDb metadata

Future phases must validate which person metadata is available from real VDR installations before connecting recordings to person search.

---

## Out of Scope

The following are intentionally out of scope for the current person query API:

- real VDR person metadata extraction
- recording person metadata extraction
- EPG person metadata extraction
- persistent person index
- external provider lookup
- TMDB identity resolution
- IMDb identity resolution
- TVScraper integration
- scraper2vdr integration
- actor-to-recording reverse lookup
- director-to-recording reverse lookup
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
- actor search over recordings
- director search over recordings
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
