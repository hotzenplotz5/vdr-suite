# Public API v1 — Recording Artwork (M2, read-only)

M2 reuses VDR-Suite's existing warmed Recording cache,
`VdrRecordingArtworkIdentity`, and `VdrRecordingArtworkService`.
It does not add a second artwork database, provider, thumbnail generator
or filesystem scanner.

## Client flow

Each GET requires authentication (existing Browser Session or Device
credential) and an explicit `recordings.view` grant on the exact backend.

1. `GET /api/v1/recordings?backendId=backend-a` obtains the existing `rec_` ID.
2. `GET /api/v1/recordings/{recordingId}/artwork?backendId=backend-a`
   returns existing poster, fanart, banner and still references, if available.
3. `GET /api/v1/recordings/{recordingId}/artwork/{artworkId}?backendId=backend-a`
   returns actual PNG, JPEG or WebP bytes through the same security gate.
4. Existing EPG events also have a direct, authenticated Artwork byte
   endpoint: `GET /api/v1/epg/artwork?backendId=backend-a&channelId=...&eventId=...`.
   This delegates to the current EPG Artwork repository, not a new provider.
   The response uses the same image MIME types and private cache policy,
   with `epg.view` required (a Live-only grant does not authorize it).
5. If artwork is absent, the list returns `items: []`. If the Recording
   or an individually requested image no longer exists, treat HTTP 404
   as a missing image and render a placeholder rather than constructing
   native VDR paths or remote provider queries.

Example artwork list response (illustrative IDs):

```json
{
  "recordingId": "rec_0123456789abcdef0123456789abcdef",
  "backendId": "backend-a",
  "items": [{
    "artworkId": "fedcba9876543210fedcba9876543210",
    "kind": "poster",
    "url": "/api/v1/recordings/rec_0123456789abcdef0123456789abcdef/artwork/fedcba9876543210fedcba9876543210?backendId=backend-a",
    "width": 640,
    "height": 960
  }]
}
```

Image responses include `Content-Type: image/jpeg`, `image/png` or
`image/webp`, `X-Content-Type-Options: nosniff` and
`Cache-Control: private, max-age=300`. The list is `no-store`.
Only indexed, safe-root, bounded files whose signatures match their formats
can be delivered. No provider credentials, absolute paths, filesystem
references or arbitrary client-supplied URLs are returned or followed.

The relative image URL is **not** a bearer token; every image GET needs
normal authentication. Device clients must request bytes with their
authorization header: ordinary browser `<img>` cannot attach a Device
header, so use an authenticated fetch and a Blob/object URL instead.
Do not pass credentials in query parameters.

### Reference JavaScript

```javascript
const refs = await client.getRecordingArtwork({
  recordingId: recId, backendId: 'backend-a'
});
if (refs.items.length) {
  const {bytes, contentType} = await client.getRecordingArtworkImage({
    recordingId: recId, artworkId: refs.items[0].artworkId,
    backendId: 'backend-a'
  });
  // In a browser: URL.createObjectURL(new Blob([bytes], {type: contentType}))
}
```

The client preserves its configured credentials/session settings.
Status: 400 invalid ID/backend query, 401 missing authentication, 403
missing `recordings.view`, 404 absent/retired Recording or image,
503 unavailable backend/cache/service.

## Web parity and remaining coverage

| Existing Web behavior | Public-v1 Recording Artwork |
| --- | --- |
| Artwork bound to cached Recording metadata | Same cached artwork refs and opaque ID calculation |
| Local allowed-root/signature/size checks | Same `VdrRecordingArtworkService` |
| Poster, fanart, still, banner assets | Listed and binary delivered where present |
| Optional Web home-preview variant | Deliberately not exposed yet; original bytes only |
| EPG cached direct artwork | `GET /api/v1/epg/artwork?backendId=...&channelId=...&eventId=...` uses the existing EPG image owner with an exact `epg.view` grant |
| TVScraper series/EPG fallback metadata images | Still on the existing authenticated metadata image route; not yet projected to Public v1 |

Kodi and Plex support artwork-based media presentation; their exact
current provider/fallback semantics were not verified in this slice.
The already working VDR-Suite Web remains the functional baseline.
A successful source CI is not evidence of real yaVDR, Kodi,
Android TV or Hisense VIDAA hardware acceptance. M2 remains incomplete
until existing series/EPG/fallback owners are assessed and integrated
as appropriate; do not introduce a new scraper or cache merely for parity.

See `api/openapi.json` for HTTP schemas. This work is on the
post-Phase-69 API v1 roadmap; Phase 70 is not started.
