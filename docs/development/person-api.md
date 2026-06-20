# Person API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This document describes the current person metadata API foundation.

The current implementation provides a domain model, resolver, JSON contract and REST-facing controller boundary for person metadata such as actors, directors, writers, producers, moderators, guests and composers.

The API is not routed through ApiRouter yet.

---

## Current Scope

Implemented:

- Person domain object
- PersonRole
- PersonCollection
- PersonResolver
- PersonResolutionResult
- PersonResolutionJsonSerializer
- PersonController

Not implemented yet:

- ApiRouter endpoint
- request parser
- query/search model
- database persistence
- provider integration
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

---

## JSON Contract

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

## REST Boundary

The current controller boundary returns an ApiResponse with:

- statusCode 200
- contentType application/json
- body containing the serialized person resolution

The current boundary is intentionally isolated and not yet connected to ApiRouter.

---

## Future Direction

Future phases may add:

- routed REST endpoint
- person query model
- actor search
- director search
- writer search
- cast and crew filters
- provider mapping
- TVScraper integration
- TMDB integration
- real VDR metadata validation

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
