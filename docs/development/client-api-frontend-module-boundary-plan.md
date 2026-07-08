# Client API and Frontend Module Boundary Plan

## Navigation

- Development Index: index.md
- Current Project Status: current-status.md
- Frontend Architecture: frontend-architecture.md
- Phase Map: ../planning/phase-map.md

---

## Purpose

This document defines the next architecture boundary before
extracting more code from web/frontend/app.js.

The goal is not to reuse the current browser UI on every
platform.

The goal is to define a stable VDR-Suite Client API.

That API should be usable by:

- Web frontend
- Windows client
- Linux client
- Android and iOS clients
- TV frontend

Each client may have its own user interface.

---

## Core Decision

VDR-Suite should expose a stable client-facing API.

Clients should not directly depend on:

- VDR internals
- VDR restfulapi response quirks
- epgsearch command details
- SVDRP transport details
- plugin-specific JSON shapes

Architecture direction:

- VDR, restfulapi, epgsearch and tvscraper are sources.
- VDR-Suite Backend normalizes those sources.
- VDR-Suite Client API exposes stable HTTP and JSON.
- Web frontend is one client of that API.
- Windows client is another client.
- Linux client is another client.
- Mobile clients are other clients.
- TV frontend is another client.

The web frontend is therefore not the shared architecture.

It is one implementation on top of the shared Client API.

---

## Non-Goal

Do not try to reuse browser DOM code directly in Windows,
Linux or TV clients.

The following must remain platform-specific:

- DOM rendering
- focus handling
- keyboard navigation
- remote-control navigation
- mobile layout behavior
- TV safe-area design
- large-card TV design
- Windows and Linux native widgets
- animations
- interaction details

The reusable part is the API contract.

View-model semantics may also be shared where useful.

---

## Layer Model

### 1. Server API Contract

The server API is the stable HTTP and JSON contract exposed
by VDR-Suite.

It hides:

- VDR plugin differences
- restfulapi JSON shape differences
- epgsearch command and parsing details
- SVDRP transport details
- cache and live fallback decisions
- per-backend permissions

### 2. Client API Wrapper

Each platform may have its own wrapper around the server API.

For the web frontend this should become:

- web/frontend/api/client-api.js

This file must be DOM-free.

Allowed in client-api.js:

- fetch
- URL construction
- JSON parsing
- HTTP error normalization
- backend id propagation
- read-only capability flags
- write capability flags

Not allowed in client-api.js:

- document.createElement
- CSS class decisions
- timer-card rendering
- channel-logo DOM rendering
- module tab manipulation

### 3. UI Modules

UI modules consume the Client API wrapper.

UI modules render platform-specific views.

Target web frontend shape:

- web/frontend/index.html
  - static shell and script order
- web/frontend/style.css
  - shared and module-specific CSS
- web/frontend/app.js
  - bootstrap and module router during migration
- web/frontend/api/client-api.js
  - DOM-free Client API wrapper
- web/frontend/modules/timers.js
  - Timer UI module
- web/frontend/modules/epg.js
  - EPG UI module
- web/frontend/modules/recordings.js
  - Recording UI module
- web/frontend/modules/searchtimers.js
  - SearchTimer UI module
- web/frontend/modules/channels.js
  - Channel UI module
- web/frontend/helpers/dom.js
  - DOM helper functions
- web/frontend/helpers/format.js
  - formatting helper functions
- web/frontend/helpers/channel-logos.js
  - channel logo helper functions

Extraction must be incremental.

The current app.js remains the bootstrap and module router
until modules are migrated.

### Channel Browser Dependency Inventory

Phase 59.11d records the current Channel browser dependency boundary before
additional extraction work.

`web/frontend/channel-browser.js` is already separated as its own frontend
asset, but it is not yet an isolated module. It still depends on shared
runtime state and helper functions owned by `web/frontend/app.js`.

Current app.js-owned helper dependencies:

- `frontendChannelId`
- `frontendEventChannelId`
- `parseFrontendEventEpoch`
- `frontendEventEnd`
- `formatEpgClockFromEpoch`
- `formatEpgDuration`
- `epgEventTitle`
- `epgEventSubtitle`
- `epgChannelTitle`
- `createEpgEventDetailCard`
- `fetchCachedOrLiveEpgWindow`
- `fetchCachedEpgWindowForVisibleChannel`
- `selectedEpgBackendId`
- `epgWindowBounds`
- `renderChannelList`

Current app.js-owned runtime state dependencies:

- `currentEvents`
- `selectedModule`
- `detailDataElement`

Current non-app frontend dependency:

- `createChannelLogoElement` from the channel logo frontend helper.

Current Client API dependency:

- `window.VdrSuiteClientApi.fetchClientEpgCacheRefresh`

Boundary rule for upcoming slices:

- Do not move the Channel browser blindly.
- First replace app.js-owned data and DOM dependencies with an explicit
  `configureContext(...)` style boundary similar to the Recording browser.
- Keep HTTP ownership in `web/frontend/api/client-api.js`.
- Keep `channel-browser.js` free of direct `fetch()` calls and direct
  `/api/epg/cache/refresh` literals.

Current Channel browser extraction boundary:

- Phase 59.11e introduced `window.VdrSuiteChannelBrowser.configureContext(...)`.
- Phase 59.11f wired `app.js` to configure the Channel browser context with
  the current detail mount target.
- Phase 59.11g routed Channel browser mount-target access through
  `channelBrowserDetailDataElement()`.
- Phase 59.11h guards the context boundary and prevents direct
  `detailDataElement.replaceChildren(...)` or `detailDataElement.appendChild(...)`
  regressions inside `channel-browser.js`.

Channel browser modularization readiness:

- Phase 59.11j moved `addText` behavior into a local Channel browser DOM text helper.
- Phase 59.11k guards the local DOM text helper so global `addText(...)` does not return.
- Phase 59.11l moved `firstValue` behavior into a local Channel browser response helper.
- Phase 59.11m guards the local first-value helper so global `firstValue(...)` does not return.
- Phase 59.11n moved `listFromResponse` behavior into a local Channel browser response helper.
- Phase 59.11o moved `listEventsFromEpgResponse` behavior into a local Channel browser response helper.
- Phase 59.11p guards local Channel browser response helpers so global list helpers do not return.
- Phase 59.11q marks the Channel browser ready for the first explicit Module API slice.

Current Channel browser module state:

- Phase 59.12a introduced `window.VdrSuiteChannelBrowser.renderList(data)`.
- Phase 59.12a kept the legacy global `renderChannelList(data)` only as a compatibility bridge.
- Phase 59.12b routes `app.js` through `window.VdrSuiteChannelBrowser.renderList(data)` via `renderChannelsThroughModule(data)`.
- Phase 59.12c guards the temporary legacy global `renderChannelList` bridge and prevents app.js from using it directly.
- Phase 59.12d removes the temporary global `renderChannelList(data)` bridge from `channel-browser.js`.
- Phase 59.12e documents the module-path constraint before moving the physical asset.

