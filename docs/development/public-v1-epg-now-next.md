# Public-v1 EPG Now/Next, server candidate

Status: **Server-Quellcode implementiert und GitHub-CI SUCCESS; reale yaVDR-/VIDAA-Abnahme ausstehend.**
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
separate SecurityHttpGate permission/scope test, and successful full daemon build.
**GitHub-CI:** https://github.com/hotzenplotz5/vdr-suite/actions/runs/37967573659
**Implementation:** `e9d7e93aca36813fe34fcd9805b552f87f7752a3`.

The paired VIDAA slice exists in commit
`294ea2667786bb0ad892b4b2b3d1aa79602d98d9` (CI SUCCESS:
https://github.com/hotzenplotz5/vdr-suite-vidaa/actions/runs/37967912131).
It checks the capability, requires device authorization, reads at most two events
for one selected channel, and has D-Pad/OK/BACK navigation without old Web routes.

Hardware acceptance pending: genuine event freshness, time format,
backend grant propagation, channel identity, timezone, D-Pad and
TV memory/latency under load. No production deployment as part of
this slice.
