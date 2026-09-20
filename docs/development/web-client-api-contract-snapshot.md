# Web Client API Contract Snapshot

## Navigation

- [Development Index](index.md)
- [Client API and Frontend Module Boundary Plan](client-api-frontend-module-boundary-plan.md)
- [Current Project Status](current-status.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Parity Audit and Frontend Gap Roadmap](../planning/parity-audit-and-frontend-gap-roadmap.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)
- [Live Remote, Overlay and Legacy OSD Contract](../architecture/live-remote-osd-contract.md)

---

## Status

Phase 59.09f introduced and guards the base Web Client API seam before UI module extraction.

This document remains the current contract snapshot after completed Phases 61-66 and the merged post-Phase-66 Home hardening/rebuild. It is intentionally a snapshot, not a design wishlist. Every exported base helper listed here must exist in `web/frontend/api/client-api.js` and must be exported through `window.VdrSuiteClientApi`; documented extension helpers must exist in their DOM-free Client API extension.

No direct `fetch()` calls in `web/frontend/app.js`.

---

## Contract Owner

Runtime API ownership:

- `web/frontend/api/client-api.js` owns base HTTP access from the web frontend.
- `web/frontend/api/genre-client-api.js` owns Genre route access.
- `web/frontend/api/live-remote-client-api.js` owns RemoteAction, LiveOverlay and live-update access.
- all Client API extensions immutably augment `window.VdrSuiteClientApi`.
- `web/frontend/app.js` owns UI orchestration and must use `window.VdrSuiteClientApi`.
- feature modules must not introduce direct backend fetches.
- private RESTfulAPI, SVDRP, TVScraper and SuiteBridge details must not enter browser contracts.

---

## Exported Client API Functions

Timer:

- `fetchClientTimers`
- `fetchClientTimerConflicts`
- `fetchClientTimerCreateAction`
- `fetchClientTimerUpdateAction`
- `fetchClientTimerDeleteAction`

Channels:

- `fetchClientChannels`
- `fetchClientChannelMoveAction`

Broadcast Companion / Teletext, HbbTV discovery and application sessions:

- `fetchClientTeletextService`
- `fetchClientTeletextPage`
- `fetchClientHbbtvApplications`
- `fetchClientHbbtvSessionLaunch`
- `fetchClientHbbtvSessionStatus`
- `fetchClientHbbtvSessionInput`
- `fetchClientHbbtvSessionClose`
- `fetchClientHbbtvMedia`
- `fetchClientHbbtvPresentation`
- the wrappers use Suite-owned authorized Broadcast Companion read and
  application-session surfaces;
- the browser receives normalized service/page/application/session descriptors,
  normalized HbbTV media state/geometry and a session-owned QOI presentation
  frame for the Live-TV overlay;
- HbbTV input is expressed only as normalized semantic actions within the
  authorized application session;
- HbbTV browser contracts do not expose provider entry-point URLs, raw AIT
  transport data, SuiteBridge commands, browser commands, JavaScript execution
  or raw remote-key codes.

Capabilities and runtime state:

- `fetchClientCapabilities`
- `fetchClientVdrOverview`
- `fetchClientVdrStatus`
- `fetchClientVdrHealth`
- `fetchClientVdrSnapshotSummary`
- `fetchClientVdrSnapshots`

Backend selection:

- `fetchClientBackends`
- `fetchClientDefaultBackend`
- `fetchClientBackendSnapshot`

EPG:

- `fetchClientEpgWindow`
- `fetchClientEpgSearch`
- `fetchClientEpgCacheStatus`
- `fetchClientEpgCacheNowNext`
- `fetchClientEpgCacheNowNextArtwork`
- `fetchClientEpgCacheWindow`
- `fetchClientEpgCacheRefresh`
- `fetchClientEpgNowNext`
- `fetchClientEpgTimeWindow`
- `fetchClientEpgChannelWindow`

Home H2/H2.1 EPG contract:

- `fetchClientEpgCacheNowNext` owns the compact Home critical-path request and remains artwork-free.
- `fetchClientEpgCacheNowNextArtwork` owns the optional bounded artwork manifest for the same channel page and `fromTime`.
- Home artwork enrichment starts only after the compact Now/Next page has rendered.
- the Home frontend must not perform per-event Metadata or Artwork requests.

