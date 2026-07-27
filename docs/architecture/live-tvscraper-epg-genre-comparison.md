# Live 3.5.5 / TVScraper 1.2.15 EPG Genre Comparison

## Status

```text
Historical Phase 61 diagnostic and acceptance evidence
Completed; no active feature branch or pending acceptance
```

This document records the source/runtime comparison that supported the completed Phase 61 EPG Genre path. The former status referring to `feature/phase61-metadata-genre-browser` is superseded by the [Phase 61 closeout](../development/phase-61-metadata-genre-performance-closeout.md).

## Scope

The comparison covers only the read-only EPG metadata/Genre resolution path. It does not change:

- the EPG timeline;
- Recordings 2;
- RemoteAction/LiveOverlay routing;
- Timer/SearchTimer behaviour.

## Analysed production versions

```text
VDR:       2.7.9
Live:      v3.5.5 / e9de9ba07335f9ea4aa451fc3f7a765aefcbd6cb
TVScraper: v1.2.15 / dbfd299678977363e8041e54e3d134c05287b73d
SuiteBridge diagnostic generation: 0.13.1
```

Authoritative upstreams were `MarkusEh/vdr-plugin-live` and `MarkusEh/vdr-plugin-tvscraper`.

## Live EPG path

Live:

1. holds the required VDR read locks;
2. resolves Channel/Schedule and the real schedule-owned `cEvent` by event ID;
3. creates `cGetScraperVideo(event, nullptr)`;
4. calls TVScraper `GetScraperVideo`;
5. keeps the returned `cScraperVideo` in request-local EPG state;
6. reads media type, Movie/Series metadata, Genres, people and images from that same resolver object.

Live does not call a second Genre resolver. Movie/Series Genres come from the `genres` vector returned by `getMovieOrTv(...)`. Episode data has no separate Genre field in this service contract.

## Live Recording path

Live uses a real `cRecording*` under the Recording read lock and calls the same `GetScraperVideo` service with the Recording. Its Recording tree caches provider metadata and is invalidated through `cGetScraperUpdateTimes`.

VDR-Suite Recordings 2 uses its own RMETA/SQLite path. The Phase 61 EPG diagnostic did not replace or bypass that owner.

## TVScraper resolution facts

For EPG events, TVScraper resolves the database mapping by:

```text
event_id + channel_id
```

Movie metadata comes from `movies3`; Series metadata comes from `tv2`. Their Genre fields are pipe-separated strings. TVScraper's conversion splits at `|`, removes empty/duplicate values and otherwise preserves the supplied values.

Therefore Live's displayed Genre list is determined by the TVScraper Movie/Series Genre vector, not by a separate Live taxonomy.

## SuiteBridge paths

### ETYPES

ETYPES builds a bounded 48-hour event snapshot, re-resolves real Channel/Schedule events, verifies event/time identity and calls TVScraper under the documented read-lock context. It persists only Movie/Series media-type evidence.

### META

The diagnostic compared the historical copied-event META path with the real schedule-owned event path used by Live. The accepted implementation aligns the provider call with real VDR event lifetime/locking rules and stores the returned public metadata/Genres as provider evidence.

### Evidence separation

```text
TVScraper Genre evidence
  provider_id = tvscraper
  source_kind = scraper-metadata

TVScraper media-type evidence
  provider_id = tvscraper-media-type
  source_kind = scraper-media-type

Derived Suite browse class
  provider_id = suite-epg-browse
  source_kind = epg-browse-content-class
```

This prevents media type or derived navigation classes from silently becoming flat provider Genres while preserving provenance.

## Transport and payload boundary

SuiteBridge serializes public metadata and sends it through bounded SVDRP commands. Transport responses require successful VDR/SVDRP status and are parsed into Suite-owned persistence by the adapter/runtime.

The current Recording metadata contract is separate and consistently bounded at 128 people / 65,535 bytes. The old Draft PR #101 plugin-only 256/256 KiB proposal is not part of this accepted EPG comparison or current end-to-end contract.

## Accepted EPG classification result

The Phase 61 read model persists exactly four main EPG classes:

```text
Film
Serie
Dokumentation
Sport
```

Decision precedence is documented in [Metadata-Backed Genre Browser](metadata-genre-browser.md). Ambiguous evidence is not guessed. Film subgenres require both Movie classification and a matching canonical Film Genre.

## Authoritative cache reconciliation

Real-system diagnosis proved that RESTfulAPI can replace backend-native event IDs for the same schedule occurrence while stale cache rows remain. The accepted cache reconciliation is deliberately bounded:

- native event IDs remain backend-scoped cache identities;
- title/subtitle/time similarity never silently rebinds canonical identity;
- potentially truncated channel pages are not destructively reconciled;
- missing native IDs are removed only inside a proven backend/channel/time window;
- dependent cache/artwork rows are cleaned while Genre evidence is retired/staled rather than rebound;
- metadata GETs for retired IDs return a stale-event result and do not enqueue provider work;
- frontend pending responses use bounded retry and are not cached forever.

## Runtime comparison conclusion

The accepted Suite path reproduces the relevant Live/TVScraper metadata source semantics while adding stronger application boundaries:

- provider evidence is persisted and stateful;
- backend scope is explicit;
- normal reads remain query-only/provider-free;
- cached browse capability survives provider outages;
- canonical Genre aliases and derived navigation are Suite-owned;
- the browser does not learn TVScraper/SuiteBridge internals.

## Historical evidence rule

This document remains valuable diagnostic evidence but is not a current planning entry point. Current status is maintained in:

- [Current State](../CURRENT.md)
- [Phase 61 Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Provider Strategy](../planning/tvscraper-recording-metadata-roadmap.md)