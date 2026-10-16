# Public-v1 SearchTimer collection (Device read-only)

The current implemented version is **Public API v1**, not v2.

`GET /api/v1/search-timers?backend=<backendId>&limit=30[&cursor=<opaque>]`

- This is **VDR SearchTimer**, distinct from native VDR Timer and Suite TimerAssignment.
- Requires an authenticated, verified **Device credential** and an **independently granted** `searchtimers.view` permission for exactly one backend. `timers.view`, `epg.view`, and `channels.view` do not grant access. Browser sessions are not a substitute.
- Administrators may grant/revoke `searchtimers.view` through the real Device Grant administration contract. No SearchTimer mutation grant or mutation route is offered.
- `backend` is mandatory, length 1–128, characters A-Z, a-z, 0-9, dot, underscore, hyphen. Unknown/duplicate query fields and invalid/mismatched cursors yield 400.
- Pages are limited to 1–30 items (default 30), ordered by the **server-owned stable opaque** `searchTimerId`. `st1_` cursors bind the backend. Only a bounded page is returned to the TV. The provider itself currently exposes `/searchtimers.json` as a catalogue: each page request performs one read on only that backend; **no backend fanout or retries**.
- `items` contain only `searchTimerId`, `backendId`, `name`, `query` and `state` (`active`, `inactive`, or `unknown`). No native IDs, provider payload, recording directory, or settings used for editing are exposed.
- IDs are persisted under the new `public_search_timer_identity` table; the native mapping is internal and does not make these TimerAssignments.
- A genuinely empty provider list is HTTP 200 with `items: []`. An unavailable provider, unavailable runtime, or identity failure is HTTP 503, not an empty successful response. Missing/invalid Device credentials yield 401 or 403, unauthorized backend 403; all use the Public-v1 Problem model.
- No lossless readback is guaranteed for full epgsearch option sets. **No edit, activation/deactivation, CREATE, DELETE or preview is enabled by this contract.**
- Public response is `no-store`. On permission revocation a client must clear stale SearchTimer cards, not keep displaying a successful prior list.

See `api/openapi.json`, `docs/planning/timer-searchtimer-parity.md` (VIDAA repository), and `docs/client-integration-playbook.md` (VIDAA repository).

This is an API/CI slice. Real Hisense hardware and production yaVDR deployment are separate gates.

## Preview for an existing saved rule (read-only)

`GET /api/v1/search-timers/{searchTimerId}/preview?backend=<backendId>[&limit=30]`

- Paired verified Device only. Requires **both** `searchtimers.view` and
  `epg.view` for precisely the requested backend; no browser cookie fallback.
- Uses the Webfrontend's existing `SearchTimerPreviewService` and *ready*
  backend-scoped SearchTimer EPG cache. No second search algorithm.
- Returns `searchTimerId`, `backendId`, `totalCount`, up to 30 EPG
  `items` (title, subtitle, channelId, startTime, endTime) and
  `meta: {partial:false, epgInput:"ready"}`. Zero matches is 200 with [].
  Internal native SearchTimer IDs and provider payloads remain private.
- Missing/stale opaque rule yields 404; unavailable provider, offline backend
  or not-yet-ready EPG cache yields 503, never a fake empty preview.
- The `backend` parameter must match the Device backend authorization;
  `limit` is 1..30. Unknown, repeated, cursor and offset parameters fail.
  Response is no-store.
- This previews **existing** rules only, not a new-rule form. Neither
  SearchTimer CREATE/DELETE, full blacklist/duplicate evaluation, lossless edit,
  nor native VDR Timer mutation is claimed as available on Public-v1.
