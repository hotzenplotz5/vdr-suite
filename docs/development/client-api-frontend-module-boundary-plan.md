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
- `fetchClientChannels`
- `fetchClientCapabilities`
- `fetchClientBackends`
- `fetchClientDefaultBackend`
- `fetchClientEpgWindow`
- `fetchClientEpgSearch`
- `fetchClientEpgCacheStatus`
- `fetchClientEpgCacheWindow`
- `fetchClientRecordings`
- `fetchClientRecordingActionValidation`
- `fetchClientRecordingActionExecution`
- `fetchClientSearchTimers`
- `fetchClientSearchTimerDiscovery`
- `fetchClientSearchTimerPreview`

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

- timer create/update/delete workflows
- SearchTimer create/update/delete workflows
- permission report route once backend exposes one
- event detail and media/artwork helpers
- recording marks, resume, cut and playback helpers

Next recommended frontend/API slices:

- add wrapper functions for EPGSearch query and SearchTimer preview
- add wrapper functions for recording action preview and execution
- add wrapper functions for capability and permission state
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

- timer mutation workflows
- SearchTimer mutation workflows
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

- timer mutation workflows
- SearchTimer mutation workflows
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