Metadata and persons:

- `fetchClientMetadata`
- `fetchClientPersons`
- `fetchClientRecordingPersons`

Global search:

- `fetchClientGlobalSearch`

Recordings:

- `fetchClientRecordings`
- `fetchClientRecordingCacheStatus`
- `fetchClientRecordingFolder`
- `fetchClientRecordingTrailer`
- `fetchClientRecordingActionValidation`
- `fetchClientRecordingActionExecution`

Recording trailer contract:

- `fetchClientRecordingTrailer` owns the authenticated Suite route for an explicit user-requested Recording trailer lookup.
- the browser passes only a bounded media type and provider identity already present in Suite metadata;
- provider credentials and TMDB transport remain server-side;
- normal Recording/metadata rendering performs no trailer provider lookup.

SearchTimer:

- `fetchClientSearchTimers`
- `fetchClientSearchTimerDiscovery`
- `fetchClientSearchTimerPreview`
- `fetchClientSearchTimerPreviewCacheRefresh`
- `fetchClientSearchTimerPlan`
- `fetchClientSearchTimerValidate`
- `fetchClientSearchTimerExecute`
- `fetchClientSearchTimerRealTest`
- `fetchClientSearchTimerCreateAction`
- `fetchClientSearchTimerUpdateAction`
- `fetchClientSearchTimerDeleteAction`

Genre Client API extension:

- the Genre extension owns the `/api/metadata/genres...` route family used by the `genres` module;
- it exposes backend-scoped overview and paged Recording/EPG Genre queries;
- the `genres` module contains no direct `fetch()` and reuses Recordings 2 and the existing EPG detail owner.

Live Remote Client API extension:

- `fetchClientRemoteAction`
- `fetchClientLiveOverlay`
- `createClientLiveUpdateSource`

Remote actions use the Suite-owned `/api/vdr/remote/actions` route; overlay reads use `/api/vdr/live/overlay`; live updates use the Suite SSE route. The in-flight guard remains frontend dispatch state and does not move transport or authorization into the module.

Global Search contract:

- `fetchClientGlobalSearch` uses the canonical `/api/search` route;
- selected `backendId` is mapped to the route's `backend` parameter;
- abort signals support stale-response protection and the mobile timeout;
- persisted EPG people are searched through this provider-free read path rather than a second browser/provider route.

---

## Current Direct Fetch Inventory

Remaining known direct API fetch inventory in `web/frontend/app.js`:

- none

The ownership guard must fail if `web/frontend/app.js` calls `fetch()` directly.

---

## Missing Backend Route Gaps

The former missing-route list that treated Phase-62 identity, Phase-63 Agent, Phase-64 Timer orchestration and Phase-65 media routes as future work is retired: those numbered foundations are completed.

The remaining forward route/compatibility gaps follow the current strict roadmap:

- **Phase 67** — Teletext service/page/subpage runtime is implemented and merged; the current HbbTV branch owns normalized application discovery plus the candidate authorized application-session/runtime and session-owned Live-TV presentation surface, while real yaVDR deployment/acceptance remains open;
- **Phase 68** — legacy OSD viewer/controller/session contracts;
- **Phase 69** — stable `/api/v1`, ETags/preconditions and common public error/compatibility contracts;
- **Phase 70** — recommendation/content-graph contracts after an accepted runtime design.

Post-Phase-66 Recording marks/cutting and Home read-model helpers are implemented Suite-owned capabilities and are not future phase gaps. Future public shapes must not be frozen merely because a private provider or experimental internal route is reachable.

---

## Next Use

This snapshot remains the handoff point for frontend work:

- extract or extend UI modules without moving HTTP ownership back into `app.js`;
- keep all Client API files DOM-free;
- keep new backend route wrappers explicit and guarded;
- preserve Recordings 2 and the existing EPG detail/playback owners as single destination owners;
- update this snapshot whenever a real new `fetchClient*` wrapper or Client API extension is added.

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
