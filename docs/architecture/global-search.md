# Backend-Scoped Global Search

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)

---

## Status

Implemented for the selected VDR backend on top of persistent recording, EPG, metadata, assignment and person read models. The first production slice deliberately does not aggregate multiple backends.

Draft PR #136 extends the existing recording group with active manual movie titles, original titles and cast while preserving the same API and frontend result contracts. See [ADR-0052](../adr/ADR-0052-manual-recording-cast-ingestion-search.md).

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

The browser never calls TVScraper, TMDB, IMDb, RESTfulAPI, SVDRP or SuiteBridge. Search GET requests perform no provider resolution, credit acquisition or metadata refresh.

## Public API

```text
GET /api/search?backend=<id>&query=<text>&limit=<n>&offset=<n>&from=<epoch>&until=<epoch>
```

`/api/vdr/search` is an equivalent compatibility alias. The response keeps recording, EPG and optional person-summary groups separate. Both recording and EPG groups expose their own total and `hasMore` state.

The controller rules are:

- minimum query length: 2 folded characters;
- default limit: 20 per result group;
- maximum limit: 50 per result group;
- default EPG window: six hours before now through fourteen days after now;
- maximum explicit EPG window: 31 days;
- negative limit or offset: rejected;
- unknown or disabled backend: rejected before repository access.

Empty and too-short requests return a structured successful response so the mobile frontend can show an explanatory state without treating normal input as an error.

## Persistent data ownership

### Automatic recording metadata

Automatic recording title results use `vdr_recording_cache` together with `vdr_recording_native_metadata`. Automatic recording person results use `vdr_recording_native_person`.

### Active manual recording metadata

An active manual recording assignment is selected from:

- `suite_metadata_assignments`;
- `suite_metadata_manual_assignment_values`;
- `suite_metadata_recording_person_relations`;
- `suite_metadata_person_values`.

The assignment must be selected, manual and relationship locked. Its presentation title, original title and people become the effective recording read model. Automatic people for the same recording are suppressed while that assignment is active. Superseded and withdrawn assignments remain stored but do not produce search results.

Withdrawal requires no reindexing or provider access: the set-based query stops selecting the manual assignment and the automatic recording metadata and people become effective again.

This is not a second manual search store. The manual tables are the same Suite-owned metadata entities, evidence, assignment and relation model used by the recording detail workflow.

### EPG metadata

EPG title results use `epg_events` and existing public TVScraper metadata. EPG person results use `epg_scraper_metadata_people`, a normalized relation in the same VDR-Suite SQLite database. It is not a second metadata store:

- the existing persistent EPG metadata resolver replaces its rows whenever it stores public metadata;
- startup schema preparation backfills rows from existing `epg_scraper_metadata_cache.public_json` data;
- authoritative EPG event retirement removes the corresponding person rows;
- search reads never update this relation.

## Effective recording search semantics

For a recording without an active manual assignment, title and person matching retains the automatic behavior.

For a recording with an active manual assignment:

- the manual presentation title is returned;
- the manual title and original title are searchable;
- the existing recording-cache title remains searchable for compatibility;
- active manual actors are searchable;
- automatic TVScraper actors for that recording are not active search evidence;
- the existing global-search recording result and person-summary shapes are reused.

A person-name match can therefore return both the person summary and the connected recording without a separate manual endpoint or frontend filter.

## Read isolation and performance

Production configuration opens a dedicated SQLite connection against the existing database and enables `PRAGMA query_only=ON`. Schema creation and one-time compatibility preparation remain on the writer connection during runtime configuration.

### Recording group

Recording title and person candidates are built through backend-scoped CTEs. Active manual assignments and matching manual people are joined once to the recording cache. Automatic people are joined through persistent recording identity. The repository executes one count and one bounded page query.

The query does not:

- issue one SQL statement per recording;
- issue one SQL statement per person;
- parse provider JSON per result;
- create or alter schema during search;
- contact TMDB or TVScraper.

Deterministic ordering uses exact title, prefix title, other title, person match, recording start time, folded title and backend-native identity. Manual cast ties use folded name, cast order and provider-qualified external identity.

### EPG group

EPG search is bounded by backend and an explicit time window before text or person matching. Title and person candidates are produced by separate set-based queries and merged by event identity. Person lookup therefore does not run correlated subqueries for every EPG row, and normal search GETs do not parse persisted provider JSON.

Both result groups have hard pagination limits and deterministic tie breakers. Existing backend/time indexes remain in use. Dedicated indexes cover normalized EPG person lookup, manual person identity/name lookup and assignment-scoped recording-person relations. Regression fixtures exercise 174,164 EPG events and SQLite statement tracing for the manual recording paths.

Search folding is deterministic and local. It is case-insensitive for ASCII, normalizes German umlauts (`ä/ö/ü`) to `ae/oe/ue`, and maps `ß` to `ss`. Descriptions are intentionally excluded from the first title-search slice to avoid an unindexed broad full-text path.

## Dedicated person search relationship

Global search and dedicated recording-person search share the same active-manual semantics but retain their established response contracts.

The dedicated person repository builds an effective-person CTE from:

- active manual actors with source `tmdb` and provider reference `tmdb:person:<id>`;
- automatic TVScraper people only where no active manual assignment overrides the recording.

It executes one count and one page query. Name, normalized name, character, role, source and provider-reference filters remain backend scoped.

## Frontend ownership

The search launcher sits directly after the feature card owned by the VDR remote runtime. The remote module and its hotspot behavior are unchanged.

The dialog implements a 280 ms debounce and a request sequence plus `AbortController`. A late response can therefore never overwrite a newer query. A 12-second client timeout replaces a permanently spinning state with a visible mobile error. Query, result payload and scroll position remain in memory while an existing detail owner is open.

Recording cards and details are owned by Recordings 2. EPG details are owned by `VdrSuiteEpgDetailOwner`. The search feature does not create a second detail view and does not wrap or modify the EPG timeline.

Manual people use the existing recording metadata person presentation. Name, actor role and character are displayed through the established public contract; internal metadata entity IDs, provider URLs and local paths are not added to the detail UI.

## Cast completeness decision

The production recording contract is bounded to 128 people. Manual selected-movie credits use the same 128-person maximum so provider acquisition, persistence, search and presentation share one defensible limit.

Credits are loaded only for the selected movie. A successful empty list is recorded as complete. Technical provider failures abort the assignment instead of producing a permanently incomplete cast.

A future increase must change provider parser, persistence, search, public transport and tests as one versioned contract, supported by evidence that real provider casts exceed 128 people and that the larger bound is operationally justified.

## Future multi-backend search

The response and repository items already carry `backendId`. A later aggregator may fan out to explicitly authorized backend-scoped searches and merge their bounded pages. It must not silently remove backend isolation or share provider state across backends.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