Frontend module asset contract:

- Channel source-of-truth: `web/frontend/modules/channels.js`.
- Channel runtime-compatible script path: `/frontend/channel-browser.js`.
- Recording source-of-truth: `web/frontend/modules/recordings.js`.
- Recording runtime-compatible script path: `/frontend/recording-browser.js`.
- `mk/install.mk` installs both module source files and their runtime-compatible copies.
- Staging install tests verify that each runtime-compatible copy is byte-identical to its module source.
- `index.html` keeps loading the runtime-compatible script paths until daemon/static serving rollout is safe.
- `check_frontend_ownership_contracts.py` prevents direct `/frontend/modules/channels.js` and `/frontend/modules/recordings.js` script loading before that rollout.
- `/frontend/modules/channels.js` and `/frontend/modules/recordings.js` remain prepared for later direct module-path loading.
- Keep runtime HTTP access routed through `window.VdrSuiteClientApi`.

Phase 59.15 closeout:

- Channel and Recording browser source files now live under `web/frontend/modules/`.
- Runtime-compatible script paths remain authoritative for the deployed daemon:
  - `/frontend/channel-browser.js`
  - `/frontend/recording-browser.js`
- Install rules keep runtime-compatible copies byte-identical to their module sources.
- Ownership guards protect script order, runtime paths, module sources, install copies and smoke-check documentation.
- The next architectural step is Phase 60 frontend platform work: shared bootstrap, module registry, shared UI helpers and continued Client API consolidation.

Phase 60 Frontend Platform roadmap anchor:

- Phase 60 must not start by moving another large browser file.
- First target: a small `web/frontend/platform/` foundation with explicit runtime-safe loading.
- Phase 60.1b prepares `/frontend/platform/bootstrap.js` static serving and install contracts before creating or loading the bootstrap asset.
- Phase 60.1c creates `web/frontend/platform/bootstrap.js` as a DOM-free namespace anchor.
- Phase 60.1d loads `/frontend/platform/bootstrap.js` before `/frontend/api/client-api.js` and guards the load order.
- Phase 60.1e extends the runtime smoke check with `window.VdrSuitePlatform.isLoaded()`.
- Phase 60.1f closes the minimal bootstrap contract: `/frontend/platform/bootstrap.js` is served, installed, loaded first, DOM-free and smoke-checked.
- Planned platform pieces:
  - frontend bootstrap boundary
  - module registry
  - shared DOM/UI helper namespace
  - shared formatting helper namespace
  - continued `window.VdrSuiteClientApi` consolidation

Phase 60.2 module registry roadmap:

- Do not connect existing Channel or Recording browser modules to the registry yet.
- Phase 60.2b extends `web/frontend/platform/bootstrap.js` with a private DOM-free module registry.
- Planned registry API:
  - `registerModule(name, moduleApi)`
  - `getModule(name)`
  - `hasModule(name)`
  - `listModules()`
- Registry names must be stable strings such as `channels` and `recordings`.
- Registry storage must remain private inside `platform/bootstrap.js`.
- Registry methods must avoid DOM access, HTTP access and dynamic script loading.
- Phase 60.2b provides the registry API but does not register Channel or Recording browser modules yet.
- Phase 60.2c guards the smoke-check expectation that the registry starts empty before module registration slices.
- Existing globals remain authoritative until dedicated migration slices:
  - `window.VdrSuiteChannelBrowser`
  - `window.VdrSuiteRecordingBrowser`
- Existing browser modules remain on their runtime-compatible script paths until daemon/static rollout is deliberately changed.
- Every platform slice must preserve the Phase 59 smoke-check expectations for `Kanäle` and `Aufnahmen`.

Frontend module runtime smoke check:

- After installing frontend assets, hard-reload the browser.
- Verify `window.VdrSuitePlatform.isLoaded()` returns `true` after hard reload.
- Verify `window.VdrSuitePlatform.listModules()` returns an empty array before module registration slices.
- Verify the `Kanäle` module renders channel groups and selected-channel programmes.
- Verify the `Aufnahmen` module renders the recording tree and opens recording details.
- Verify the browser console has no missing script errors for:
  - `/frontend/channel-browser.js`
  - `/frontend/recording-browser.js`
- Verify the browser console has no accidental direct module path load errors for:
  - `/frontend/modules/channels.js`
  - `/frontend/modules/recordings.js`
- If the running daemon is not rebuilt/restarted, the runtime-compatible script paths remain authoritative.
- `check_frontend_ownership_contracts.py` guards this smoke-check section so it does not disappear during later module work.

Recording browser module-path preparation:

- The logical Recording browser module API already exists as `window.VdrSuiteRecordingBrowser`.
- The current runtime script remains `/frontend/recording-browser.js`.
- The source-of-truth file is now `web/frontend/modules/recordings.js`.
- Phase 59.14b prepares install and static serving contracts for `web/frontend/modules/recordings.js` before the physical asset move.
- Phase 59.14c applies the Channel browser compatibility-copy pattern to Recording browser: `recording-browser.js` remains the runtime path and is verified against `modules/recordings.js` when the module source exists.
- Phase 59.14d moves the physical Recording browser source to `web/frontend/modules/recordings.js`.
- Phase 59.14e aligns frontend ownership guards with `web/frontend/modules/recordings.js` as the source of truth.
- Phase 59.14f makes `modules/recordings.js` installation mandatory and verifies the runtime `/frontend/recording-browser.js` compatibility copy.

Next phase direction:

- Move formatting helpers only after the Module API bridge is stable.
- Plan a separate backend-aware static asset path slice before any physical file move into `web/frontend/modules/`.

---

## Candidate Client API Endpoints

The exact route names may evolve.

The client contract should cover these read-only endpoints:

- GET /api/client/backends
  - list available backends
- GET /api/client/dashboard
  - dashboard summary
- GET /api/client/timers
  - timer list
- GET /api/client/timer-conflicts
  - timer conflict report
- GET /api/client/epg/now-next
  - now and next EPG summary
- GET /api/client/epg/window
  - EPG time window
- GET /api/client/channels
  - channel list
- GET /api/client/recordings
  - recording list
- GET /api/client/searchtimers
  - SearchTimer list
- GET /api/client/capabilities
  - client capability report

The client contract should cover these mutation endpoints:

- POST /api/client/timers
  - create timer
- PUT /api/client/timers/{id}
  - update timer
- DELETE /api/client/timers/{id}
  - delete timer
- POST /api/client/searchtimers
  - create SearchTimer
- PUT /api/client/searchtimers/{id}
  - update SearchTimer
