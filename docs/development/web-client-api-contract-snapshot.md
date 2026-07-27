# Web Client API Contract Snapshot

## Status

Phase 59.09f established and guarded the base Web Client API ownership seam before UI module extraction. This is the current implementation snapshot of that same owner, refreshed after Phase 61, PR #110 and PR #111; it is not a design wishlist.

Browser HTTP ownership is split across DOM-free Client API files:

- `web/frontend/api/client-api.js` — base API and common domain wrappers;
- `web/frontend/api/genre-client-api.js` — Genre browse wrappers;
- `web/frontend/api/live-remote-client-api.js` — RemoteAction, LiveOverlay and live-update wrappers.

Each extension immutably augments `window.VdrSuiteClientApi`. Feature modules and `app.js` must not introduce direct backend `fetch()` calls.

## Ownership rules

- route literals belong in Client API files, not feature modules;
- Client API files remain DOM-free;
- feature modules own rendering/navigation only;
- private RESTfulAPI, SVDRP, TVScraper and SuiteBridge details never enter browser contracts;
- new wrappers require a real verified backend route and contract tests;
- Recordings 2 and the existing EPG detail owner remain the single detail destinations.

## Base Client API functions

### Timers

- `fetchClientTimers`
- `fetchClientTimerConflicts`
- `fetchClientTimerCreateAction`
- `fetchClientTimerUpdateAction`
- `fetchClientTimerDeleteAction`

### Channels and runtime state

- `fetchClientChannels`
- `fetchClientChannelMoveAction`
- `fetchClientCapabilities`
- `fetchClientVdrOverview`
- `fetchClientVdrStatus`
- `fetchClientVdrHealth`
- `fetchClientVdrSnapshotSummary`
- `fetchClientVdrSnapshots`

### Backends

- `fetchClientBackends`
- `fetchClientDefaultBackend`
- `fetchClientBackendSnapshot`

### EPG and search

- `fetchClientEpgWindow`
- `fetchClientEpgSearch`
- `fetchClientGlobalSearch`
- `fetchClientEpgCacheStatus`
- `fetchClientEpgCacheWindow`
- `fetchClientEpgCacheRefresh`
- `fetchClientEpgNowNext`
- `fetchClientEpgTimeWindow`
- `fetchClientEpgChannelWindow`

`fetchClientGlobalSearch` calls the canonical `/api/search` route and maps the selected `backendId` to the route's `backend` query parameter. It accepts abort signals used by the search module's stale-response protection and mobile timeout.

### Metadata and people

- `fetchClientMetadata`
- `fetchClientPersons`
- `fetchClientRecordingPersons`

Persisted EPG people are searched through `fetchClientGlobalSearch`; a second provider-calling browser route is not introduced.

### Recordings

- `fetchClientRecordings`
- `fetchClientRecordingCacheStatus`
- `fetchClientRecordingFolder`
- `fetchClientRecordingActionValidation`
- `fetchClientRecordingActionExecution`

### SearchTimer

- `fetchClientSearchTimers`
- `fetchClientSearchTimerDiscovery`
- `fetchClientSearchTimerPreview`
- `fetchClientSearchTimerPlan`
- `fetchClientSearchTimerValidate`
- `fetchClientSearchTimerExecute`
- `fetchClientSearchTimerRealTest`
- `fetchClientSearchTimerCreateAction`
- `fetchClientSearchTimerUpdateAction`
- `fetchClientSearchTimerDeleteAction`

## Genre Client API extension

`genre-client-api.js` owns all `/api/metadata/genres...` route literals and exposes the wrappers consumed by the `genres` module for:

- Recording/EPG overview counts;
- paged Recording Genre results;
- EPG main-class and Film-subgenre results;
- backend/time-window/filter parameters.

The `genres` module contains no direct `fetch()` and no provider dependency.

## Live Remote Client API extension

`live-remote-client-api.js` exposes:

- `fetchClientRemoteAction`
- `fetchClientLiveOverlay`
- `createClientLiveUpdateSource`

Remote actions POST to the Suite-owned `/api/vdr/remote/actions` route. Overlay reads use `/api/vdr/live/overlay`; live updates use the Suite SSE route. The remote module does not learn the private backend protocol.

PR #110's in-flight guard is UI dispatch state, not a reason to globally disable other button elements.

## Direct-fetch inventory

Known direct API fetch inventory in `web/frontend/app.js`:

```text
none
```

Architecture guards must fail if `app.js` or extracted feature modules bypass the Client API boundary.

## Known future route areas

Do not add speculative wrappers before matching backend contracts exist and are verified. Major future route areas include:

- production actor/session/permission/accountability APIs (Phase 62);
- Agent lifecycle and command APIs (Phase 63);
- TimerIntent/orchestration APIs (Phase 64);
- Recording marks/resume/cut/playback and media sessions (Phase 65);
- legacy OSD frames/viewer/controller sessions (Phase 66);
- stable `/api/v1`, ETags and common error contracts (Phase 67).

## Maintenance rule

Update this snapshot whenever a verified `fetchClient*` wrapper or Client API extension is added, removed or changes canonical route ownership. Do not use it to predeclare planned functions.