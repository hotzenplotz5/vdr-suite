# Person API and Persistent Person Search

## Status

This document describes the current person model, legacy/general person endpoints and the persistent Recording/EPG person paths used by current `main`.

Phase 61 and PR #111 changed the previous assessment: persisted EPG people are now implemented for EPG metadata and backend-scoped Global Search. They must no longer be listed as wholly missing.

## Implemented person foundations

- `Person`, `PersonRole` and `PersonCollection` domain values;
- deterministic person resolution/evidence model;
- standalone person query matcher/service/controller foundations;
- persistent native Recording-person relations and search;
- persistent EPG person relations derived from stored public metadata;
- backend-scoped Global Search over Recording and EPG people;
- provider/source/role/name/character fields used by current read models.

Supported roles:

```text
unknown, actor, director, writer, producer,
moderator, guest, composer, other
```

## Current routes

### Standalone/general person query foundation

```text
GET /api/persons
GET /api/vdr/persons
```

These endpoints expose the general person query contract. They remain a foundation rather than a complete provider-wide person catalogue.

### Recording-person search

```text
GET /api/recordings/persons/search
GET /api/vdr/recordings/persons/search
```

These routes query the persistent backend-scoped native Recording-person index and join matches to the persistent Recording cache.

### Backend-scoped Global Search

```text
GET /api/search?backend=<id>&query=<text>&...
GET /api/vdr/search?backend=<id>&query=<text>&...
```

Global Search searches:

- persisted Recording people in `vdr_recording_native_person`;
- persisted EPG people in `epg_scraper_metadata_people`;
- corresponding persisted Recording/EPG titles and subtitles.

It returns person summaries inside grouped Recording/EPG results and navigates to the existing Recordings 2 or EPG detail owner.

## EPG person persistence

EPG people are not fetched live from a provider during search. The persistent EPG metadata resolver replaces normalized person rows when it stores public metadata. Schema preparation backfills rows from existing persisted public metadata where required, and authoritative event retirement removes dependent person rows.

Normal Global Search GET requests:

- use a dedicated query-only SQLite connection in production configuration;
- do not parse provider JSON for every event;
- do not call TVScraper, SuiteBridge, RESTfulAPI, TMDB or IMDb;
- do not update person or metadata state.

## Person object fields

Current person/evidence models use fields such as:

- source;
- role;
- original name;
- normalized name;
- character name;
- confidence where applicable;
- provider reference where applicable;
- optional artwork/provider image reference in persisted metadata contracts.

The original name preserves provider spelling. Normalized values support deterministic matching. Provider references remain evidence and do not automatically establish a universal cross-provider person identity.

## Recording metadata payload boundary

Current main uses one consistent bounded RMETA contract:

```text
maximum people: 128
maximum payload: 65,535 bytes
```

The regression model preserves all 52 modelled `Pulp Fiction` people, including John Travolta beyond the former twelve-person cutoff.

This proves completeness for the modelled 52-person payload. It does not promise that every provider payload can never exceed 128 people.

Draft PR #101 raises only plugin-side limits to 256 people / 256 KiB. It is not compatible with the current SVDRP transport/backend parser contract and must not be merged piecemeal.

## General person-query parameters

The standalone person query foundation supports fields including:

| Parameter | Meaning |
| --- | --- |
| `name` | case-insensitive partial original-name match |
| `normalizedName` | exact normalized-name match |
| `characterName` | case-insensitive partial character match |
| `role` | supported role filter |
| `source` | source/provenance filter |
| `providerReference` | exact provider-reference filter |
| `limit`, `offset` | bounded paging values |

Negative limit/offset and unknown role/source values are rejected.

## Current limitations

The following are not claimed as complete:

- one universal cross-provider person identity graph;
- a complete standalone EPG-person catalogue endpoint independent of Global Search/details;
- every TVScraper cast/crew field exposed through a general person API;
- direct scraper2vdr/TMDB/IMDb provider integrations;
- user corrections/merges with actor identity and accountability;
- universal completeness beyond the current 128-person bounded transport.

## Related documents

- [Backend-Scoped Global Search](../architecture/global-search.md)
- [Recording Person Cast Completeness Fix](recording-person-cast-completeness-fix.md)
- [Phase 61 Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Current State](../CURRENT.md)