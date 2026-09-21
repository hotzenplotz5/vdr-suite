# EPG series artwork fallback runtime

## Purpose

The optional series-artwork fallback is a daemon-side enrichment layer for EPG
series metadata that already resolved successfully through TVScraper but has no
valid primary artwork.

TVScraper remains the primary metadata and artwork source. The fallback layer
must never replace valid TVScraper artwork and must not change the SuiteBridge
trust boundary.

## Runtime chain

```text
SuiteBridgeEpgMetadataResolver
    -> SeriesArtworkFallbackResolver
    -> PersistentEpgScraperMetadataResolver
    -> registered EPG metadata resolver
```

The decorator runs only when all of the following conditions are true:

- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED` is enabled;
- a fallback provider has been injected;
- the delegate resolution was attempted and found;
- the metadata contract is valid TVScraper metadata;
- the media type is `Series`;
- `preferredArtwork` is missing;
- no fallback candidate is already attached.

## Candidate isolation

A provider result is stored in `seriesArtworkFallback`, not in
`preferredArtwork`. The candidate must declare:

- an external provider other than `tvscraper` or `none`;
- `ExternalFallback` origin;
- a non-empty local candidate path;
- positive image dimensions.

The candidate is intentionally not serialized into the public metadata JSON,
not persisted as selected artwork, and not served by an HTTP route in this
slice. A later secure materialization layer must validate file content, path
containment, dimensions, provenance, and cache policy before public selection.

## Activation state in this slice

The runtime switch defaults to disabled. The production runtime constructs the
decorator but injects no provider (`nullptr`). Consequently, enabling the switch
alone still produces no network access and no visible behavior change.

This is deliberate: provider selection, network execution, matching, negative
cache, secure download, persistence, and public selection are separate review
boundaries.

## Safety invariants

- valid primary TVScraper artwork always wins;
- movies and unresolved metadata never invoke the provider;
- invalid, ambiguous, or incomplete provider results are ignored;
- the existing TVScraper-only `EpgScraperArtwork::valid()` contract is not
  weakened;
- no external URL reaches the browser;
- no provider dependency is linked into the daemon in this slice.
