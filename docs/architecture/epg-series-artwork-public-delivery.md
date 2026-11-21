# EPG series artwork public delivery

## Scope

This boundary makes an already materialized and persisted series-artwork fallback usable by existing API clients. It does not add a provider, perform network access, change TVScraper, redesign the frontend, or create a second public file-serving route.

The existing authenticated endpoint remains the only client-visible reference:

```text
/api/epg/cache/metadata/image?backend=...&channelId=...&eventId=...&kind=preferred&index=0
```

Public metadata continues to expose only this controlled reference plus dimensions. It never exposes a provider URL, bearer token, local path, cache root, provider cache key, or provider-specific response data.

## Selection order

1. A valid and safely readable primary TVScraper `preferredArtwork` is attempted first.
2. Fallback delivery is considered only for `kind=preferred&index=0`.
3. Person and gallery images never use the series fallback.
4. The fallback must have crossed all of these boundaries:
   - provider-neutral resolution;
   - secure materialization;
   - persistence in `epg_series_artwork_fallback`;
   - persisted provenance `external-fallback`;
   - valid provider, absolute normalized path, positive dimensions and timestamp;
   - managed-root and file-content validation at delivery time.

A provider candidate or materialized file is not public-delivery eligible by itself. The internal `managed` marker is asserted only after persistence succeeds or a valid persisted record is rehydrated.

## Runtime flow

```text
SuiteBridge metadata
  -> optional provider-neutral fallback resolver
  -> optional TMDB provider
  -> secure materializer
  -> persistent fallback resolver
       - validates managed root
       - persists explicit ExternalFallback provenance
       - asserts managed delivery eligibility
       - exposes read-only delivery capability
  -> persistent public metadata resolver
       - preserves the delivery capability
       - serializes only the controlled image URL
  -> EpgCacheController
       - serves primary first
       - otherwise loads the persisted fallback
  -> existing authenticated metadata image route
```

When the fallback runtime is disabled, the chain has no usable fallback-delivery provider and public behavior remains unchanged.

## Persistence contract

`epg_series_artwork_fallback` remains separate from primary TVScraper artwork persistence. Each accepted record must contain:

- backend, channel and event identity;
- a constrained non-primary provider name;
- provenance exactly `external-fallback`;
- an absolute normalized path without `.` or `..` components;
- positive width and height;
- a positive resolution timestamp.

The schema setup adds the provenance column to the previous PR-3 table when needed. Existing rows created by that guarded persistence boundary receive the explicit `external-fallback` default. This is a schema qualification, not an import of arbitrary legacy files.

## File access boundary

The delivery service does not use browser-provided paths and does not use a generic local-file endpoint.

For every request it:

- resolves the record by backend, channel and event identity;
- requires explicit persisted fallback provenance and valid metadata;
- accepts only configured managed roots other than `/`;
- opens the root from `/` component by component;
- opens child directories and the final file with `openat(..., O_NOFOLLOW)`;
- requires a regular file with `fstat`;
- enforces an exact bounded read and verifies the descriptor identity and size after reading;
- validates PNG structure and CRCs or JPEG framing and dimensions from bytes;
- requires the detected type to match the managed `.png`, `.jpg` or `.jpeg` suffix;
- requires detected dimensions to match persisted dimensions;
- enforces byte, dimension and pixel limits;
- returns only `image/png` or `image/jpeg`.

Any mismatch fails closed as a generic not-found response. No local path is included in an error.

## HTTP boundary

The existing HTTP listener remains responsible for response security headers and applies them to this route:

- `X-Content-Type-Options: nosniff`
- controlled `Cache-Control: no-cache`

The route remains behind the existing security gate. No bypass route or unauthenticated static-file mount is introduced.

## Compatibility

- Primary TVScraper artwork keeps precedence.
- Existing metadata without fallback remains unchanged.
- Existing clients continue using the same `preferredArtwork.url` contract.
- No provider-specific public fields are added.
- No frontend redesign is included.
- With fallback disabled, no new image becomes visible.

## Deliberately deferred

The following remain separate review boundaries:

- managed-cache orphan cleanup;
- incoming-file cleanup;
- broad telemetry and operator dashboards;
- additional artwork providers;
- automatic migration of unguarded legacy files;
- larger frontend or TV-client changes.
