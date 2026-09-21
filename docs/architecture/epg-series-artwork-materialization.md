# EPG series artwork materialization

## Scope

This slice establishes the daemon-side trust boundary for optional external
series artwork. It does not add a provider, network client, matching policy,
public fallback selection, or frontend behavior.

The runtime remains disabled by default. Even when enabled, the currently
wired provider pointer is `nullptr`, so production cannot produce a new raw
candidate in this slice.

## Resolver order

The backend-scoped metadata chain is:

1. `SuiteBridgeEpgMetadataResolver`
2. `SeriesArtworkFallbackResolver`
3. `SeriesArtworkFallbackMaterializingResolver`
4. `PersistentSeriesArtworkFallbackResolver`
5. `PersistentEpgScraperMetadataResolver`

The primary TVScraper artwork remains in `preferredArtwork`. External series
artwork remains in the separate internal `seriesArtworkFallback` field and is
never promoted by this slice.

## Trust boundaries

### Incoming boundary

A future provider may return only an absolute local file path under an
operator-configured incoming root. The defaults are:

- incoming root: `/var/cache/vdr-suite/epg-artwork/incoming`
- managed cache root: `/var/cache/vdr-suite/epg-artwork/external`

Incoming roots are read-only inputs to the materializer. A candidate outside
those roots is rejected.

### File validation boundary

The filesystem materializer:

- resolves the candidate below an allowed incoming root
- opens the final source path with `O_NOFOLLOW`
- accepts only regular files
- enforces a bounded byte count
- recognizes PNG or JPEG from file content rather than filename extension
- validates PNG chunk boundaries and CRC values
- validates JPEG framing and reads dimensions from a start-of-frame segment
- rejects reported dimensions that differ from the file dimensions
- enforces maximum dimensions and a pixel-count ceiling

This is bounded structural validation, not a general-purpose image decoder.
A later provider slice may add decoding or transcoding before public use.

### Managed-cache boundary

Validated bytes are copied into a daemon-managed cache. Backend, channel, and
event identifiers are hex encoded before they become path components.
Directories are opened relative to trusted directory descriptors with
`O_NOFOLLOW`. Files are written with exclusive temporary names, synchronized,
and atomically renamed to the final event-local path.

Runtime configuration accepts a cache root only when it is a strict descendant
of `/var/cache/vdr-suite/epg-artwork`.

## Persistence

Materialized fallback artwork is stored in the separate
`epg_series_artwork_fallback` table. The table rejects empty providers and the
reserved `none` and `tvscraper` providers.

The persistence resolver stores only external-fallback artwork below the
configured managed cache root. On a later metadata resolution it may rehydrate
the retained internal candidate when the file path still satisfies that root
policy.

The existing primary artwork tables and `preferredArtwork` field are not
modified by fallback persistence.

## Fail-closed behavior

The internal candidate is cleared when any of the following applies:

- the runtime switch is disabled
- the resolution is missing, invalid, or not a series
- no materializer is available
- the fallback schema cannot be initialized
- source validation or atomic storage fails
- the managed cache path is no longer allowed
- persistence fails and no valid retained entry exists

Raw incoming paths therefore cannot pass into downstream persistence or public
serialization.

## Public boundary

This slice adds no HTTP route and no public JSON field. The public metadata
serializer continues to ignore `seriesArtworkFallback`, so provider names and
local paths remain daemon-internal.

## Runtime configuration

- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED`
- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_SOURCE_ROOTS`
- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_CACHE_ROOT`
- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_BYTES`
- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_DIMENSION`

Invalid values fall back to bounded defaults. The enablement switch remains
`false` by default.

## Deferred review boundaries

Separate follow-up changes are required for:

- a concrete external metadata or artwork provider
- bounded HTTP execution and credential handling
- identity matching and provider precedence
- negative caching, retry policy, and rate limits
- complete image decoding or safe transcoding
- stale-file and orphan cleanup
- telemetry and operational diagnostics
- deliberate public selection and delivery of fallback artwork
