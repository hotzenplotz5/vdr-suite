# Backend-Scoped Global Search

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)

---

## Status

Implemented for the selected VDR backend on top of the persistent recording,
EPG, metadata and person read models. The first production slice deliberately
does not aggregate multiple backends.

## Request path

```text
Frontend search dialog
  -> VdrSuiteClientApi.fetchClientGlobalSearch()
  -> GET /api/search
  -> GlobalSearchApiRuntime
  -> GlobalSearchController
  -> GlobalSearchService
  -> GlobalSearchRepository
  -> existing VDR-Suite SQLite database
```

The browser never calls TVScraper, TMDB, IMDb, RESTfulAPI, SVDRP or
SuiteBridge. Search GET requests perform no provider resolution and do not
refresh metadata.

## Public API

```text
GET /api/search?backend=<id>&query=<text>&limit=<n>&offset=<n>&from=<epoch>&until=<epoch>
```

`/api/vdr/search` is an equivalent compatibility alias. The response keeps
recording, EPG and optional person-summary groups separate. Both recording and
EPG groups expose their own total and `hasMore` state.

The controller rules are:

- minimum query length: 2 folded characters;
- default limit: 20 per result group;
- maximum limit: 50 per result group;
- default EPG window: six hours before now through fourteen days after now;
- maximum explicit EPG window: 31 days;
- negative limit or offset: rejected;
- unknown or disabled backend: rejected before repository access.

Empty and too-short requests return a structured successful response so the
mobile frontend can show an explanatory state without treating normal input as
an error.

## Persistent data ownership

Recording title results use `vdr_recording_cache` together with
`vdr_recording_native_metadata`. Recording person results use the existing
`vdr_recording_native_person` relation.

EPG title results use `epg_events` and existing public TVScraper metadata.
EPG person results use `epg_scraper_metadata_people`, a normalized relation in
the same VDR-Suite SQLite database. It is not a second metadata store:

- the existing persistent EPG metadata resolver replaces its rows whenever it
  stores public metadata;
- startup schema preparation backfills rows from existing
  `epg_scraper_metadata_cache.public_json` data;
- authoritative EPG event retirement removes the corresponding person rows;
- search reads never update this relation.

## Read isolation and performance

Production configuration opens a dedicated SQLite connection against the
existing database and enables `PRAGMA query_only=ON`. Schema creation and the
one-time compatibility backfill remain on the writer connection during runtime
configuration.

EPG search is bounded by backend and an explicit time window before text or
person matching. Title and person candidates are produced by separate set-based
queries and merged by event identity. Person lookup therefore no longer runs
correlated subqueries for every EPG row, and normal search GETs do not parse the
persisted provider JSON. Recording search is bounded by the selected backend.
Both result groups have hard pagination limits and deterministic tie breakers.
Existing backend/time indexes remain in use; the only new indexes cover
normalized EPG person lookup and event cleanup. A regression fixture exercises
174,164 EPG events to guard the live-search latency path.

Search folding is deterministic and local. It is case-insensitive for ASCII,
normalizes German umlauts (`ä/ö/ü`) to `ae/oe/ue`, and maps `ß` to `ss`.
Descriptions are intentionally excluded from the first title-search slice to
avoid an unindexed broad full-text path.

## Frontend ownership

The search launcher sits directly after the feature card owned by the VDR
remote runtime. The remote module and its hotspot behavior are unchanged.

The dialog implements a 280 ms debounce and a request sequence plus
`AbortController`. A late response can therefore never overwrite a newer
query. A 12-second client timeout replaces a permanently spinning state with a
visible mobile error. Query, result payload and scroll position remain in memory
while an existing detail owner is open.

Recording cards and details are owned by Recordings 2. EPG details are owned by
`VdrSuiteEpgDetailOwner`. The search feature does not create a second detail
view and does not wrap or modify the EPG timeline.

## Cast completeness decision

The current main branch already carries the production-accepted recording
contract of 128 people and 65,535 payload bytes. Its regression payload keeps
all 52 modelled `Pulp Fiction` people, including John Travolta beyond the former
12-person cutoff. The open PR #101 only raises plugin-side limits to 256 people
and a 256 KiB payload; on current main that would no longer match the backend
parser and transport contracts. Therefore no part of that old conflicting
patch is copied into this implementation.

A future increase must change plugin, SVDRP transport, backend parser and tests
as one versioned contract, supported by evidence that real provider casts exceed
128 people and that the larger bound is operationally justified.

## Future multi-backend search

The response and repository items already carry `backendId`. A later aggregator
may fan out to explicitly authorized backend-scoped searches and merge their
bounded pages. It must not silently remove backend isolation or share provider
state across backends.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
