# Token-free TVmaze series artwork provider

## Decision

VDR-Suite uses TVmaze as the packaged default for external series artwork.
The provider is token-free and therefore works for normal installations without
an account, API key, secret file, systemd drop-in, or manual post-install setup.
TVScraper remains the unchanged primary source. TVmaze is queried only when
SuiteBridge reports valid series metadata but no safely usable preferred artwork.

TMDB remains an optional alternative for operators who deliberately select it
and supply their own API Read Access Token. VDR-Suite never ships a shared TMDB
credential.

## Identity boundary

The provider never searches by title, original title, episode name, description,
or another fuzzy text field. It accepts only provider-qualified external IDs
whose scope is explicitly `series`:

1. exact IMDb series ID;
2. exact TheTVDB series ID.

A TMDB-only identity, movie identity, season identity, episode identity,
malformed value, or ambiguous metadata does not trigger TVmaze.

The lookup request uses TVmaze's exact `/lookup/shows` endpoint. TVmaze answers a
successful lookup with HTTP 301 and a `Location` identifying `/shows/<id>`.
VDR-Suite keeps redirects disabled in libcurl, validates that Location against
the exact TVmaze show-resource grammar, extracts only the positive numeric ID,
and constructs the next HTTPS request itself. No remote redirect is followed.

## Artwork selection

After exact identity resolution, VDR-Suite requests `/shows/<id>/images` and
accepts only bounded JSON with an `original` image URL on the exact
`static.tvmaze.com` host. Selection is deterministic:

1. landscape `background` images;
2. the main poster;
3. remaining posters;
4. closest expected aspect ratio;
5. larger pixel area;
6. stable image ID and URL tie-breakers.

The chosen PNG or JPEG is downloaded without credentials and passes through the
existing guarded incoming-file, content-validation, materialization,
persistence, and controlled public-delivery boundaries. Provider output never
overwrites TVScraper `preferredArtwork` directly.

## Network and failure policy

The shared external-artwork transport retains its exact HTTPS host allowlist,
TLS verification, disabled proxy/netrc/redirect behavior, private-address socket
rejection, bounded response sizes, and bounded timeouts. TVmaze adds only:

- `api.tvmaze.com` for exact lookup and image metadata;
- `static.tvmaze.com` for selected image bytes.

HTTP 429 and bounded transient server failures use the existing capped retry and
provider-cache policy. A miss or outage never disables TVScraper metadata, the
VDR backend, EPG cache, or frontend.

## Packaged defaults

The package enables the local Suite Bridge, the series-artwork fallback, and
`provider=tvmaze`. All timeouts, retry limits, cache TTLs, byte limits, source
roots, and managed cache roots are supplied with bounded values. No user secret
is required.

Startup deletion remains disabled by default. The existing incoming cleaner is
strictly TMDB-namespaced; TVmaze incoming-file cleanup is a separate future
boundary rather than an unsafe broad deletion rule.

## Attribution and license

TVmaze API data is provided under CC BY-SA. VDR-Suite identifies and links the
provider in `docs/third-party-data-providers.md`; redistributed builds must keep
that attribution. This integration is not endorsed by TVmaze.