- DELETE /api/client/searchtimers/{id}
  - delete SearchTimer
- POST /api/client/recordings/actions/validate
  - validate recording action
- POST /api/client/recordings/actions/execute
  - execute recording action

The existing /api/vdr routes can continue to exist.

The future /api/client layer should become the stable
multi-client contract.

---

## Client ViewModel Direction

Raw backend objects are not ideal for every client.

Future client responses should provide frontend-friendly
view-model fields where useful.

Useful fields include:

- backendId
  - stable backend identifier
- backendName
  - display name
- readonly
  - whether the selected backend is read-only
- source
  - live, cache, fallback or similar source marker
- items
  - client-friendly rows or cards
- items.id
  - stable item id
- items.title
  - display title
- items.channelName
  - human-readable channel name
- items.startText
  - formatted start text where useful
- items.endText
  - formatted end text where useful
- items.status
  - client-facing status
- items.hasConflict
  - conflict marker for Timer views
- conflicts.count
  - number of conflicts
- conflicts.severity
  - none, warning, critical or similar severity

This avoids duplicating formatting and conflict interpretation
in every client.

---

## Multi-Backend and Permission Requirements

The Client API must always be ready for multiple backends.

Required fields:

- backendId
- backendName
- readonly
- capabilities
- source
- lastUpdated
- permissions

The second-site read-only requirement must be explicit.

A client should not infer read-only behavior from UI state.

Every mutation response must be backend-scoped.

Every mutation response must be permission-aware.

---

## TV Frontend Requirements

A TV frontend is not just a scaled web page.

TV-specific requirements:

- remote-control focus navigation
- predictable focus order
- large target cards
- reduced text density
- visible loading states
- visible error states
- no hover-only interactions
- no dependency on precise pointer movement
- minimal DOM complexity for weaker embedded browsers

The Client API should provide clean enough data for TV views.

A TV UI should not need backend-specific logic.

---

## Web Frontend Migration Order

Recommended next phases:

- Phase 58.98
  - Web Client API Wrapper
- Phase 58.99
  - Extract Timer Web Module
- Phase 59.00
  - Extract Channel Web Module
- Phase 59.01
  - Extract EPG Web Module
- Phase 59.02
  - Extract Recording Web Module
- Phase 59.03
  - Extract SearchTimer Web Module

Do not extract all modules at once.

The first implementation step should be:

- web/frontend/api/client-api.js

This creates the seam between HTTP access and DOM rendering.

---

## First Implementation Slice

The first code slice should be intentionally small.

Target file:

- web/frontend/api/client-api.js

Initial functions:

- fetchClientTimers
- fetchClientTimerConflicts
- fetchClientChannels
- fetchClientEpgWindow
- fetchClientRecordings
- fetchClientSearchTimers

At first these functions may wrap existing /api/vdr routes.

They do not require a new backend route immediately.

This allows web modularization before the final /api/client
route layer is introduced.

---

## Guard Rules

The existing frontend ownership guard remains valid.

Additional future guard rules should be added when
client-api.js exists.

Future guard rules:

