# Web Client API Contract Snapshot

## Navigation

- [Development Index](index.md)
- [Client API and Frontend Module Boundary Plan](client-api-frontend-module-boundary-plan.md)
- [Current Project Status](current-status.md)
- [Parity Audit and Frontend Gap Roadmap](../planning/parity-audit-and-frontend-gap-roadmap.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)
- [Live Remote, Overlay and Legacy OSD Contract](../architecture/live-remote-osd-contract.md)

---

## Status

Phase 59.09f documents and guards the base Web Client API seam before UI module extraction.

This document remains the current contract snapshot, refreshed after completed Phase 61, PR #110 and PR #111. It is intentionally a snapshot, not a design wishlist. Every exported base helper listed here must exist in `web/frontend/api/client-api.js` and must be exported through `window.VdrSuiteClientApi`; documented extension helpers must exist in their DOM-free Client API extension.

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
- `fetchClientEpgCacheWindow`
- `fetchClientEpgCacheRefresh`
- `fetchClientEpgNowNext`
- `fetchClientEpgTimeWindow`
- `fetchClientEpgChannelWindow`

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
- `fetchClientRecordingActionValidation`
- `fetchClientRecordingActionExecution`

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

Remote actions use the Suite-owned `/api/vdr/remote/actions` route; overlay reads use `/api/vdr/live/overlay`; live updates use the Suite SSE route. PR #110's in-flight guard remains frontend dispatch state and does not move transport or authorization into the module.

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

The following helpers must not be added until matching backend routes exist and are verified:

- `fetchClientPermissionReport`
- `fetchClientEventDetail`
- `fetchClientEventArtwork`
- `fetchClientEventMedia`
- `fetchClientRecordingMarks`
- `fetchClientRecordingResume`
- `fetchClientRecordingCut`
- `fetchClientRecordingPlayback`

Known missing route areas:

- production actor/session/permission/accountability APIs (Phase 62)
- Agent lifecycle and command APIs (Phase 63)
- TimerIntent/orchestration APIs (Phase 64)
- event media and Recording playback/session routes (Phase 65)
- Recording marks, resume and cut operations
- legacy OSD viewer/controller/session routes (Phase 66)
- stable `/api/v1`, ETags and common error contracts (Phase 67)

---

## Next Use

This snapshot remains the handoff point for frontend work:

- extract or extend UI modules without moving HTTP ownership back into `app.js`
- keep all Client API files DOM-free
- keep new backend route wrappers explicit and guarded
- preserve Recordings 2 and the existing EPG detail owner as single destination owners
- update this snapshot whenever a real new `fetchClient*` wrapper or Client API extension is added

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)