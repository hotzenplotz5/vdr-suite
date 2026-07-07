# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Phase Map](../planning/phase-map.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite.

---

## Current Verified State

Latest completed major implementation block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Latest completed implementation slice:

```text
Phase 59.12i - Channel Browser Legacy Callback Hotfix
```

Current documentation consolidation state:

```text
Phase 58.90c - Documentation Consolidation
```

Current implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

Next planned implementation slice:

```text
Phase 59.04 - Recording and EPG Frontend Performance Hardening
```

---

## Latest Verified Implementation Slice

Phase 59.12i3 restores the runtime-safe Channel browser script path while keeping the module source path.

Stable scope:

- Phase 59.03 batches EPG cache window loading for the visible channel set.
- Phase 59.03b serves `/frontend/api/client-api.js` through the daemon HTTP frontend whitelist.
- Phase 59.03c removes the broken recording live-only client route.
- Phase 59.03d loads recordings through `/api/vdr/recordings/query`.
- Phase 59.04a bounds Recording module rendering for large real catalogs.
- Phase 59.04b renders recordings as a hierarchical folder tree instead of a flat folder map.
- Phase 59.04c adds explicit Recording folder paging controls with 20 recordings per page.
- Phase 59.04d promotes single-recording leaf folders into recording entries instead of rendering them as fake folders.
- Phase 59.05a adds a Recording detail view opened from recording entries.
- Phase 59.05b indexes visible EPG events for the active channel window.
- Phase 59.05c reuses the visible EPG event index in the live/now/next program views.
- Phase 59.05d builds the visible EPG event index in one event scan instead of repeated per-channel full-list filtering.
- Phase 59.05e removes the unused EPG overlap helper and cleans the remaining EPG channel-event helper signature.
- Phase 59.05f removes the now-unused EPG per-channel full-list helper after all active EPG views use the visible event index.
- Phase 59.05g removes raw EPG debug URL and per-channel debug counters from the visible EPG header text.
- Phase 59.05h clamps the EPG channel offset before rendering so stale offsets cannot produce an empty visible-channel window.
- Phase 59.05i guards the EPG visible-channel limit with a minimum of one before offset math and slicing.
- Phase 59.05j removes redundant EPG program-view reload branches that performed the same reload in both paths.
- Phase 59.05k deduplicates the EPG horizontal and vertical time-view reload fallback through one local helper.
- Phase 59.05l deduplicates the live/now/next EPG program-view switch through one local helper.
- Phase 59.05m deduplicates the EPG 24h time-window switch through one local helper while preserving the next-window program-view fallback.
- Phase 59.05n deduplicates the EPG channel-window pager through one local move helper.
- Phase 59.06a adds tooltip and accessibility affordance text for vertical EPG drag scrolling.
- Phase 59.06b adds a visible vertical EPG drag hint above the channel scrollbar.
- Phase 59.07a reconciles Live, RESTfulAPI and EPGSearch parity documentation with the newer VDR-Suite source state.
- Phase 59.08a routes SearchTimer discovery catalog loading through the Web Client API wrapper.
- Phase 59.08b routes EPGSearch query and SearchTimer preview loading through the Web Client API wrapper.
- Phase 59.08c routes Recording action validation and execution through the Web Client API wrapper.
- Phase 59.08d routes SearchTimer list loading in app.js through `fetchClientSearchTimers()`.
- Phase 59.08e routes capability and backend state loading through dedicated Web Client API wrappers.
- Phase 59.08f routes Timer create, update and delete actions through dedicated Web Client API wrappers.
- Phase 59.08g routes SearchTimer workflow and mutation actions through dedicated Web Client API wrappers.
- Phase 59.08h routes metadata, person search, recording-person search and auxiliary EPG reads through dedicated Web Client API wrappers.
- Phase 59.08i routes VDR overview, status, health and snapshot state reads through dedicated Web Client API wrappers.
- Phase 59.08j prevents fake Web Client API wrappers for permission report, event media/detail and recording marks/resume/cut/playback route gaps.
- Phase 59.09a guards the Web Client API export registry against missing or undefined fetchClient exports.
- Phase 59.09b guards `web/frontend/app.js` against new direct `/api...` fetch calls and inventories the remaining legacy direct fetches for later migration.
- Phase 59.09c migrates EPG timer live-sync and EPG timer create actions to `window.VdrSuiteClientApi`.
- Phase 59.09d migrates backend list and backend snapshot loading to `window.VdrSuiteClientApi`.
- Phase 59.09e migrates channel sorter list loading and channel move actions to `window.VdrSuiteClientApi`.
- Phase 59.09f adds a checked Web Client API contract snapshot for the UI module extraction handoff.
- Phase 59.10a prepares `recording-browser.js` as a served and installed static frontend asset without loading it yet.
- Phase 59.10b loads `recording-browser.js` after `channel-browser.js` and removes the inline Recording browser runtime from `index.html`.
- Phase 59.10c guards the temporary Recording browser dependency boundary against accidental runtime API ownership.
- Phase 59.10d exposes `window.VdrSuiteRecordingBrowser` as the explicit Recording browser UI-only API surface.
- Phase 59.10e guards the rich Recording renderer migration boundary before moving `renderRecordingList(data)` out of `app.js`.
- Phase 59.10f moves `renderRecordingList(data)` into `recording-browser.js` and routes `app.js` through `window.VdrSuiteRecordingBrowser.renderList(data)`.
- Phase 59.10g strips leading VDR `%` title markers from Recording folder/list/detail display text.
- Phase 59.10h documents the temporary Recording browser context boundary before dependency injection.
- Phase 59.10i adds the explicit Recording browser `configureContext()` API handshake.
- Phase 59.10j removes the global `recordingDisplayParts` mutation and uses `recordingBrowserDisplayParts(...)` locally.
- Phase 59.10k routes shared helper calls through Recording browser context accessors.
- Phase 59.10l removes the temporary global helper fallback and fails clearly if the context is missing.
- Phase 59.10m moves `firstValue` and `listFromResponse` behavior into local Recording browser helpers.
- Phase 59.10n moves Recording duration, size and start-time formatting into local Recording browser helpers.
- Phase 59.10o moves `addText` behavior into a local Recording browser DOM text helper.
- Phase 59.10p moves Recording display-parts behavior into local Recording browser helpers.
- Phase 59.10q replaces the remaining context object with `configureMountTarget(detailDataElement)`.
- Phase 59.10r removes migrated legacy Recording display helper functions from `app.js`.
- Phase 59.11a routes Channel browser EPG cache refresh through `fetchClientEpgCacheRefresh()`.
- Phase 59.11a1 restores `epgEventsForChannel(...)` for the Channel browser selected-channel programme view.
- Phase 59.11b enables drag-scroll on the selected-channel `Programm` list.
- Phase 59.11c limits Channel browser drag-scroll handling to mouse pointer input so touch/mobile scrolling remains native.
- Phase 59.11d documents the Channel browser app.js dependency inventory before additional extraction slices.
- Phase 59.11e introduces `window.VdrSuiteChannelBrowser.configureContext(...)` as the future Channel browser context boundary.
- Phase 59.11f configures the Channel browser context boundary from `app.js` with the current mount target.
- Phase 59.11g routes Channel browser mount-target access through `channelBrowserDetailDataElement()`.
- Phase 59.11h guards the Channel browser context boundary in `check_frontend_ownership_contracts.py`.
- Phase 59.11i synchronizes the Channel browser boundary documentation after the context accessor slices.
- Phase 59.11j moves `addText` behavior into `channelBrowserAddText(...)`.
- Phase 59.11k guards the local Channel browser DOM text helper in `check_frontend_ownership_contracts.py`.
- Phase 59.11l moves `firstValue` behavior into `channelBrowserFirstValue(...)`.
- Phase 59.11m guards the local Channel browser first-value helper in `check_frontend_ownership_contracts.py`.
- Phase 59.11n moves `listFromResponse` behavior into `channelBrowserListFromResponse(...)`.
- Phase 59.11o moves `listEventsFromEpgResponse` behavior into `channelBrowserListEventsFromEpgResponse(...)`.
- Phase 59.11p guards local Channel browser response helpers in `check_frontend_ownership_contracts.py`.
- Phase 59.11q documents Channel browser readiness for `window.VdrSuiteChannelBrowser.renderList(data)`.
- Phase 59.12a exposes `window.VdrSuiteChannelBrowser.renderList(data)` and keeps `renderChannelList(data)` as a compatibility bridge.
- Phase 59.12b routes `app.js` through `window.VdrSuiteChannelBrowser.renderList(data)` via `renderChannelsThroughModule(data)`.
- Phase 59.12c guards the Channel browser module bridge and prevents app.js from calling the legacy global bridge.
- Phase 59.12d removes the temporary global `renderChannelList(data)` bridge from `channel-browser.js`.
- Phase 59.12e keeps the physical asset at `web/frontend/channel-browser.js` until the daemon/static asset whitelist is updated.
- Phase 59.12f prepares `/frontend/modules/channels.js` serving and install contracts before moving the physical asset.
- Phase 59.12g moves the physical Channel browser asset from `web/frontend/channel-browser.js` to `web/frontend/modules/channels.js`.
- Phase 59.12h loads frontend modules before `app.js` so Channel browser context configuration can succeed.
- Phase 59.12i replaces the remaining `renderChannelList(...)` callback with `renderChannelBrowserList(...)`.
- Phase 59.12i3 loads the Channel browser through `/frontend/channel-browser.js` until the running daemon serves `/frontend/modules/channels.js`.
- EPG cache window loading remains SQLite-backed.
- Timer loading is verified through the Web Client API wrapper.
- Timer conflict loading is verified through `/api/vdr/timers/conflicts/live`.
- Recording loading is verified against a real catalog with 1007 recordings.
- `/api/vdr/recordings` snapshot emptiness no longer blocks the recording module.
- `web/frontend/api/client-api.js` remains DOM-free.
- `web/frontend/app.js` remains the current frontend module owner.

