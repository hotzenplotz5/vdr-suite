# Public-v1 EPG Now/Next, server candidate

Status: **Source code only, pending CI and real yaVDR/VIDAA acceptance.**
Web references inspected first:
`web/frontend/modules/channels.js`, `web/frontend/home-now-next.js`,
`api/rest/src/EpgCacheController.cpp` and
`core/vdr/include/EpgCacheService.h`. The existing cached EPG service
is reused, **not** the old `/api/epg/cache/now-next` web route.

## Read-only wire contract

`GET /api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&limit=2`

- Required `backendId`, `channelId` and `fromTime` (Unix seconds,
  decimal 10–12 digits); optional `limit=1|2`, default 2.
- Single authorized backend; gate authorizes `epg.view@backend`,
  not merely `channels.view`. Scope is rechecked in the runtime.
- Backend must be online/enabled with an existing VDR snapshot and
  the requested channel must occur in that backend snapshot.
- The existing `EpgCacheServiceRegistry` and
  `findNowNextPerChannelForBackend` return **up to two** cached events.
  No implicit live refresh or out-of-band network work.
- Response: `backendId`, `channelId`, `items` (each:
  `channelId`, `title`, `subtitle`, `startTime`, `endTime`,
  `durationSeconds`), `page:{limit,hasMore:false}`,
  `meta:{partial:false}`. No internal URLs, no native EPG IDs,
  no automatic artwork requests. Empty array means no cache events,
  **not** a synthetic schedule.
- Strict 400 on malformed, duplicate, unrecognized query keys,
  excessive limit or scope mismatch; 401 unauthenticated,
  403 unauthorized via security gate; 404 unknown channel, 503
  cache/backend unavailable or invalid provider projection.
  Standard Public-v1 `Cache-Control:no-store` responses.
- Capability `public-api.epg-now-next-read`, version 1, dynamic
  availability via registered read callback.

Automated test coverage: standalone PublicApiRuntime unit executable,
separate SecurityHttpGate permission/scope test, branch CI compiles
full daemon. Follow-on VIDAA slice will require capability and
`epg.view` scope, request only for a selected channel via the
Device credential, preserve a loading/offline/permission/empty state,
and never call unversioned Web routes.

Hardware acceptance pending: genuine event freshness, time format,
backend grant propagation, channel identity, timezone, D-Pad and
TV memory/latency under load. No production deployment as part of
this slice.
