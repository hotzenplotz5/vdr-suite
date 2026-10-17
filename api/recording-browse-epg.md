# Public-v1 Recording browse, Genres and EPG now/next — developer contract

## Navigation

- [API developer entry](README.md)
- [OpenAPI 3.1 source-verified subset](openapi.json)
- [Recording playback API: separate access boundary](../docs/development/public-v1-device-recording-playback.md)
- [Public-v1 EPG now/next original contract](../docs/development/public-v1-epg-now-next.md)
- [REST runtime](rest/src/PublicApiRuntime.cpp)
- [Security gate](../core/security/include/SecurityHttpGate.h)

## Scope and evidence

Source baseline: `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`, 2026-10-11. Public-v1 routes, bounded parsers, serializers, security permissions and REST tests have been inspected. These are *read-only metadata* APIs and must not be confused with device-owned playback, media-track, native VDR directory or Web-private endpoints. Source presence is not proof that a given yaVDR daemon or Hisense VIDAA build has deployed these routes.

All four GET operations require an authenticated Actor and **exactly one** `backendId` query value. The security gate enforces the grant and authorized backend scope; runtime checks that the supplied backend matches the scoped set. The Recording and Genre reads require `recordings.view@backend`; EPG now/next requires **either `epg.view@backend` or `media.live.play@backend`**, never `channels.view` alone. Live-TV playback permission includes access to the EPG on that same backend. Invalid/duplicate/unknown query fields and mismatched scopes yield HTTP 400 (the security gate can return 401/403 first).

## 1. Recording collection (keyset mode)

```http
GET /api/v1/recordings?backendId=home&limit=50&sort=recordingId&order=asc
```

The default mode reads the normalized Suite recording catalog with `recordingId ASC`, `limit` 1–100 (default 50) and opaque `rc1_` backend-scoped cursor. Do **not** construct, inspect or substitute a different backend's cursor. A malformed cursor yields 400; a cursor from another backend scope yields **409 `cursor_expired`**. Response:

```json
{"items":[{"recordingId":"rec_0123456789abcdef0123456789abcdef","backendId":"home","title":"Film","recordedAt":"2026-10-09T20:15:00Z","durationSeconds":3600,"durationKnown":true,"description":"Kurzbeschreibung","sizeMb":2048}],"page":{"limit":50,"nextCursor":null,"hasMore":false},"meta":{"partial":false},"links":{"self":"/api/v1/recordings?backendId=home&limit=50&sort=recordingId&order=asc","next":null}}
```

No provider/native recording path, native identifier, playlist URL, artwork URL, ETag or media session is exposed. The `recordingId` is a durable Suite identity (`rec_` + 32 lowercase hexadecimal digits). For `durationKnown:false`, the numerical field is not proof of a measured duration.

## 2. Folder browse (same URL, distinct wire shape)

```http
GET /api/v1/recordings?backendId=home&view=folders&limit=50&offset=0
GET /api/v1/recordings?backendId=home&view=folders&folderId=fld1_0123456789abcdef0123456789abcdef&limit=50&offset=0
```

Folder mode uses **offset pagination**, not a cursor. It permits `view=folders`, optional opaque `folderId` (`fld1_` + 32 lowercase hex characters), `limit` 1–100, `offset` 0–1,000,000. It rejects `cursor`, `sort`, `order`; default keyset mode rejects `folderId` and `offset`. The shape is:

```json
{"items":[{"kind":"folder","folderId":"fld1_0123456789abcdef0123456789abcdef","name":"Serien","recordingCount":3},{"kind":"recording","recordingId":"rec_0123456789abcdef0123456789abcdef","backendId":"home","title":"Film","recordedAt":"2026-10-09T20:15:00Z","durationSeconds":3600,"durationKnown":true,"description":"Kurzbeschreibung","sizeMb":2048}],"page":{"limit":50,"offset":0,"totalCount":2,"hasMore":false},"meta":{"partial":false}}
```

**Single-recording title folders may appear as `kind=recording` leaves**, not an extra clickable folder. Do not reconstruct a filesystem path from the folder ID. For player requests use the separate device-owned Recording playback API, not this metadata endpoint.

## 3. Recording genre facets and membership

```http
GET /api/v1/genres?backendId=home&locale=de&limit=30&offset=0
GET /api/v1/genres/recordings?backendId=home&genreId=crime&locale=en&limit=30&offset=0
```

Both require `recordings.view@home`; `genreId` is **required** only on the membership endpoint and forbidden on the facet root. The genre ID is a bounded alphanumeric/dot/underscore/hyphen key, not a path. Optional `locale` = `de` or `en` (default `de`), limit 1–30 (default 30), offset 0–1,000,000. No cursor or ETag.

`GET /genres` items carry `genreId`, `label`, `labelDe`, `labelEn`, `count`. `GET /genres/recordings` items carry `recordingId`, `backendId`, `title`, `recordedAt`, `durationSeconds`, `durationKnown`. Both responses have `items`, `page:{limit,offset,totalCount,hasMore}`, `meta:{partial:false}`. The count is a facet membership count, not evidence that an item can be played with the current Device grants.

## 4. EPG now/next (cached read)

```http
GET /api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&limit=2
```

Requires **`epg.view@home` or `media.live.play@home`** for the requested `backendId`. Both `backendId`, `channelId` and `fromTime` are mandatory. `channelId` is limited to 1–256 alphanumeric/dot/underscore/hyphen characters; `fromTime` is **decimal Unix epoch seconds**, 10–12 digits without a leading zero. Limit is 1–2 (default 2); `cursor`/`offset` and all unknown fields are invalid. One `GET` uses cached service data only; it does **not** refresh EPG.

```json
{"backendId":"home","channelId":"S19.2E-1-100","items":[{"channelId":"S19.2E-1-100","title":"Nachrichten","subtitle":"Jetzt","startTime":"1791565200","endTime":"1791568800","durationSeconds":3600}],"page":{"limit":2,"hasMore":false},"meta":{"partial":false}}
```

An empty `items` array means no matching cached events, not that nothing is broadcast. Response timestamps are **strings** in seconds, not JavaScript milliseconds or localized date strings. It returns `404` for an unknown channel, `503` for an unavailable cache/backend or inconsistent provider data; no native EPG IDs or private URLs are exposed.

## Error and client interoperability notes

| HTTP | Meaning |
| --- | --- |
| `200` | Read succeeded, including a valid empty result |
| `400` | Invalid query or backend authorization scope mismatch |
| `401` / `403` | Authentication required / backend-specific permission denied |
| `404` | Unknown EPG channel (not a generic empty event list) |
| `409` | Recording keyset cursor from wrong backend scope |
| `503` | Backend/data provider/read projection or accountability unavailable |

These HTTP reads cannot create Timer assignments, start playback or select tracks. Track 404 diagnostics must compare the **installed** daemon's actual route/method and Device grant scope with the requesting client's URL; Web frontend success is not a substitute for that comparison.

## Regression evidence

- [Public recording collection and folders](rest/tests/test_public_recording_collection.cpp)
- [Genre facet/membership responses](rest/tests/test_public_recording_genre_collection.cpp)
- [EPG now/next contract](rest/tests/test_public_epg_now_next.cpp)
- [Recording read security checks](../core/security/tests/test_public_recording_collection_security.cpp)
- [EPG permission/scope security checks](../core/security/tests/test_public_epg_now_next_security.cpp)
- [Scoped OpenAPI drift guard](../tools/check_public_v1_openapi.py)

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)