Previous verified slices:

- Phase 59.03 batches EPG cache window loading for visible channels.
- Phase 59.02b routed EPG cache status loading through fetchClientEpgCacheStatus().
- Phase 59.02a routed EPG Timeline Channel loading through fetchClientChannels().
- Phase 59.01 routed Channel list loading through fetchClientChannels().
- Phase 59.00 routed Timer conflict loading through fetchClientTimerConflicts().

---

<!-- PROJECT_PROGRESS_START -->
## Project State Snapshot

This is a verified implementation-state snapshot, not a product-completion percentage.

### Verified foundations

- Core runtime and daemon foundation
- VDR backend adapter and RESTfulAPI integration foundation
- Backend registry and multi-backend runtime foundation
- Snapshot cache and change-feed foundation
- REST routing and JSON response boundaries
- Recording query foundation
- Recording action validation foundation
- EPG query and search foundation
- Backend-scoped EPG database foundation
- SearchTimer backend and workflow foundations
- Live parity discovery foundation
- Channel move and stable frontend sorter foundation
- Timer conflict discovery and frontend rendering foundation
- Frontend ownership contract foundation
- Lightweight frontend ownership guard Make target
- Client API and frontend module boundary planning
- Web Client API Wrapper implementation
- Timer loading through Web Client API Wrapper
- Timer conflict loading through Web Client API Wrapper
- Channel loading through Web Client API Wrapper
- EPG Timeline Channel loading through Web Client API Wrapper
- EPG Cache Status loading through Web Client API Wrapper
- EPG Cache Window visible channel batch loading
- Frontend Client API wrapper static serving through daemon HTTP frontend routes
- Recording module loading through Recording Query Endpoint

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start handling is verified.
- Phase 58.39 verifies bounded live EPG input for channel cards.
- Phase 58.90b verifies stable channel sorting on desktop and touch devices.
- Phase 58.94c verifies RESTfulAPI timer conflict discovery with live count=2 and total=2.
- Phase 58.95 documents frontend ownership boundaries and adds a static guard against helper-file module drift.
- Phase 58.96 verifies `make test-frontend-contracts` as a lightweight guard target outside the heavier C++ fast-test path.
- Phase 58.97 defines the future Client API and frontend module boundary for Web, Windows, Linux, mobile and TV clients.
- Phase 58.98 introduces web/frontend/api/client-api.js as the first DOM-free web API wrapper.
- Phase 58.99 routes Timer list loading through fetchClientTimers().
- Phase 59.00 routes Timer conflict loading through fetchClientTimerConflicts().
- Phase 59.01 routes Channel list loading through fetchClientChannels().
- Phase 59.02a routes EPG Timeline Channel loading through fetchClientChannels().
- Phase 59.02b routes EPG cache status loading through fetchClientEpgCacheStatus().
- Phase 59.03 batches visible-channel EPG cache window loading into one `/api/epg/cache/window` request with `channelIds`.
- Phase 59.03b verifies `/frontend/api/client-api.js` is served by the daemon HTTP frontend route.
- Phase 59.03c verifies the recording Client API wrapper no longer depends on the missing `/api/vdr/recordings/live` route.
- Phase 59.03d verifies the Recording module loads the real recording catalog through `/api/vdr/recordings/query`.
- Phase 59.04a verifies bounded Recording rendering for large catalogs.
- Phase 59.04b verifies nested Recording folder tree rendering.
- Phase 59.04c verifies explicit previous/next paging controls for folder recordings.
- Phase 59.04d verifies single-recording leaf folders are displayed as recordings while real folders remain navigable.
- Phase 59.05a verifies recording entries can open an in-module detail view.
- Phase 59.05b verifies horizontal and vertical EPG time views reuse a visible-channel event index.
- Phase 59.05c verifies live/now/next EPG program views reuse the same visible-channel event index.
- Phase 59.05d verifies the visible EPG event index is built in one pass over the loaded event payload.
- Phase 59.05e verifies the obsolete EPG overlap helper has no remaining call sites and is removed.
- Phase 59.05f verifies the obsolete EPG per-channel full-list helper has no remaining call sites and is removed.
- Phase 59.05g verifies the EPG header keeps useful source/event information without exposing debug URL or sample channel counters.
- Phase 59.05h verifies the EPG channel offset is bounded before visible-channel slicing.
- Phase 59.05i verifies the visible-channel limit cannot become zero for EPG offset calculations.
- Phase 59.05j verifies EPG program-view buttons use one deterministic reload path.
- Phase 59.05k verifies EPG time-axis buttons share one deterministic rerender-or-load path.
- Phase 59.05l verifies EPG program-view buttons share one deterministic switch-and-load path.
- Phase 59.05m verifies EPG time-window buttons share one deterministic switch-and-load path.
- Phase 59.05n verifies EPG channel-window buttons share one deterministic move-and-load path.
- Phase 59.06a verifies the vertical EPG channel scrollbar, header row and scroll content expose drag-scrolling affordances.
- Phase 59.06b verifies the vertical EPG view renders an explicit drag hint for channel scrolling.
- Phase 59.07a verifies stale parity documentation entries are downgraded from missing/check where backend-neutral source support now exists.
- Phase 59.08a verifies `fetchClientSearchTimerDiscovery()` owns the SearchTimer discovery route access.
- Phase 59.08b verifies `fetchClientEpgSearch()` and `fetchClientSearchTimerPreview()` own their route access.
- Phase 59.08c verifies `fetchClientRecordingActionValidation()` and `fetchClientRecordingActionExecution()` own their route access.
- Phase 59.08d verifies `loadSearchTimers()` no longer directly fetches SearchTimer routes.
- Phase 59.08e verifies `fetchClientCapabilities()`, `fetchClientBackends()` and `fetchClientDefaultBackend()` own their route access.
- Phase 59.08f verifies `fetchClientTimerCreateAction()`, `fetchClientTimerUpdateAction()` and `fetchClientTimerDeleteAction()` own their route access.
- Phase 59.08g verifies SearchTimer plan, validate, execute, real-test, create, update and delete wrappers own their route access.
- Phase 59.08h verifies metadata, person, recording-person and auxiliary EPG read wrappers own their route access.
- Phase 59.08i verifies VDR overview, status, health, snapshot summary and snapshot list wrappers own their route access.
- Phase 59.08j verifies the Client API wrapper does not fake missing backend route gap tokens.
- Phase 59.09a verifies every defined fetchClient function is exported and every exported fetchClient name is defined.
- Phase 59.09b verifies `app.js` does not add new direct `/api...` fetch routes beyond the explicit legacy inventory.
- Phase 59.09c verifies the legacy direct fetch inventory no longer includes `/api/vdr/timers/live` or `/api/vdr/timers/actions/create`.
- Phase 59.09d verifies the legacy direct fetch inventory no longer includes `/api/backends` or `/api/backends/`.
- Phase 59.09e verifies `app.js` has no remaining direct `fetch()` calls.
- Phase 59.09f verifies every exported `fetchClient*` helper is documented in the Web Client API contract snapshot.
- Phase 59.10a verifies the daemon static frontend whitelist and install staging include `recording-browser.js`.
- Phase 59.10b verifies `recording-browser.js` owns the extracted Recording browser runtime and still has no direct `fetch()` calls.
- Phase 59.10c verifies `recording-browser.js` depends only on the documented shared frontend globals and still owns no runtime API access.
- Phase 59.10d verifies the Recording browser module API surface remains UI-only and free of runtime API access.
- Phase 59.10e verifies the rich Recording renderer still lives completely in `app.js` and is not partially migrated.
- Phase 59.10f verifies the rich Recording renderer lives in `recording-browser.js`, remains UI-only and preserves 20-item paging.
- Phase 59.10g verifies `recording-browser.js` keeps VDR title decode cleanup and remains API-free.
- Phase 59.10h verifies the Recording browser shared context dependency list remains explicit and API-free.
- Phase 59.10i verifies `app.js` hands the documented shared context to `recording-browser.js` before rendering.
- Phase 59.10j verifies Recording display text decoding no longer mutates the global `recordingDisplayParts` helper.
- Phase 59.10k verifies direct shared helper calls are replaced by Recording browser context accessors.
- Phase 59.10l verifies Recording browser context accessors no longer fall back to global helper symbols.
- Phase 59.10m verifies response helper behavior is local and no longer part of the Recording browser context.
- Phase 59.10n verifies Recording formatting helper behavior is local and no longer part of the Recording browser context.
- Phase 59.10o verifies DOM text helper behavior is local and no longer part of the Recording browser context.
- Phase 59.10p verifies Recording display-parts behavior is local and no longer part of the Recording browser context.
- Phase 59.10q verifies the Recording browser uses an explicit mount target instead of a dependency context object.
- Phase 59.10r verifies `app.js` no longer keeps the migrated legacy Recording display helper functions.

### Guarded or deliberately incomplete areas

- SearchTimer production changes remain gated and closed by default.
- Recording operation write probes remain explicitly gated.
- Recording query loading is restored and the first Recording frontend performance slice is complete through bounded rendering, folder-tree navigation, 20-item paging, single-recording leaf promotion and an in-module recording detail view; deeper recording action UI remains a follow-up.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy remain planned beyond the current access-mode foundation.
- Web, Windows, Android, iOS and TV frontends remain planned product layers; the current web frontend is a Phase 58 foundation, not the final client product.
- Full frontend module extraction remains planned; Phase 58.95 only defines and guards the current script-based ownership model.

### Current active focus

```text
Phase 58 - Frontend and Live Parity
```

### Later strategic milestones

- Multi-site backend federation and permission hardening
- Frontend and live-parity foundation
- EPG synchronization service
- Suite metadata database and external provider integration
- Safe production-grade recording operations

Progress source: ../planning/project-progress.md
<!-- PROJECT_PROGRESS_END -->

---

## Back

- [Back to Development Index](index.md)

- Phase 59.03 batches EPG cache window loading for visible channels.
