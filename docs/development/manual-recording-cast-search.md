# Manual Recording Cast Ingestion and Search Integration

## Status

This is the active, limited post-Phase-62 feature block in Draft PR #136. PR #135 is the merged foundation. Phase 63 remains planned and has not started.

Architecture decision: [ADR-0052](../adr/ADR-0052-manual-recording-cast-ingestion-search.md).

The feature remains incomplete until the exact final source head has complete green CI and the real yaVDR checklist below has been executed and accepted.

## Scope

When an administrator selects a TMDB movie for a recording, the backend requests that movie's credits, normalizes its cast and persists the selected movie and cast atomically in Suite-owned tables.

The feature adds no new browser-to-provider path, no TVScraper write, no TVScraper fork and no manual-only search endpoint.

## Acquisition contract

```text
bounded movie candidate search
  -> user selects one exact movie
  -> backend resolves backend-scoped TMDB credential
  -> GET /movie/{id}/credits
  -> bounded parse, maximum 128 cast entries
  -> one atomic manual-assignment transaction
```

Credits are not requested for candidate lists, folder navigation, detail GETs, global search or person search.

A successful provider response with `cast: []` is stored as a complete empty cast. Transport, timeout, authentication, rate-limit, HTTP, malformed-JSON and response-size failures abort the complete assignment and return a visible error.

## Suite-owned model

The implementation reuses the metadata platform:

- `suite_metadata_entities` for canonical person entities;
- `suite_metadata_entity_external_ids` for `tmdb/person/<id>` identity;
- `suite_metadata_evidence` for immutable selected-provider evidence;
- `suite_metadata_assignments` for selected, relationship-locked revisions;
- `suite_metadata_person_values` for normalized person display/search values;
- `suite_metadata_recording_person_relations` for assignment-scoped role, character and order;
- `suite_metadata_manual_assignment_values.cast_complete` to distinguish valid empty cast from legacy/non-cast assignments.

The provider-qualified person identity is the deduplication key. The same TMDB person may be referenced by several recordings without creating a second person entity.

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

### Dedicated person search

`VdrRecordingNativeMetadataRepository::searchPeople` builds an effective-person relation from active manual cast plus automatic fallback. `VdrRecordingNativePersonSearchService` maps manual people to the existing domain person contract with source `tmdb`, actor role, character and provider reference.

### Recording detail

The existing recording metadata detail route returns manual actors in the established public shape:

```json
{
  "role": "actor",
  "name": "Example Actor",
  "characterName": "Example Character",
  "image": { "available": false }
}
```

Internal entity IDs, provider URLs, local artwork paths and actor/accountability references are not part of this detail payload.

## Performance contract

- one backend-scoped assignment-and-cast batch read for folder serialization;
- one count and one page query for dedicated person search;
- one count and one page query for global recording search;
- no SQL query per recording;
- no SQL query per person;
- no provider request during read paths;
- no schema DDL during repeated search or navigation;
- deterministic ordering by relevance, cast order, folded name and stable identity;
- existing repeated-folder-navigation performance from PR #135 must remain intact.

Regression tests use SQLite statement tracing to verify the constant query contracts and absence of repeated schema DDL.

## Security contract

The protected mutation continues to use `metadata.recording.assign`.

- the backend route is authoritative;
- Admin with exact backend scope is allowed;
- Read-only, wrong-backend and invalid-CSRF requests are denied before provider access;
- authorization and operation outcomes remain accountable;
- the existing managed backend credential resolver is reused;
- credentials are never returned, logged by the feature or copied into accountability data;
- search and detail routes remain read-only and provider-free.

## Automated validation matrix

### Provider and parser

- name, TMDB person ID, character and cast order;
- complete empty cast;
- malformed and oversized JSON;
- transport/timeout, HTTP 5xx and rate limit;
- bounded retry;
- no token text in errors.

### Persistence

- atomic movie and cast storage;
- provider-qualified person deduplication;
- one person referenced by several recordings;
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
- detail displays name, role and character without internal IDs or paths.

### REST and security

- credits are not loaded by candidate search or GET;
- provider failure creates no assignment;
- valid empty cast succeeds visibly;
- Admin allowed;
- Read-only, wrong-backend and invalid-CSRF denied;
- protected-operation accountability remains correct.

## Real yaVDR acceptance checklist

Use the exact final Draft-PR head and matching installed daemon/web assets.

1. Open a recording with automatic metadata and repeat folder/subfolder/back navigation several times; confirm PR #135 navigation performance remains fast.
2. Search movie candidates and confirm no unusual delay from credit loading before selection.
3. Select a known movie with several recognizable actors.
4. Confirm assignment succeeds and the recording detail shows actor names and character names.
5. Reload the page and confirm title, cast and character data remain.
6. Search globally for the selected manual title and original title; confirm the recording appears.
7. Search for one imported actor in the existing person search; confirm the person and recording appear.
8. Search globally for that actor; confirm the connected recording appears according to the existing global-search contract.
9. Restart `vdr-suited`; repeat the title and actor searches without a new assignment and confirm no provider dependency.
10. Reassign the recording to a different movie; confirm actors from the former movie no longer return the recording as active.
11. Withdraw the manual assignment; confirm automatic TVScraper/native title and people become effective again and manual actors no longer return the recording.
12. Restart again and confirm the withdrawn manual relationship does not reappear.
13. Repeat assignment as Read-only and against a backend outside the actor scope; confirm denial.
14. Confirm browser responses and accountability records contain no token, provider URL, local artwork path or actor reference.
15. Record the exact head, CI run, installed build identity and redacted recording identity.

The PR must remain Draft until this checklist is completed and the user explicitly approves readiness.
