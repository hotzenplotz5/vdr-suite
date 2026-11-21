# Manual Recording Cast Ingestion and Search Integration

## Status

This is the accepted, limited post-Phase-62 feature block implemented by PR #136 on top of merged PR #135. Phase 63 remains planned and has not started.

Architecture decision: [ADR-0052](../adr/ADR-0052-manual-recording-cast-ingestion-search.md).

The final source head must retain complete green CI. The real yaVDR installation and primary presentation acceptance have been completed, and the user explicitly approved merge on 2026-08-05.

## Scope

When an administrator selects a TMDB movie for a recording, the backend requests that movie's credits, normalizes its cast and persists the selected movie and cast atomically in Suite-owned tables.

The same bounded mutation may materialize a limited set of cast profile images into Suite-owned local storage. The feature adds no browser-to-provider path, no provider access during reads, no TVScraper write, no TVScraper fork and no manual-only search endpoint.

## Acquisition contract

```text
bounded movie candidate search
  -> user selects one exact movie
  -> backend resolves backend-scoped TMDB credential
  -> GET /movie/{id}/credits
  -> bounded parse, maximum 128 cast entries
  -> bounded optional portrait materialization for leading cast
  -> one atomic manual-assignment transaction
```

Credits are not requested for candidate lists, folder navigation, detail GETs, global search or person search.

A successful provider response with `cast: []` is stored as a complete empty cast. Transport, timeout, authentication, rate-limit, HTTP, malformed-JSON and response-size failures abort the complete assignment and return a visible error.

Portrait download is best-effort presentation enrichment. Failure to materialize an individual portrait does not invalidate an otherwise valid movie-and-cast assignment. Browser responses expose only protected Suite-owned image URLs, never TMDB image URLs or local paths.

## Suite-owned model

The implementation reuses the metadata platform:

- `suite_metadata_entities` for canonical person entities;
- `suite_metadata_entity_external_ids` for `tmdb/person/<id>` identity;
- `suite_metadata_evidence` for immutable selected-provider evidence;
- `suite_metadata_assignments` for selected, relationship-locked revisions;
- `suite_metadata_person_values` for normalized person display/search values;
- `suite_metadata_recording_person_relations` for assignment-scoped role, character and order;
- `suite_metadata_manual_assignment_values.cast_complete` to distinguish valid empty cast from legacy/non-cast assignments;
- `suite_metadata_person_profiles` for provider-qualified Suite-owned portrait materialization metadata.

The provider-qualified person identity is the deduplication key. The same TMDB person may be referenced by several recordings without creating a second person entity or a second canonical portrait identity.

## Active and historical semantics

Only relations whose assignment is simultaneously:

- `selected`;
- `manual_assignment = 1`;
- `relationship_locked = 1`

belong to the active read model.

Reassignment creates a new revision and marks the old assignment superseded. Withdrawal marks the active assignment withdrawn. Neither operation deletes evidence, entities or historical relations.

While a manual assignment is active, its title, original title and cast override automatic presentation/search data for that recording. After withdrawal, automatic TVScraper/native data becomes effective again.

## Search integration

### Global search

The existing `GlobalSearchRepository` reads active manual title, original title and people through backend-scoped CTEs. The returned recording remains the existing global-search recording result; no frontend-only filter or manual result type is added.

A dedicated core repository resolves available local portraits for person summaries in one backend-scoped SQLite read. REST and frontend receive only a revision-versioned Suite image URL. There is no SQLite access in the REST controller and no provider request during search.

### Dedicated person search

`VdrRecordingNativeMetadataRepository::searchPeople` builds an effective-person relation from active manual cast plus automatic fallback. `VdrRecordingNativePersonSearchService` maps manual people to the existing domain person contract with source `tmdb`, actor role, character and provider reference.

### Recording detail

The existing recording metadata detail route returns manual actors in the established public shape. When a local portrait exists, the image object contains only the protected Suite URL; otherwise it reports unavailable.

```json
{
  "role": "actor",
  "name": "Example Actor",
  "characterName": "Example Character",
  "image": {
    "available": true,
    "url": "/api/recordings/metadata/image?...&kind=person&index=0&assignmentRevision=2"
  }
}
```

Internal entity IDs, provider URLs, local artwork paths and actor/accountability references are not part of this detail payload.

## Poster and portrait presentation contract

- the open detail view refreshes the recording read model after a successful assignment instead of re-rendering stale in-memory data;
- preferred poster URLs are versioned by assignment revision so a cached old poster or cached 404 cannot survive reassignment;
- the visible detail hero poster is eager/high-priority while folder grids and galleries remain lazy;
- global-search recording results retain their established root artwork shape and expose the nested presentation shape used by Recordings 2 tiles;
- local metadata images use revision-safe browser caching;
- person cards and recording cast rows use real portraits when available and retain a stable fallback when unavailable or when image decoding fails;
- no provider URL, local path or credential is exposed to the browser.

