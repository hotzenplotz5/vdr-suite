# TMDB series artwork provider

## Scope

This slice adds the first concrete provider behind the provider-neutral series
artwork fallback boundary. TMDB remains optional, disabled by default, and
internal to the daemon. It does not add a public artwork route, public JSON
field, frontend behavior, or promotion into `preferredArtwork`.

The provider can run only when all of these conditions hold:

- SuiteBridge EPG metadata is enabled for the backend;
- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED=true`;
- `VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER=tmdb`;
- a syntactically valid `VDR_SUITE_TMDB_READ_ACCESS_TOKEN` is present;
- the persistent provider-cache schema is available;
- the metadata represents a series with no valid primary artwork;
- a provider- and scope-qualified series identity is available.

## Identity matching

No title search or fuzzy matching is permitted. Identity precedence is:

1. qualified TMDB series ID;
2. qualified IMDb series ID resolved through TMDB `/find` with
   `external_source=imdb_id`;
3. qualified TVDB series ID resolved through TMDB `/find` with
   `external_source=tvdb_id`.

Episode-, season-, movie-, unknown-, malformed-, or ambiguous identities are
not accepted. `/find` must return exactly one TV result; zero or multiple
results become a bounded negative-cache entry.

## Request flow

For an accepted series identity the provider performs at most:

1. one `/find` request when the identity is not already a TMDB series ID;
2. one `/tv/{series_id}/images` request;
3. one `image.tmdb.org/t/p/original` image request.

The API requests use the operator-provided bearer token. The public image CDN
request deliberately omits that token.

## Transport boundary

The curl transport accepts only HTTPS URLs whose exact host is one of:

- `api.themoviedb.org`;
- `image.tmdb.org`.

Redirects, proxies, netrc credentials, userinfo, explicit ports, non-HTTPS
protocols, and non-allowlisted hosts are rejected. TLS peer and hostname
verification remain enabled. Resolved loopback, link-local, private, multicast,
and other non-public IP ranges are rejected in the socket-open callback.

Connect timeout, total timeout, maximum response bytes, response-body growth,
and retry count are bounded. Only 429 and selected transient 5xx or transport
failures are retried. Retry-After is capped by the provider policy.

## Response and image selection

TMDB JSON is parsed by a bounded parser with maximum size, nesting depth, array
count, and string-length limits. Image paths must be single safe TMDB file
components with PNG or JPEG suffixes.

Series cover selection is deterministic. Valid posters are preferred over all
backdrops and are ranked by:

1. configured language base, for example `de` from `de-DE`;
2. language-neutral image;
3. English image;
4. any other valid language;
5. smallest deviation from the 2:3 cover ratio;
6. highest pixel count;
7. highest vote average;
8. lexical file path as a stable tie-breaker.

Only when no valid poster exists does selection fall back to a backdrop. The
same language, pixel-count, vote, and stable-path order applies, but aspect
ratio is ranked against 16:9.

The downloaded bytes are written atomically to the configured incoming root.
The existing materializer remains responsible for content signature, image
structure, dimensions, root containment, cache storage, and persistence.

## Negative and transient cache

The separate `epg_series_artwork_provider_cache` table is keyed by provider,
identity provider, and identity value. It stores only:

- `not-found` outcomes with a bounded negative-cache TTL;
- `temporarily-unavailable` outcomes with a shorter bounded TTL.

Successful materialization removes the matching cache entry. Expired entries
are deleted lazily on lookup. The cache never stores bearer tokens, remote
URLs, response bodies, or artwork bytes.

## Secret handling

The packaged example does not contain a token. Operators should provide the
TMDB API Read Access Token through a mode-0600 systemd environment file or
service drop-in. The token is held only in the provider runtime configuration,
used only in TMDB API Authorization headers, and is not copied into logs,
public metadata, diagnostics, or persistence.

## Runtime order

The backend-scoped chain is:

1. SuiteBridge metadata resolver;
2. optional TMDB transport, provider cache, and provider;
3. provider-neutral fallback resolver;
4. secure filesystem materializer;
5. separate fallback persistence;
6. existing primary metadata persistence.

If provider configuration or provider-cache schema setup fails, the provider
pointer remains null. Existing TVScraper metadata, materialization retention,
and primary persistence continue without network access.

## Deferred boundary

This slice deliberately leaves fallback artwork internal. Public selection,
authenticated delivery, stale incoming-file cleanup, managed-cache orphan
cleanup, detailed provider telemetry, and operator-visible health diagnostics
remain separate review boundaries.
