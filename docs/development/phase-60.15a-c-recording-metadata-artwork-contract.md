# Phase 60.15a-c - Recording Metadata and Artwork Contract

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current State](../CURRENT.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [ADR-0038](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0036](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)

---

## Status

```text
Phase 60.15 is in progress.
Internal slices 60.15a through 60.15c are implemented by this contract.
The full Phase 60.15 exit gate is not complete.
```

This document records the first runtime foundation for Recording metadata and artwork preparation. It does not claim that Recording cache responses or the Web frontend already display posters.

---

## Proven Current Input

The current `VdrRecording` read model contains technical Recording facts and the existing person collection. It does not yet own normalized metadata or artwork.

The current RESTfulAPI Recording payload already provides two different information classes:

```text
native Recording event text
  event_title
  event_short_text
  event_description

optional scraper bridge enrichment
  additional_media.type
  movie and series identifiers
  titles, overview, genres, dates and rating
  movie poster and fanart references
  series poster, banner and fanart arrays
  episode still reference
```

The source shape was verified against the current `yavdr/vdr-plugin-restfulapi` Recording and scraper2vdr serialization code. Current upstream names are treated as canonical. A limited set of older aliases remains accepted only as transition compatibility.

---

## Field Ownership Decision

Phase 60.15 separates three concepts before any frontend integration:

```text
VdrRecordingNativeMetadata
  text copied from the native Recording event

VdrRecordingProviderMetadata
  optional provider-derived descriptive enrichment

VdrRecordingArtworkRef
  one temporary source-scoped artwork reference
```

They are grouped in:

```text
VdrRecordingMetadata
```

This separation prevents provider values from silently replacing technical Recording identity or native VDR facts.

### Native metadata

Native fields are:

- event title;
- short text;
- description.

They remain valid when no scraper or external metadata provider exists.

### Provider metadata

Provider-derived fields currently include:

- content kind: movie or series episode;
- source kind;
- movie, series and episode provider identifiers;
- title and original title;
- series and episode title;
- tagline and overview;
- genre text;
- release or first-aired date;
- season and episode number;
- runtime in minutes;
- rating.

The provider identifiers are evidence from the scraper bridge. They are not VDR-Suite `MetadataEntityId` values and are not public stable identity.

### Artwork reference

Each reference contains:

```text
kind
source
reference
width
height
temporary
```

Supported kinds are:

- poster;
- fanart;
- banner;
- still.

The reference is deliberately not a public URL and not a permanent Suite artwork identity. Phase 61 replaces temporary source references with Suite-owned `ArtworkAsset` identity, storage and delivery.

---

## Mapping Rules

`RestfulApiRecordingMetadataMapper` accepts the JSON object for one Recording.

### Movie mapping

Current fields include:

```text
type = movie
movie_id
title
original_title
tagline
overview
genres
release_date
runtime
vote_average
poster
fanart
```

### Series and episode mapping

Current fields include:

```text
type = series
series_id
episode_id
name
overview
genre
rating
episode_number
episode_season
episode_name
episode_first_aired
episode_overview
episode_rating
episode_image
posters[]
banners[]
fanarts[]
```

For image arrays, path, width and height are preserved.

### Transition aliases

The mapper temporarily accepts selected historical aliases such as:

```text
scraper instead of type
movie_title instead of title
movie_overview instead of overview
movie_poster instead of poster
```

Compatibility aliases do not become the preferred contract.

---

## Artwork Safety Boundary

Source artwork values are stored only as relative source-scoped references.

The mapper rejects:

- absolute filesystem paths;
- HTTP or HTTPS URLs;
- other URI-style values;
- parent-directory traversal;
- encoded path or traversal separators;
- empty references.

This prevents a provider payload from becoming an arbitrary browser URL, filesystem path or open proxy input.

The frontend must not concatenate these references with a backend address. A later Suite-owned resolver or artwork delivery endpoint is required.

---

## Build and Regression Integration

The mapper source is part of the normal `VDR_SRC` build graph, so the daemon build verifies it even before the read model consumes it.

The focused test target is:

```text
test-restful-api-recording-metadata-mapper
```

It is attached to:

```text
test-restful-api-recording-mapper
test-fast
```

Coverage includes:

- native event text without provider data;
- current movie payload mapping;
- current series and episode payload mapping;
- poster, fanart, banner and still references;
- image dimensions;
- legacy alias compatibility;
- duplicate and unsafe path rejection;
- unsupported provider types remaining unclaimed.

---

## Deliberately Not Implemented Yet

This slice does not yet:

- add metadata to `VdrRecording`;
- persist enrichment in the lazy Recording cache;
- add metadata to Recording JSON responses;
- create Suite-owned artwork identity;
- proxy or serve artwork bytes;
- render a poster or placeholder in the Web frontend;
- query TVScraper or scraper2vdr directly from the frontend;
- implement the Phase 61 metadata database.

The absence of visible images after this slice is therefore expected.

---

## Next Internal Slice

The next work follows the accepted Phase 60.15 order:

```text
60.15d deterministic no-provider placeholders
60.15e Recording list and detail presentation
60.15f lazy Recording cache preservation
60.15g compatibility and regression coverage
60.15h Phase 61 migration contract
```

Before visible provider artwork can be shown safely, the Recording read model and lazy cache must carry the metadata contract and a Suite-owned delivery boundary must resolve temporary source references.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current State](../CURRENT.md)