- client-api.js must not use document.createElement
- client-api.js must not access detailDataElement
- client-api.js must not assign CSS classes
- modules/*.js may render DOM
- modules/*.js should call client-api.js for data
- helpers/*.js must stay domain-specific
- helpers/*.js must not become bootstraps

---

## Acceptance Criteria

Phase 58.97 is complete when:

- this plan exists
- it is linked from the development index
- current status references this plan
- no production code is moved yet
- make test-frontend-contracts still passes
- make test-phase-map-coverage still passes

---

## Phase 58.98 Implementation Status

Phase 58.98 introduces the first web Client API wrapper.

Implemented file:

- web/frontend/api/client-api.js

The wrapper is intentionally DOM-free.

It currently wraps existing /api/vdr routes.

No production UI module has been moved out of app.js yet.

---

## Phase 58.99 Implementation Status

Phase 58.99 moves Timer loading HTTP access behind the web
Client API wrapper.

Implemented behavior:

- loadTimers() still lives in web/frontend/app.js
- Timer rendering still lives in web/frontend/app.js
- fetchClientTimers() owns the Timer list HTTP access
- fetchClientTimers() keeps the live route and fallback route
- loadTimers() no longer fetches /api/vdr/timers directly

No Timer UI module has been extracted yet.

---

## Phase 59.00 Implementation Status

Phase 59.00 moves Timer conflict HTTP access behind the web
Client API wrapper.

Implemented behavior:

- loadTimerConflictPanel() still lives in web/frontend/app.js
- Timer conflict rendering still lives in web/frontend/app.js
- fetchClientTimerConflicts() owns the Timer conflict HTTP access
- loadTimerConflictPanel() no longer fetches the conflict route directly

No Timer UI module has been extracted yet.

---

## Phase 59.01 Implementation Status

Phase 59.01 moves Channel list HTTP access behind the web
Client API wrapper.

Implemented behavior:

- loadChannels() still lives in web/frontend/app.js
- Channel rendering still lives in web/frontend/app.js
- fetchClientChannels() owns the Channel list HTTP access
- backend selection stays based on selectedEpgBackendId()
- cache-busting stays based on the _ query parameter
- no-store and same-origin request options are preserved
- loadChannels() no longer fetches the Channel route directly

No Channel UI module has been extracted yet.

---

## Phase 59.02a Implementation Status

Phase 59.02a moves the Channel loading part of the EPG
Timeline behind the web Client API wrapper.

Implemented behavior:

- loadEpgTimeline() still lives in web/frontend/app.js
- EPG rendering still lives in web/frontend/app.js
- fetchClientChannels() owns the Channel list HTTP access
  used by loadEpgTimeline()
- fetchCachedOrLiveEpgWindow(channelData) remains unchanged
- EPG cache status loading remains unchanged
- EPG cache window loading remains unchanged
- loadEpgTimeline() no longer fetches /api/vdr/channels directly

No EPG UI module has been extracted yet.

---

## Phase 59.02b Implementation Status

Phase 59.02b moves EPG cache status HTTP access behind the
web Client API wrapper.

Implemented behavior:

- fetchEpgCacheStatusForBackend() still lives in web/frontend/app.js
- EPG cache status formatting still lives in web/frontend/app.js
- fetchClientEpgCacheStatus() owns /api/epg/cache/status access
- backend selection stays passed through the backend query parameter
- cache-busting stays based on the _ query parameter
- no-store loading is preserved
- __statusError fallback behavior is preserved
- /api/epg/cache/window remains unchanged

No EPG UI module has been extracted yet.

## Phase 59.03 Implementation Status

Phase 59.03 batches EPG cache window loading for the visible channel set.

Implemented behavior:

- /api/epg/cache/window accepts channelIds as a comma-separated query parameter
- channelIds are decoded by RestQueryParameters before routing
- EPG cache reads use SQLite channel_id IN (...) filtering for visible channels
- legacy channelId behavior remains supported
- unfiltered cache-window behavior remains supported
- the frontend sends one cache-window request for the visible channel page
- per-channel Promise.all EPG cache loading was removed
- EPG cache status remains routed through fetchClientEpgCacheStatus()
- EPG cache window loading is routed through fetchClientEpgCacheWindow()
- the EPG data path remains SQLite-based
- no EPG UI module has been extracted yet.

---

## Back

- [Back to Development Index](index.md)

---

## Phase 59.07a Web Client API Parity Refresh

Phase 59.07a promotes the Web Client API wrapper from a migration detail to an explicit parity tracking dimension.

Current implemented wrapper file:

- `web/frontend/api/client-api.js`

Current wrapper functions:

- `fetchClientTimers`
- `fetchClientTimerConflicts`
- `fetchClientTimerCreateAction`
- `fetchClientTimerUpdateAction`
- `fetchClientTimerDeleteAction`
- `fetchClientChannels`
- `fetchClientChannelMoveAction`
- `fetchClientCapabilities`
- `fetchClientVdrOverview`
- `fetchClientVdrStatus`
- `fetchClientVdrHealth`
- `fetchClientVdrSnapshotSummary`
- `fetchClientVdrSnapshots`
- `fetchClientBackends`
- `fetchClientDefaultBackend`
- `fetchClientBackendSnapshot`
- `fetchClientEpgWindow`
- `fetchClientEpgSearch`
- `fetchClientEpgCacheStatus`
- `fetchClientEpgCacheWindow`
- `fetchClientEpgNowNext`
- `fetchClientEpgTimeWindow`
- `fetchClientEpgChannelWindow`
- `fetchClientMetadata`
- `fetchClientPersons`
- `fetchClientRecordingPersons`
- `fetchClientRecordings`
- `fetchClientRecordingActionValidation`
- `fetchClientRecordingActionExecution`
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

Current status:

- The wrapper is DOM-free.
- The wrapper still calls existing `/api/vdr` and `/api/epg` routes.
- The wrapper is the required seam before extracting more UI modules from `web/frontend/app.js`.
- The final stable `/api/client` route layer remains planned, not implemented.

New parity rule:

- Every Live, RESTfulAPI or epgsearch capability must be tracked across backend/domain status, Web Client API status and UI status.
- A backend feature is not considered frontend-ready until the Web Client API status is explicit.
- New browser frontend data loading must not add direct fetch calls in `web/frontend/app.js`.
- New Web Client API wrapper functions must stay backend-neutral and DOM-free.

High-value missing wrapper areas:

- permission report route once backend exposes one
- event detail and media/artwork route gaps once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers

Next recommended frontend/API slices:

- audit event detail and media/artwork backend route gaps before adding wrappers
- add wrapper functions for recording marks, resume, cut and playback helpers
- add a permission report route once the backend exposes one
- only then expand the corresponding UI modules

---

## Phase 59.08a Implementation Status

Phase 59.08a moves SearchTimer discovery catalog HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientSearchTimerDiscovery()` is exposed by `web/frontend/api/client-api.js`
- the wrapper uses `/api/vdr/searchtimers/discovery`
- the wrapper keeps `/api/searchtimers/discovery` as a compatibility fallback
- the wrapper remains DOM-free
- no SearchTimer discovery UI is added yet

The covered catalog payload includes:

- extended EPG info
- channel groups
- blacklists
- recording directories

Next open Web Client API areas:

- permission report route once backend exposes one

---

## Phase 59.11b Implementation Status

Phase 59.11b improves the Channel browser selected-channel programme list usability.

Implemented behavior:

- `.channel-agenda-scroll` now gets the same drag-scroll helper used by the Kanalliste
- `renderAll()` attaches `enableChannelMouseDragScroll(...)` to the selected-channel programme scroll area
- the old explicit exclusion comment for programme drag-scroll is removed
- the programme scroll area exposes a title hint for dragging
- row click suppression still uses `channelDragRecentlyEnded(...)`
- the Channel browser still does not call `fetch()` directly
- `/api/epg/cache/refresh` remains owned by `web/frontend/api/client-api.js`
- no backend route is changed

Next open frontend extraction areas:

- keep Channel browser UI interactions small and smoke-tested
- reduce remaining Channel browser dependencies on app.js helpers in later slices
- keep mobile touch behavior under observation after drag-scroll changes

---

## Phase 59.11a1 Implementation Status

Phase 59.11a1 fixes the Channel browser selected-channel programme view after the EPG cache refresh Client API boundary change.

Implemented behavior:

- `channel-browser.js` owns `epgEventsForChannel(channel, sourceEvents, nowSeconds)`
- `channelEntries(channel)` can resolve current and upcoming entries again
- the helper filters events by `frontendEventChannelId(event)`
- the helper parses event start and end using the existing EPG helper functions
- stale/ended entries are ignored before rendering the selected-channel programme pane
- the Channel browser still does not call `fetch()` directly
- `/api/epg/cache/refresh` remains owned by `web/frontend/api/client-api.js`
- no backend route is changed

Next open frontend extraction areas:

- keep Channel browser helper ownership explicit
- reduce remaining Channel browser dependencies on app.js helpers in later small slices
- keep browser smoke tests focused on Kanäle, selected-channel details and EPG refresh fallback

---

## Phase 59.11a Implementation Status

Phase 59.11a starts the Channel browser HTTP boundary cleanup.

Implemented behavior:

- `fetchClientEpgCacheRefresh(options)` is added to `web/frontend/api/client-api.js`
- the wrapper owns POST access to `/api/epg/cache/refresh`
- `fetchClientEpgCacheRefresh` is exported through `window.VdrSuiteClientApi`
- the Web Client API contract snapshot lists the new wrapper
- `channel-browser.js` no longer builds the EPG cache refresh route literal
- `channel-browser.js` no longer calls `fetch()` directly for EPG cache refresh
- `channel-browser.js` requests refresh through `window.VdrSuiteClientApi.fetchClientEpgCacheRefresh(...)`
- the existing fallback behavior remains: refresh failure falls back to the existing event data
- no backend route is changed

Next open frontend extraction areas:

- reduce remaining Channel browser dependencies on app.js helpers
- keep `client-api.js` DOM-free
- keep Channel browser UI and refresh behavior stable before larger extraction

---

## Phase 59.10r Implementation Status

Phase 59.10r removes migrated Recording helper code from `app.js`.

Implemented behavior:

- `normalizePathText(value)` is removed from `app.js`
- `recordingDisplayParts(recording, index)` is removed from `app.js`
- `groupRecordings(recordings)` is removed from `app.js`
- `recording-browser.js` remains the owner of Recording path normalization
- `recording-browser.js` remains the owner of Recording display-part derivation
- `app.js` remains the HTTP/loading bridge for recordings
- `app.js` still calls `recordingBrowser.configureMountTarget(detailDataElement)`
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- consider splitting additional large frontend modules only after Recording browser remains stable
- keep cleanup slices small and verified against the real UI
- keep `recording-browser.js` UI-only

---

## Phase 59.10q Implementation Status

Phase 59.10q replaces the last Recording browser context dependency with an explicit mount target.

Implemented behavior:

- `recordingBrowserMountTarget` is now the internal DOM target holder
- `configureRecordingBrowserMountTarget(element)` validates the mount target
- invalid mount targets fail with `Recording browser mount target is invalid`
- missing mount targets fail with `Recording browser mount target is not configured`
- `app.js` now calls `recordingBrowser.configureMountTarget(detailDataElement)`
- `app.js` no longer passes a context object for Recording rendering
- `contextDependencies` is no longer exported
- `configureContext(...)` remains as a compatibility wrapper over the mount-target API
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- consider later cleanup of legacy unused Recording helper functions in `app.js`
- keep `app.js` as HTTP/loading bridge only
- keep `recording-browser.js` UI-only

---

## Phase 59.10p Implementation Status

Phase 59.10p moves Recording display-part derivation into the Recording browser.

Implemented behavior:

- `recordingBrowserNormalizePathText(value)` is now implemented locally
- `recordingBrowserDisplayParts(recording, index)` is now implemented locally
- title-derived folders are resolved locally
- path-derived folders are resolved locally
- VDR title/path marker cleanup still runs through `decodeRecordingText(...)`
- `recordingDisplayParts` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `app.js` no longer passes `recordingDisplayParts` to `recordingBrowser.configureContext(...)`
- only the shared detail target remains in the configured Recording browser context
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- narrow the remaining DOM mount dependency
- keep `app.js` as HTTP/loading bridge only
- consider later cleanup of legacy unused Recording helpers in `app.js`

---

## Phase 59.10o Implementation Status

Phase 59.10o moves the small DOM text helper into the Recording browser.

Implemented behavior:

- `recordingBrowserAddText(element, text)` is now implemented locally
- `recordingBrowserAddText(...)` sets `element.textContent`
- `recordingBrowserAddText(...)` returns the element
- `addText` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `app.js` no longer passes `addText` to `recordingBrowser.configureContext(...)`
- only the shared detail target and display-part helper remain in the configured Recording browser context
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- reduce or replace the remaining `recordingDisplayParts` context dependency
- keep `detailDataElement` as the explicit DOM mount target
- keep `app.js` as HTTP/loading bridge only

---

## Phase 59.10n Implementation Status

Phase 59.10n moves Recording-specific formatting helpers into the Recording browser.

Implemented behavior:

- `recordingBrowserFormatDurationSeconds(value)` is now implemented locally
- `recordingBrowserFormatSizeMb(value)` is now implemented locally
- `recordingBrowserFormatRecordingStart(value)` is now implemented locally
- `formatDurationSeconds` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `formatSizeMb` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `formatRecordingStart` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `app.js` no longer passes the three formatting helpers to `recordingBrowser.configureContext(...)`
- DOM and display-part helpers still come from the configured context
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- move the small DOM text helper into `recording-browser.js`
- reduce the remaining context dependency list
- keep `app.js` as HTTP/loading bridge only

---

## Phase 59.10m Implementation Status

Phase 59.10m moves simple response helpers into the Recording browser.

Implemented behavior:

- `recordingBrowserFirstValue(object, keys, fallback)` is now implemented locally
- `recordingBrowserListFromResponse(data, key)` is now implemented locally
- `firstValue` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `listFromResponse` is removed from `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- `app.js` no longer passes `firstValue` to `recordingBrowser.configureContext(...)`
- `app.js` no longer passes `listFromResponse` to `recordingBrowser.configureContext(...)`
- formatting and DOM helpers still come from the configured context
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- move Recording-specific formatting helpers into `recording-browser.js`
- reduce the remaining context dependency list
- keep `app.js` as HTTP/loading bridge only

---

## Phase 59.10l Implementation Status

Phase 59.10l removes the temporary global helper fallback from the Recording browser.

Implemented behavior:

- `recordingBrowserContextValue(name)` now requires `recordingBrowserContext`
- missing context fails with `Recording browser context is not configured`
- missing context members still fail with `Recording browser context value missing: <name>`
- `recording-browser.js` no longer defines `recordingBrowserFallbackContextValue(name)`
- `recording-browser.js` no longer checks global helper symbols with `typeof ...`
- shared helper access is only routed through the configured context
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- reduce the context dependency list by moving helper implementations into `recording-browser.js`
- keep `app.js` as HTTP/loading bridge only
- keep `recording-browser.js` UI-only

---

## Phase 59.10k Implementation Status

Phase 59.10k routes shared Recording browser helper calls through context accessors.

Implemented behavior:

- `recording-browser.js` defines `recordingBrowserContextValue(name)`
- `recording-browser.js` defines local wrappers for:
  - `detailDataElement`
  - `addText`
  - `firstValue`
  - `listFromResponse`
  - `formatDurationSeconds`
  - `formatSizeMb`
  - `formatRecordingStart`
  - `recordingDisplayParts`
- Recording rendering calls the local wrappers instead of direct shared globals
- `recordingBrowserDisplayParts(...)` gets its source helper through `recordingBrowserContextValue('recordingDisplayParts')`
- the configured context remains the preferred source
- a guarded fallback keeps the module compatible while the script order still exposes shared app helpers
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- remove the temporary fallback to global helper symbols
- reduce the context dependency list as helpers move fully into `recording-browser.js`
- keep `recording-browser.js` UI-only

---

## Phase 59.10j Implementation Status

Phase 59.10j removes the global Recording display-parts mutation.

Implemented behavior:

- `recording-browser.js` defines `recordingBrowserDisplayParts(recording, index)`
- the helper reads `recordingDisplayParts` from `recordingBrowserContext` when configured
- the helper keeps the VDR display cleanup:
  - `#xx` decoding
  - underscore-to-space conversion
  - leading `%` marker cleanup
- `recording-browser.js` no longer reassigns `recordingDisplayParts`
- Recording folder/tree/list rendering uses `recordingBrowserDisplayParts(...)`
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- switch remaining Recording browser calls from globals to `recordingBrowserContext`
- remove the temporary global helper fallback
- keep `recording-browser.js` UI-only

---

## Phase 59.10i Implementation Status

Phase 59.10i adds the Recording browser context API handshake.

Implemented behavior:

- `recording-browser.js` defines `configureRecordingBrowserContext(context)`
- `recording-browser.js` stores the provided context in `recordingBrowserContext`
- `window.VdrSuiteRecordingBrowser` exposes:
  - `contextDependencies`
  - `configureContext`
  - `renderList`
  - the existing UI helpers
- `app.js` calls `recordingBrowser.configureContext(...)` before `renderList(data)`
- the context passed by `app.js` includes:
  - `detailDataElement`
  - `addText`
  - `firstValue`
  - `listFromResponse`
  - `formatDurationSeconds`
  - `formatSizeMb`
  - `formatRecordingStart`
  - `recordingDisplayParts`
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- switch internal Recording browser calls from globals to `recordingBrowserContext`
- remove the temporary global Recording helper dependency bridge
- keep `recording-browser.js` UI-only

---

## Phase 59.10h Implementation Status

Phase 59.10h prepares the Recording browser context boundary.

Implemented behavior:

- `recording-browser.js` defines `RECORDING_BROWSER_CONTEXT_DEPENDENCIES`
- the dependency list documents the temporary shared context from `app.js`
- the listed shared dependencies are:
  - `detailDataElement`
  - `addText`
  - `firstValue`
  - `listFromResponse`
  - `formatDurationSeconds`
  - `formatSizeMb`
  - `formatRecordingStart`
  - `recordingDisplayParts`
- the frontend ownership guard verifies every listed dependency is still provided by `app.js`
- the guard prevents `recording-browser.js` from adding `window.VdrSuiteClientApi`
- the guard prevents `recording-browser.js` from adding literal `/api/` routes
- no runtime HTTP behavior and no backend route is changed

Next open frontend extraction areas:

- replace the shared global dependencies with an injected Recording browser context
- move remaining Recording-specific helper dependencies out of `app.js`
- keep `recording-browser.js` UI-only

---

## Phase 59.10g Implementation Status

Phase 59.10g cleans VDR encoded Recording display titles in `recording-browser.js`.

Implemented behavior:

- `decodeRecordingText(value)` strips leading VDR `%` markers
- the cleanup is applied through the existing Recording display hook
- folder titles, Recording list titles and Recording detail titles share the cleanup
- HTTP loading remains in `app.js`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Observed real-data symptom fixed:

- `%Schwarzer Donnerstag` renders as `Schwarzer Donnerstag`
- `%Kampf gegen die Krise` renders as `Kampf gegen die Krise`

Next open frontend extraction areas:

- replace shared global dependencies with a narrow Recording browser context
- move remaining Recording-specific helper dependencies out of `app.js`
- keep `recording-browser.js` UI-only

---

## Phase 59.10f Implementation Status

Phase 59.10f migrates the rich Recording renderer into `recording-browser.js`.

Implemented behavior:

- `recording-browser.js` owns `renderRecordingList(data)`
- `recording-browser.js` owns the rich Recording folder tree renderer
- `recording-browser.js` owns the Recording detail view
- `recording-browser.js` owns 20-item Recording paging
- `recording-browser.js` exposes `renderList: renderRecordingList`
- `app.js` no longer owns `renderRecordingList(data)`
- `app.js` routes loaded Recording data through `window.VdrSuiteRecordingBrowser.renderList(data)`
- HTTP loading remains in `app.js` through `window.VdrSuiteClientApi.fetchClientRecordings()`
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route is changed

Next open frontend extraction areas:

- replace shared global dependencies with a narrow Recording browser context
- move remaining Recording-specific helper dependencies out of `app.js`
- keep `recording-browser.js` UI-only

---

## Phase 59.10e Implementation Status

Phase 59.10e prepares the rich Recording renderer migration.

Implemented behavior:

- the frontend ownership guard verifies `app.js` still owns the rich `renderRecordingList(data)` renderer
- the guard verifies the rich renderer still contains:
  - folder tree building
  - Recording detail view
  - Recording list item creation
  - folder node rendering
  - 20-item Recording paging
  - previous and next paging controls
  - single-recording leaf folder promotion
- the guard prevents `app.js` from switching to `window.VdrSuiteRecordingBrowser` before the rich renderer is moved
- the guard verifies `recording-browser.js` still contains only the lightweight renderer baseline
- the guard prevents a partial rich renderer migration into `recording-browser.js`
- no runtime behavior and no backend route is changed

Next open frontend extraction areas:

- move the rich `renderRecordingList(data)` block from `app.js` into `recording-browser.js`
- then migrate `loadRecordings()` to call `window.VdrSuiteRecordingBrowser`
- preserve detail view, folder batching and 20-item paging during the move

---

## Phase 59.10d Implementation Status

Phase 59.10d exposes a UI-only Recording browser module API surface.

Implemented behavior:

- `recording-browser.js` documents the module API surface phase
- `window.VdrSuiteRecordingBrowser` is exposed by `recording-browser.js`
- the module API currently exposes:
  - `decodeRecordingText`
  - `setRecords`
  - `renderRoot`
  - `renderNode`
- `setRecords` owns the local `currentRecordingRecords` state update
- `recording-browser.js` still does not call `fetch()`
- `recording-browser.js` still does not use `window.VdrSuiteClientApi`
- no backend route and no runtime HTTP behavior is changed
- `app.js` is not migrated to the module API yet

Next open frontend extraction areas:

- migrate `app.js` Recording rendering to call `window.VdrSuiteRecordingBrowser`
- replace the global dependency bridge with a narrow Recording browser context
- move additional Recording UI helpers out of `app.js`

---

## Phase 59.10c Implementation Status

Phase 59.10c guards the temporary dependency boundary of the extracted Recording browser runtime.

Implemented behavior:

- `recording-browser.js` documents the dependency-boundary phase
- `recording-browser.js` remains an explicit DOM-rendering module
- `recording-browser.js` must not use `fetch()`
- `recording-browser.js` must not use `window.VdrSuiteClientApi`
- `recording-browser.js` must not own `/api/`, `XMLHttpRequest`, `EventSource` or `WebSocket`
- the frontend ownership guard verifies the shared app.js dependencies used during migration:
  - `detailDataElement`
  - `addText`
  - `firstValue`
  - `recordingDisplayParts`
- no backend route and no runtime behavior is changed

Next open frontend extraction areas:

- replace the global dependency bridge with a narrow Recording browser context
- move additional Recording UI helpers out of `app.js`
- keep HTTP ownership in `window.VdrSuiteClientApi`

---

## Phase 59.10b Implementation Status

Phase 59.10b extracts the Recording browser runtime from `web/frontend/index.html` into `web/frontend/recording-browser.js`.

Implemented behavior:

- `index.html` loads `recording-browser.js` after `channel-browser.js`
- the old inline Recording browser script is removed from `index.html`
- `recording-browser.js` owns `renderRecordingNode(node)`
- `recording-browser.js` keeps the Recording display text normalization hook
- `recording-browser.js` renders into the shared detail data element during this migration slice
- `recording-browser.js` does not call `fetch()` directly
- HTTP ownership remains in `window.VdrSuiteClientApi` through `app.js`
- no backend route is changed

Next open frontend extraction areas:

- move more Recording UI helpers out of `app.js`
- define a narrower shared frontend context instead of relying on globals
- keep `recording-browser.js` free of direct backend fetches

---

## Phase 59.10a Implementation Status

Phase 59.10a prepares the Recording browser frontend module as a static asset before extracting runtime logic from `index.html`.

Implemented behavior:

- `web/frontend/recording-browser.js` exists as a static asset placeholder
- `TestHttpServer` allows `/frontend/recording-browser.js`
- `TestHttpServer` maps `/frontend/recording-browser.js` to `recording-browser.js`
- `install-runtime` installs `web/frontend/recording-browser.js`
- `test-install-staging` verifies the installed Recording browser asset
- the frontend ownership guard verifies the static serving and install contracts
- `index.html` does not load `recording-browser.js` yet
- no runtime behavior and no backend route is changed

Next open frontend extraction areas:

- move the inline Recording browser script from `index.html` into `web/frontend/recording-browser.js`
- load `recording-browser.js` after `channel-browser.js`
- keep HTTP ownership in `window.VdrSuiteClientApi`
- keep `recording-browser.js` free of direct `fetch()` calls

---

## Phase 59.09f Implementation Status

Phase 59.09f adds a checked Web Client API contract snapshot.

Implemented behavior:

- `docs/development/web-client-api-contract-snapshot.md` documents every exported `fetchClient*` helper
- the frontend ownership guard verifies the snapshot contains every defined and exported Web Client API helper
- the snapshot documents that `web/frontend/app.js` has no remaining direct `fetch()` calls
- the snapshot documents the missing backend route gaps that must not be faked
- the development documentation index links the snapshot as a current project-state document
- no runtime behavior and no backend route is changed

Next open Web Client API areas:

- UI module extraction using `window.VdrSuiteClientApi` as the only HTTP boundary
- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.09e Implementation Status

Phase 59.09e migrates channel sorter API access in `web/frontend/app.js` to the Web Client API wrapper.

Implemented behavior:

- channel sorter list loading now uses `window.VdrSuiteClientApi.fetchClientChannels()`
- channel move actions now use `window.VdrSuiteClientApi.fetchClientChannelMoveAction()`
- `fetchClientChannelMoveAction(options)` owns `/api/vdr/channels/move`
- the direct-fetch legacy inventory is now empty
- `web/frontend/app.js` must not call `fetch()` directly
- no backend route and no UI expansion is added

Remaining known direct API fetch inventory:

- none in `web/frontend/app.js`

Next open Web Client API areas:

- client API contract snapshot
- preparation for UI module extraction
- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.09d Implementation Status

Phase 59.09d migrates backend selection API access in `web/frontend/app.js` to the Web Client API wrapper.

Implemented behavior:

- backend list loading now uses `window.VdrSuiteClientApi.fetchClientBackends()`
- backend snapshot loading now uses `window.VdrSuiteClientApi.fetchClientBackendSnapshot()`
- `fetchClientBackendSnapshot(backendId, options)` owns `/api/backends/<id>/snapshot`
- `/api/backends` is removed from the direct-fetch legacy inventory
- `/api/backends/` is removed from the direct-fetch legacy inventory
- the remaining literal direct-fetch legacy inventory is reduced to channel move
- no backend route and no UI expansion is added

Remaining known literal direct API fetch inventory:

- none

Next open Web Client API areas:

- add or verify channel move wrapper before migrating channel move UI
- reduce variable-based channel sorter loading after channel move ownership is explicit
- preparation for UI module extraction
- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.09c Implementation Status

Phase 59.09c migrates EPG timer direct API access in `web/frontend/app.js` to the Web Client API wrapper.

Implemented behavior:

- EPG timer live-sync now uses `window.VdrSuiteClientApi.fetchClientTimers()`
- EPG timer creation now uses `window.VdrSuiteClientApi.fetchClientTimerCreateAction()`
- `/api/vdr/timers/live` is removed from the direct-fetch legacy inventory
- `/api/vdr/timers/actions/create` is removed from the direct-fetch legacy inventory
- the remaining direct-fetch legacy inventory is reduced to backend selection and channel move routes
- no backend route and no UI expansion is added

Remaining known legacy direct API fetch inventory:

- `/api/vdr/channels/move`

Next open Web Client API areas:

- migrate backend selection direct fetches to `window.VdrSuiteClientApi`
- add or verify channel move wrapper before migrating channel move UI
- preparation for UI module extraction
- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.09b Implementation Status

Phase 59.09b hardens browser API ownership in `web/frontend/app.js` by turning the remaining direct API fetches into an explicit legacy inventory.

Implemented behavior:

- the frontend ownership guard scans `web/frontend/app.js` for direct literal `fetch('/api...')`, `fetch("/api...")` and `fetch(`/api...`)` calls
- known legacy direct API fetches are explicitly allowlisted
- new direct browser API access in `app.js` is rejected
- frontend API access must move through `window.VdrSuiteClientApi`
- `web/frontend/api/client-api.js` remains the only current web API wrapper
- no backend route and no UI expansion is added

Known legacy direct API fetch inventory:

- `/api/backends`
- `/api/backends/`
- `/api/vdr/channels/move`

Next open Web Client API areas:

- migrate the remaining legacy direct API fetches to `window.VdrSuiteClientApi`
- preparation for UI module extraction
- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.09a Implementation Status

Phase 59.09a hardens the Web Client API export registry.

Implemented behavior:

- the frontend ownership guard extracts all defined `fetchClient...` functions from `web/frontend/api/client-api.js`
- the guard extracts all exported `fetchClient...` entries from `window.VdrSuiteClientApi`
- every defined `fetchClient...` function must be exported
- every exported `fetchClient...` entry must refer to a defined function
- the existing explicit route ownership checks remain in place
- the wrapper remains DOM-free
- no new backend route and no UI expansion is added

Next open Web Client API areas:

- direct browser fetch guard hardening for `web/frontend/app.js`
- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.08j Implementation Status

Phase 59.08j guards the Web Client API against fake wrappers for backend route gaps.

Guarded backend route gaps:

- permission report routes
- dedicated event detail routes
- dedicated event media/artwork routes
- recording marks routes
- recording resume routes
- recording cut routes
- recording playback routes

Implemented behavior:

- the frontend ownership guard rejects fake Client API wrappers for the route gaps above
- the guard is intentionally negative until backend contracts exist
- future backend work must add real routes first, then update the guard and only then add wrappers
- the wrapper remains DOM-free
- no UI expansion is added

Next open Web Client API areas:

- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers once backend exposes dedicated contracts
- permission report route once backend exposes one

---

## Phase 59.08i Implementation Status

Phase 59.08i moves VDR runtime state HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientVdrOverview()` owns `/api/vdr/overview` with `/api/vdr` fallback
- `fetchClientVdrStatus()` owns `/api/vdr/status`
- `fetchClientVdrHealth()` owns `/api/vdr/health`
- `fetchClientVdrSnapshotSummary()` owns `/api/vdr/snapshot`
- `fetchClientVdrSnapshots()` owns `/api/vdr/snapshots`
- the wrapper remains DOM-free
- no VDR runtime state UI expansion is added yet

Next open Web Client API areas:

- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers
- permission report route once backend exposes one

---

## Phase 59.08h Implementation Status

Phase 59.08h moves metadata, person and auxiliary EPG read HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientMetadata()` owns `/api/metadata`
- `fetchClientPersons()` owns `/api/vdr/persons` with `/api/persons` fallback
- `fetchClientRecordingPersons()` owns `/api/vdr/recordings/persons/search` with `/api/recordings/persons/search` fallback
- `fetchClientEpgNowNext()` owns `/api/epg/now-next`
- `fetchClientEpgTimeWindow()` owns `/api/epg/time-window`
- `fetchClientEpgChannelWindow()` owns `/api/epg/channel-window`
- backend-scoped recording-person queries are normalized through `backendQueryOptions()`
- the wrapper remains DOM-free
- no metadata, person or auxiliary EPG UI expansion is added yet
- dedicated event detail and media/artwork route contracts remain backend gaps

Next open Web Client API areas:

- event detail and media/artwork routes once backend exposes dedicated contracts
- recording marks, resume, cut and playback helpers
- permission report route once backend exposes one

---

## Phase 59.08g Implementation Status

Phase 59.08g moves SearchTimer workflow and mutation HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientSearchTimerPlan()` owns `/api/vdr/searchtimers/plan` with `/api/searchtimers/plan` fallback
- `fetchClientSearchTimerValidate()` owns `/api/vdr/searchtimers/validate` with `/api/searchtimers/validate` fallback
- `fetchClientSearchTimerExecute()` owns `/api/vdr/searchtimers/execute` with `/api/searchtimers/execute` fallback
- `fetchClientSearchTimerRealTest()` owns `/api/vdr/searchtimers/real-test` with `/api/searchtimers/real-test` fallback
- `fetchClientSearchTimerCreateAction()` owns `/api/vdr/searchtimers` with `/api/searchtimers` fallback
- `fetchClientSearchTimerUpdateAction()` owns `/api/vdr/searchtimers/update` with `/api/searchtimers/update` fallback
- `fetchClientSearchTimerDeleteAction()` owns `/api/vdr/searchtimers/delete` with `/api/searchtimers/delete` fallback
- JSON request bodies are normalized through `jsonPostOptions()`
- the wrapper remains DOM-free
- no SearchTimer mutation UI is added yet

Next open Web Client API areas:

- permission report route once backend exposes one

---

## Phase 59.08f Implementation Status

Phase 59.08f moves Timer create, update and delete action HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientTimerCreateAction()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientTimerCreateAction()` owns `/api/vdr/timers/actions/create`
- `fetchClientTimerUpdateAction()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientTimerUpdateAction()` owns `/api/vdr/timers/actions/update`
- `fetchClientTimerDeleteAction()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientTimerDeleteAction()` owns `/api/vdr/timers/actions/delete`
- JSON request bodies are normalized through `jsonPostOptions()`
- the wrapper remains DOM-free
- no Timer action UI is added yet

Next open Web Client API areas:

- permission report route once backend exposes one

---

## Phase 59.08e Implementation Status

Phase 59.08e moves capability and backend state HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientCapabilities()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientCapabilities()` owns `/api/vdr/capabilities`
- `fetchClientBackends()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientBackends()` owns `/api/backends`
- `fetchClientDefaultBackend()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientDefaultBackend()` owns `/api/backends/default`
- generic permission reporting is not faked in the frontend and remains a backend route gap
- the wrapper remains DOM-free
- no capability or permission UI is added yet

Next open Web Client API areas:

- permission report route once backend exposes one

---

## Phase 59.08d Implementation Status

Phase 59.08d routes SearchTimer list loading through the Web Client API wrapper.

Implemented behavior:

- `fetchClientSearchTimers()` now tries `/api/vdr/searchtimers/live`
- `fetchClientSearchTimers()` falls back to `/api/vdr/searchtimers`
- `fetchClientSearchTimers()` falls back to `/api/searchtimers`
- `loadSearchTimers()` uses `window.VdrSuiteClientApi.fetchClientSearchTimers()`
- `loadSearchTimers()` no longer calls `fetch()` directly
- the wrapper remains DOM-free
- no SearchTimer UI expansion is added yet

Next open Web Client API areas:

- capability and permission report
- timer mutation workflows
- SearchTimer mutation workflows

---

## Phase 59.08c Implementation Status

Phase 59.08c moves Recording action validation and execution HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientRecordingActionValidation()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientRecordingActionValidation()` uses `/api/vdr/recordings/actions/validate`
- `fetchClientRecordingActionValidation()` keeps `/api/recordings/actions/validate` as a compatibility fallback
- `fetchClientRecordingActionExecution()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientRecordingActionExecution()` uses `/api/vdr/recordings/actions/execute`
- `fetchClientRecordingActionExecution()` keeps `/api/recordings/actions/execute` as a compatibility fallback
- JSON request bodies are normalized through `jsonPostOptions()`
- the wrapper remains DOM-free
- no Recording action UI is added yet

Next open Web Client API areas:

- capability and permission report
- timer mutation workflows
- SearchTimer mutation workflows

---

## Phase 59.08b Implementation Status

Phase 59.08b moves EPGSearch query and SearchTimer preview HTTP access behind the Web Client API wrapper.

Implemented behavior:

- `fetchClientEpgSearch()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientEpgSearch()` owns `/api/epg/search`
- `fetchClientSearchTimerPreview()` is exposed by `web/frontend/api/client-api.js`
- `fetchClientSearchTimerPreview()` uses `/api/vdr/searchtimers/preview`
- `fetchClientSearchTimerPreview()` keeps `/api/searchtimers/preview` as a compatibility fallback
- backend-aware callers may pass `backendId`; the wrapper maps it to the existing backend query parameter for these backend routes
- the wrapper remains DOM-free
- no EPGSearch or SearchTimer preview UI is added yet

Next open Web Client API areas:

- recording action validation and execution
- capability and permission report
- timer mutation workflows
- SearchTimer mutation workflows