## Performance contract

- one backend-scoped assignment-and-cast batch read for folder serialization;
- one count and one page query for dedicated person search;
- one count and one page query for global recording search;
- one backend-scoped local portrait lookup for global person summaries;
- no SQL query per recording;
- no SQL query per person;
- no provider request during read paths;
- no schema DDL during repeated search or navigation;
- deterministic ordering by relevance, cast order, folded name and stable identity;
- existing repeated-folder-navigation performance from PR #135 must remain intact;
- local image caching and asynchronous decoding avoid repeated materialization or blocking reads.

Regression tests use SQLite statement tracing to verify the constant query contracts and absence of repeated schema DDL.

## Security contract

The protected mutation continues to use `metadata.recording.assign`.

- the backend route is authoritative;
- Admin with exact backend scope is allowed;
- Read-only, wrong-backend and invalid-CSRF requests are denied before provider access;
- authorization and operation outcomes remain accountable;
- the existing managed backend credential resolver is reused;
- credentials are never returned, logged by the feature or copied into accountability data;
- search, detail and image reads remain provider-free;
- browser payloads contain no provider URL, local artwork path, actor reference or secret-bearing process environment.

## Automated validation matrix

### Provider and parser

- name, TMDB person ID, character, cast order and optional `profile_path`;
- complete empty cast;
- malformed and oversized JSON;
- transport/timeout, HTTP 5xx and rate limit;
- bounded retry;
- no token text in errors;
- unsupported credits fail closed.

### Persistence

- atomic movie and cast storage;
- provider-qualified person deduplication;
- one person referenced by several recordings;
- provider-qualified portrait reuse;
- restart-compatible readback;
- reassignment suppresses old active relations;
- withdrawal removes manual relations from the active read model;
- evidence and historical relations remain;
- revision conflicts remain protected;
- one assignment-and-cast batch read, no repeated DDL.

### Search and detail

- manual title and original title find the recording;
- manual actor finds the person and connected recording;
- backend isolation;
- active manual people suppress automatic people for that recording;
- withdrawal restores automatic people;
- deterministic ordering and pagination;
- detail displays name, role, character and optional Suite portrait URL without internal IDs or paths;
- global person summaries expose only revision-versioned Suite portrait URLs;
- recording search cards expose local covers through the shared tile contract.

### REST and security

- credits are not loaded by candidate search or GET;
- provider failure creates no assignment;
- valid empty cast succeeds visibly;
- Admin allowed;
- Read-only, wrong-backend and invalid-CSRF denied;
- protected-operation accountability remains correct;
- metadata image errors are non-cacheable while revision-versioned successful images are cacheable.

## Real yaVDR acceptance checklist

Use the exact final PR head and matching installed daemon/web assets.

1. Open a recording with automatic metadata and repeat folder/subfolder/back navigation several times; confirm PR #135 navigation performance remains fast.
2. Search movie candidates and confirm no unusual delay from credit loading before selection.
3. Select a known movie with several recognizable actors.
4. Confirm assignment succeeds and the recording detail shows actor names and character names.
5. Confirm the newly selected poster appears immediately without a page reload.
6. Reload the page and confirm title, cast, character data and poster remain.
7. Search globally for the selected manual title and original title; confirm the recording appears with its local cover.
8. Search for one imported actor in the existing person search; confirm the person portrait and connected recording covers appear.
9. Search globally for that actor; confirm the connected recording appears according to the existing global-search contract.
10. Restart `vdr-suite-daemon`; repeat the title and actor searches without a new assignment and confirm no provider dependency.
11. Reassign the recording to a different movie; confirm actors from the former movie no longer return the recording as active.
12. Withdraw the manual assignment; confirm automatic TVScraper/native title and people become effective again and manual actors no longer return the recording.
13. Restart again and confirm the withdrawn manual relationship does not reappear.
14. Repeat assignment as Read-only and against a backend outside the actor scope; confirm denial.
15. Confirm browser responses and accountability records contain no token, provider URL, local artwork path or actor reference.
16. Record the exact head, CI run, installed build identity and redacted recording identity.

## Real yaVDR evidence

The exact source head `6122ddde2ef3581cf233a1f94869e447c403c241` was built and installed on the real yaVDR host with branch/SHA guards. Daemon and changed web assets matched the checkout, the service restarted successfully and the installation reported:

```text
INSTALLATION_PR_136_PORTRAITS=PASS
```

Real UI testing confirmed on first attempt:

- movie assignment, cast names and character names work;
- actor search finds the expected recordings;
- the newly assigned movie poster appears immediately without a browser reload;
- recording covers appear in actor/global search results;
- locally materialized actor portraits appear correctly;
- image loading and page rendering are responsive.

The user explicitly accepted this result and approved merge on 2026-08-05. Reassignment, withdrawal, authorization and provider-failure behavior remain covered by the green automated regression matrix and the established protected-mutation contracts.
