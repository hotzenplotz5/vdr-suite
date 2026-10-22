# Public API v1 — E1 cached EPG window continuation

Status: source implementation under review in [PR #519](https://github.com/hotzenplotz5/vdr-suite/pull/519). **Do not treat a source PR/CI success as a productive yaVDR installation or Hisense TV acceptance.**

## Endpoint and parameters

`GET /api/v1/epg/window` uses the **existing** backend-scoped EPG cache, also read by the legacy Web `/api/epg/cache/window` endpoint. The Public-v1 SecurityHttpGate requires an authenticated actor and an exact backend with `epg.view` **or** `media.live.play`; no provider fallback, synthesized programmes, timer mutation or extra cache is involved.

Required query parameters: `backendId` (one permitted backend), `channelId` (one VDR channel), `fromTime` (Unix seconds), `untilTime` (exclusive Unix seconds strictly greater than `fromTime`). Optional `limit` is 1–192, default 192. Optional `cursor` is the verbatim opaque `page.nextCursor` returned by a previous call. The new implementation does not constrain `untilTime - fromTime` to twelve hours: it streams pages from **only the entries actually in cache**.

Illustrative request (the identifiers and times are examples, not verified live data):

```http
GET /api/v1/epg/window?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&untilTime=1791824400&limit=2
```

A page has the established `backendId`, `channelId`, `items`, `page.limit`, `page.hasMore` and `meta.partial=false` fields; E1 appends `page.nextCursor`. Each item contains channel, title, optional subtitle, start/end times and duration. It contains `eventId` **only when the existing repository proves source-ID provenance**. The private cache key is **not** promoted to a native `eventId`.

Illustrative response fragment (cursor shortened for explanation; use the real complete value unchanged):

```json
{
  "backendId": "home",
  "channelId": "S19.2E-1-100",
  "items": [
    {"channelId": "S19.2E-1-100", "title": "Example 1",
     "subtitle": "", "startTime": "1791565200",
     "endTime": "1791568800", "durationSeconds": 3600,
     "eventId": "81234"},
    {"channelId": "S19.2E-1-100", "title": "Example 2",
     "subtitle": "", "startTime": "1791568800",
     "endTime": "1791572400", "durationSeconds": 3600}
  ],
  "page": {"limit": 2, "hasMore": true, "nextCursor": "ew1_<opaque>"},
  "meta": {"partial": false}
}
```

Continue with the *same* backend, channel, `fromTime`, `untilTime` and a bound `limit`, appending the entire real `&cursor=...` value. Stop only when `hasMore=false`; the terminal `nextCursor` is `null`. Pages can be empty only when no matching cache entries exist in the queried range.

## Scope and failure behavior

- The cursor binds the originating backend, channel and exact time interval. Invalid, duplicate, tampered, mismatched-scope or stale (deleted/moved anchor) cursors return HTTP 400 after the normal auth/scope gates; the client must restart the query from page one.
- Unknown channel is HTTP 404; backend or EPG cache unavailable is HTTP 503; an authenticated but incorrectly scoped read is rejected by the existing Public-v1 gate/runtime. Normal granted/denied/unauthenticated behavior is unchanged.
- Results are sorted by the existing cache start-time and event-key order (Public-v1 is single channel). Equal start times are disambiguated by cache identity regardless of whether a row carries a genuine VDR event ID.
- The continuation is deterministic for an unchanged cache. The cache can be refreshed between requests: a removed cursor anchor is rejected, but insertions or updates after earlier pages do **not** constitute an atomic multi-request snapshot. Restart the traversal when the EPG changes if snapshot-level consistency is required.
- Event-Detail ETag/If-None-Match and Now/Next contracts are unchanged. This window endpoint does not introduce an ETag or grant write authority. Existing 12-hour callers remain valid without cursors; their previous `hasMore` meaning is unchanged but E1 now supplies a continuation.
- Historical Web EPG and Public-v1 can differ in their *response projection* (legacy enriched artwork vs. Public-v1 minimal title/times/source-ID fields). They continue to read one cache and one sorting/index source; a repository regression compares legacy unpaged cache results with assembled E1 pages over realistic overlapping/same-start and multi-day fixtures.

## Product comparison (scope of E1 only)

The client need is the same as a multi-day TV guide in Kodi/PVR or Plex Live TV: navigable programme horizons without a gigantic response. Those clients' **specific current pagination APIs and cache behavior are not verified here**. E1 exposes a safe backend-owned retrieval primitive; it does not deliver TV-guide UI, new listings, missing programme data or a replacement metadata provider.

## Validation boundary

Focused source checks: `test-public-epg-now-next-api`, `test-epg-event-repository`, existing SecurityHttpGate tests and the exact-head CI/architecture/OpenAPI guards. No live yaVDR daemon installation, frontend/VIDAA deployment, destructive test recording or Timer modification is authorized by this E1 PR.
