# Native SuiteBridge Recording Metadata Handoff

## Navigation

- [Plugin README](../README.md)
- [Shared SuiteBridge Handoff](VDR-SUITE-HANDOFF.md)
- [Plugin Roadmap](ROADMAP.md)
- [ADR-0036: TVScraper Recording Metadata Integration](../../docs/adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [TVScraper and Recording Metadata Roadmap](../../docs/planning/tvscraper-recording-metadata-roadmap.md)
- [Current Project Status](../../docs/development/current-status.md)

---

## Purpose

This document is the authoritative new-chat handoff for the next VDR-Suite
architecture slice:

> Import recording metadata, people and artwork natively from TVScraper through
> `vdr-plugin-suite-bridge` and TVScraper's public VDR service for
> `cRecording*`, then persist normalized results in VDR-Suite.

It replaces the assumption that RESTfulAPI recording JSON is the authoritative
source for TVScraper cast and artwork.

The browser must never communicate directly with TVScraper, IMDb, TMDB or a VDR
plugin.

---

## Repository checkpoint and workflow

| Item | Value |
| --- | --- |
| Repository | `hotzenplotz5/vdr-suite` |
| Source of truth | `main` |
| Integrated checkpoint | `f0715f1283839d021dd27c886b33cb989a4d1abd` |
| Checkpoint meaning | Recordings 2 plus Channels 2 EPG details, recording cards and current Timer confirmation state |
| Closed historical PR | `#90` |
| Unmerged unrelated PR | `#88` |

Development now proceeds directly on `main` unless the user explicitly requests
another branch.

Use small, logically complete commits as rollback points. Prefer `git revert`
over parallel long-lived branches. Do not force-push `main`.

Do not merge, cherry-pick or reproduce PR `#88` as part of this slice unless the
user explicitly asks for that separate HTTP/image-write isolation work.

---

## Proven live state

The following was observed on the real installation before this handoff:

```json
{
  "backendId": "default",
  "state": "ready",
  "cacheReady": true,
  "totalCount": 999,
  "lastError": ""
}
```

The persistent recording cache was therefore healthy and complete for the
current installation.

The recording-person endpoint nevertheless returned:

```json
{
  "totalCount": 0,
  "returnedCount": 0,
  "limit": 20,
  "offset": 0,
  "matches": []
}
```

for `Tom Hanks`, even though EPG TVScraper metadata exposed the correct cast and
the installation contains matching recordings.

The user confirmed the source boundary on the real VDR:

- rich EPG cast and artwork are available through SuiteBridge and TVScraper;
- RESTfulAPI supplies the recording inventory and technical recording data;
- RESTfulAPI does not supply the required authoritative recording cast in this
  installation.

Therefore a ready recording cache does not prove that recording people were
imported. The current person search reads a domain field that is not populated
from the authoritative provider on this installation.

---

## Existing implemented paths

### EPG metadata path

The working EPG path is:

```text
VDR EPG event
  -> SuiteBridge META / ARTW
  -> TVScraper public GetScraperVideo service
  -> typed Suite transport and parser
  -> normalized EPG metadata
  -> safe Suite-owned HTTP image URLs
  -> EPG / Scraper / Besetzung / Bilder frontend
```

The current plugin exposes:

```text
PLUG suitebridge CAPS [discovery-schema]
PLUG suitebridge SNAP
PLUG suitebridge ARTW <channel-id> <event-id>
PLUG suitebridge META <channel-id> <event-id>
```

Plugin version at the checkpoint is `0.11.0`.

### Recording inventory path

The current recording path obtains inventory and technical facts through the
existing VDR adapter, primarily RESTfulAPI, then stores recording read models in
the SQLite-backed recording cache.

That path remains valid for:

- recording identity and inventory;
- path and folder structure;
- start time, duration and size;
- recording actions and other existing mutation contracts.

It is not the authoritative TVScraper metadata source for this installation.

### Current broken person-search assumption

The existing recording-person search operates on `VdrRecording::persons` in the
current recording collection or snapshot. The RESTfulAPI mapper can populate
that field only when provider-specific actor data happens to exist in its JSON.

This is insufficient and non-portable. Alias merging or frontend changes cannot
repair metadata that never entered the recording domain from TVScraper.

---

## Architectural decision for this slice

The target ownership split is:

```text
RESTfulAPI / existing VDR adapter
  -> recording inventory and technical recording facts

SuiteBridge + TVScraper public VDR service for cRecording*
  -> provider metadata
  -> people and character names
  -> preferred artwork and gallery references
  -> provider IDs, ratings, genres and external IDs

VDR-Suite
  -> validation and normalization
  -> backend-scoped persistence and refresh policy
  -> person search and recording joins
  -> authenticated Suite-owned APIs and image delivery
  -> Recordings 2 and EPG-person recording cards
```

TVScraper remains authoritative for TVScraper-owned metadata. VDR-Suite must not
read TVScraper's SQLite database or private cache layout and must not reimplement
its scraper/update logic.

The frontend consumes provider-neutral VDR-Suite contracts only.

---

## Mandatory first analysis

Before changing code, trace the complete existing implementation and document
the findings. At minimum inspect:

### SuiteBridge and TVScraper

- `vdr-plugin-suite-bridge/services.h`
- `vdr-plugin-suite-bridge/suitebridge.cpp`
- `vdr-plugin-suite-bridge/suitebridge_tvscraper_adapter.cpp`
- `vdr-plugin-suite-bridge/suitebridge_epg_metadata_contract.h`
- `vdr-plugin-suite-bridge/suitebridge_epg_metadata_contract.cpp`
- `vdr-plugin-suite-bridge/tests/`
- `vdr-plugin-suite-bridge/README.md`
- `vdr-plugin-suite-bridge/docs/SB-6-read-only-svdrp.md`

Prove from the vendored/public service contract how `cGetScraperVideo` accepts
`cEvent*` and `cRecording*`. Do not rely only on prior chat statements.

### SuiteBridge transport and normalized EPG model

- `core/agent/include/SuiteBridgeSvdrpTransport.h`
- `core/agent/src/SuiteBridgeSvdrpMetadataTransport.cpp`
- `core/vdr/include/ISuiteBridgeMetadataTransport.h`
- `core/vdr/src/SuiteBridgeEpgMetadataResolver.cpp`
- existing EPG metadata parser, serializer, image-reference and repository code

Determine which contracts can be generalized and which must remain explicitly
EPG-owned. Do not rename or generalize working EPG code merely for symmetry.

### Recording domain, cache and runtime

- `core/vdr/include/VdrRecording.h`
- `core/vdr/include/VdrRecordingCacheRepository.h`
- `core/vdr/src/VdrRecordingCacheRepository.cpp`
- recording metadata JSON codec and serializer sources
- `core/vdr/include/VdrRecordingQueryService.h`
- `core/vdr/src/VdrRecordingQueryService.cpp`
- `core/vdr/include/VdrRecordingArtworkService.h`
- `core/vdr/src/VdrRecordingArtworkService.cpp`
- `core/daemon/include/DaemonRuntime.h`
- `core/daemon/src/DaemonRuntime.cpp`
- `mk/recording-metadata-tests.mk`
- `mk/daemon-sources.mk`

Prove exactly which metadata fields, people and artwork identities survive a
cache write, daemon restart and cache read.

### Person search and public API

- `core/vdr/include/RecordingPersonSearchService.h`
- `core/vdr/src/RecordingPersonSearchService.cpp`
- `api/rest/include/RecordingPersonSearchController.h`
- `api/rest/src/RecordingPersonSearchController.cpp`
- `RecordingPersonSearchResultJsonSerializer`
- `docs/development/person-api.md`
- associated tests and router wiring

Prove which collection the endpoint currently searches and why a ready cache can
still return zero people.

### Frontend consumers

- `web/frontend/recordings2.js`
- `web/frontend/epg-metadata-detail.js`
- `web/frontend/api/client-api.js`
- focused frontend tests

Do not redesign the frontend before the backend metadata source and persistence
contract are proven.

---

## Required recording identity contract

Do not use a volatile RESTfulAPI list number as the durable metadata key.

Define and test one backend-scoped canonical recording identity that can join:

- the recording returned by the existing inventory adapter;
- the live VDR `cRecording` found by SuiteBridge;
- the persistent SQLite recording cache;
- artwork and person records;
- public API responses.

Likely evidence sources include the canonical VDR recording path and the existing
`backendNativeId`, but the implementation must prove their semantics from the
current code and live payloads.

The SuiteBridge command must never treat caller input as an arbitrary filesystem
path to open. It must resolve a bounded, validated identity only against VDR's
current recording list.

Path encoding, maximum input size, normalization, duplicate aliases, missing
recordings and moved/deleted recordings require explicit tests.

---

## Required SuiteBridge recording metadata contract

Introduce a separate read-only recording metadata command rather than
reinterpreting the existing EPG `META` command. The final command name must be
chosen after reviewing current SVDRP naming and parser constraints. `RMETA` is a
candidate, not an unreviewed mandate.

The command must:

1. accept one bounded encoded recording identity;
2. resolve that identity only within VDR's current recording list;
3. call TVScraper through its public `GetScraperVideo` service with the matching
   `cRecording*` contract;
4. return one bounded, versioned JSON document;
5. distinguish `found:false` from transport, validation and provider failures;
6. expose no arbitrary filesystem access;
7. transmit metadata and validated image references, not image bytes;
8. remain read-only and leave all SuiteBridge counters and mutation state
   unchanged unless an explicit existing contract requires otherwise.

The schema should reuse field semantics already proven by EPG metadata where
appropriate, including:

- media type and provider ID;
- title, original title, series and episode titles;
- overview and tagline;
- season, episode and runtime;
- genres, rating and external IDs;
- bounded people with role, character and portrait reference;
- preferred artwork and bounded gallery entries.

Do not copy an EPG field merely because it exists. Recording-only provenance and
identity fields must be explicit.

---

## VDR locking and object-lifetime proof

This is a critical acceptance gate.

The implementation must prove a safe lifetime strategy for the `cRecording*`
passed to TVScraper. Do not assume that the EPG detached-event approach can be
copied directly or that `cRecording` is safely copyable.

Analyze VDR's recording-list lock semantics, the TVScraper service call and the
current adapter implementation. Document whether the service call:

- occurs while a VDR recording read lock is held;
- uses a safe detached representation accepted by TVScraper;
- or requires another proven lifetime mechanism.

Avoid holding a global VDR lock across slow filesystem inspection or unrelated
work. Also avoid releasing the lock while retaining an invalid `cRecording*`.
No compromise is acceptable without source-level evidence and tests.

---

## Persistence and refresh requirements

Provider metadata must be persisted backend-scoped in VDR-Suite so that:

- Recordings 2 does not perform synchronous SuiteBridge calls while rendering;
- person search works after daemon restart;
- people and character names survive cache round-trips;
- cover/artwork identity survives without exposing provider paths;
- metadata origin, schema and freshness remain explicit;
- `found:false` can be negatively cached for a bounded period;
- moved, deleted or changed recordings invalidate or migrate predictably.

Do not start one synchronous request per recording during a browser request. The
real installation currently has about 999 cached recordings.

Use a bounded background enrichment pipeline that processes only missing,
changed or expired entries, with:

- backend-scoped queueing;
- deduplication;
- bounded concurrency;
- retry/backoff for transport failures;
- separate negative-cache TTL;
- clean shutdown;
- visible cache/enrichment status;
- deterministic tests without a real TVScraper installation.

Choose normalized tables or a versioned cache payload only after inspecting the
existing repository and migration conventions. People must be queryable without
loading and reparsing all recordings for every search.

---

## Person search and frontend completion

After native recording metadata is persisted:

1. change recording-person search to query the authoritative persistent
   recording metadata/person source;
2. keep backend scoping, limits, offsets and deterministic ordering;
3. return recording presentation metadata and safe poster URLs in the existing
   result contract;
4. preserve the current UI placement directly below the selected person card;
5. render recording cards with cover when available and deterministic fallback
   otherwise;
6. keep card opening and recording detail behavior inside VDR-Suite;
7. remove or retire RESTfulAPI-derived person/alias work only after regression
   coverage proves the new source.

An EPG person click must be able to find matching stored recordings even when
RESTfulAPI exposes no cast data.

---

## Compatibility and capability rules

The recording metadata feature must be capability-advertised and backend-scoped.
A backend without SuiteBridge/TVScraper recording metadata must degrade to basic
recording inventory without pretending that cast or artwork is available.

Review ADR-0036. Its provider-neutral rules remain useful, but its statement that
RESTfulAPI actor payloads provide the real integration source is not valid for
the observed installation. Amend the ADR or add a superseding clarification if
the source analysis confirms SuiteBridge `cRecording*` as the canonical
TVScraper path.

Do not break EPG-only installations, mock backends or non-TVScraper backends.

---

## Out of scope

Do not mix the following into this architecture slice:

- Timer-create/readback behavior;
- PR `#88` HTTP image-writer isolation;
- EPG timeline geometry or wrapping;
- recording mutations;
- direct TMDB or IMDb browser integration;
- TVScraper database access;
- broad frontend redesign;
- branch cleanup across the whole repository.

Existing unrelated behavior may be regression-tested but must not be redesigned.

---

## Required tests

### Plugin

- command parsing and bounded identity validation;
- missing, malformed, moved and duplicate recording identities;
- `found:false` and provider failure distinction;
- exact schema and deterministic field ordering;
- payload-size rejection without truncated JSON;
- people, character, portrait, artwork and gallery bounds;
- no mutation and no counter side effects;
- safe `cRecording*` lifetime/lock contract;
- final shared-object build.

### Agent and backend

- typed transport reply-code handling;
- strict parser and schema compatibility;
- backend-scoped resolver behavior;
- persistent metadata/person/artwork round-trip;
- migration from the existing recording cache;
- daemon restart persistence;
- queue deduplication, retry, negative cache and shutdown;
- moved/deleted recording invalidation;
- person-search results from persisted native metadata;
- no dependence on RESTfulAPI actor fields.

### API and frontend

- recording-person API with cover and presentation data;
- backend, limit, offset and error contracts;
- EPG person click to inline recording cards;
- poster, placeholder and expandable detail behavior;
- no direct browser access to TVScraper/provider paths;
- Recordings 2 regression;
- EPG detail and timeline regression.

Run focused local tests and builds. Do not wait for GitHub Actions before asking
for real-installation verification.

---

## Required live acceptance

Use at least one known recording whose TVScraper metadata contains a known
person, for example a recording of `Forrest Gump` with `Tom Hanks` when present
on the test system.

Prove all of the following on the real VDR:

1. the recording inventory exists through the normal recording path;
2. the new SuiteBridge recording metadata command returns valid bounded JSON;
3. the result contains the expected person and preferred artwork when TVScraper
   has them;
4. enrichment persists the result in SQLite;
5. the result survives daemon restart;
6. `/api/vdr/recordings/persons/search?...name=Tom%20Hanks...` returns the
   recording;
7. the frontend renders the recording card directly below the selected EPG
   person with its cover;
8. no TVScraper path is exposed publicly;
9. VDR recordings, timers, channels and setup remain unchanged;
10. rollback and service restart are clean.

Record exact commands and observed output in this handoff or a linked completion
document.

---

## Delivery rules

- Analyze the complete code path before implementation.
- Do not stop after isolated intermediate findings.
- Do not ask for confirmation after every small step.
- Do not use superficial frontend workarounds.
- Make changes directly on `main` in small, coherent commits.
- Push each accepted checkpoint to `origin/main`.
- Keep the worktree clean between checkpoints.
- Provide focused local tests and real-VDR verification commands.
- Do not claim a full build or live result that was not actually executed.
- Update this handoff with the final contract, commit IDs and acceptance state.

---

## New-chat entrypoint

The next chat must begin by reading this document from `main` and treating it as
the operational source of truth for the recording metadata slice. It must also
re-read the referenced source files rather than trusting chat memory.

The first response should report:

1. confirmed `main` head;
2. the existing EPG metadata path;
3. the exact current recording-person data source;
4. the proven `cRecording*` TVScraper service contract;
5. the proposed minimal implementation sequence;
6. any contradiction found between this handoff, ADR-0036 and the current code.
