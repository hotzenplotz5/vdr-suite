# Web Client API Contract Snapshot

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)

## Status

This document began as the Phase-59 Web Client API seam contract and remains a living snapshot of frontend HTTP ownership. The later platform phases and post-Phase-66 work extended that seam; old phase-numbered gap notes are no longer current authority.

No direct `fetch()` calls belong in `web/frontend/app.js`.

## Contract owner

- `web/frontend/api/client-api.js` owns base HTTP access from the web frontend.
- feature Client API extensions own their route families and immutably augment `window.VdrSuiteClientApi`.
- `web/frontend/app.js` owns UI orchestration and consumes `window.VdrSuiteClientApi`.
- feature modules must not introduce direct private backend/provider fetches.
- RESTfulAPI, SVDRP, TVScraper, Streamdev and SuiteBridge details remain private implementation details.

## Established route families

The Client API now covers the established product domains needed by the shipped first-party UI, including:

- Timers and conflict/action flows;
- Channels, backend selection and runtime status;
- EPG window/search/cache/Now-Next routes;
- Recording cache/folder/query/action and metadata routes;
- metadata/person/search/Genre flows;
- SearchTimer flows;
- Live remote/overlay/update flows;
- Phase-65 MediaSession/playback routes used by the canonical playback owners;
- post-Phase-66 Recording marks/cut flows;
- Home-specific compact EPG read-model helpers without creating a second Home HTTP owner.

## Home H2/H2.1 EPG contract

- the compact Home Now/Next request remains artwork-free on the critical path;
- optional bounded artwork enrichment follows after the compact projection is usable;
- Home must not perform per-event Metadata/Artwork fan-out;
- Home Recording-folder cards use embedded persisted native metadata when available rather than adding per-Recording metadata HTTP reads.

## Ownership examples

Timer helpers remain explicit `fetchClient*` functions. Channels/EPG/Recordings/SearchTimer/Genre/Live Remote use their Client API owners. Global Search uses the canonical `/api/search` route and persisted provider-free read models.

The ownership guard must fail if `web/frontend/app.js` calls `fetch()` directly.

## Current remaining platform gaps

The old gap list that assigned already-completed work to Phases 62-65 is retired. Those foundations now exist.

Remaining future route/compatibility areas are intentionally tied to the current roadmap:

- Phase 67: Teletext service/page/subpage and HbbTV broadcast-application discovery/session contracts;
- Phase 68: Legacy OSD viewer/controller/session contracts;
- Phase 69: stable public `/api/v1`, compatibility/deprecation semantics, ETags/preconditions and common public error contracts;
- Phase 70: recommendation/content-graph contracts after an accepted runtime design.

Do not pre-freeze those future public shapes in the internal first-party Client API merely because a provider or experimental route is reachable.

## Next use

- extend modules without moving HTTP ownership back into `app.js`;
- keep Client API files DOM-free;
- add route wrappers only for implemented Suite-owned backend contracts;
- preserve existing Recording/EPG/detail/playback destination owners;
- update this snapshot when a real new `fetchClient*` wrapper or Client API extension is added.

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
