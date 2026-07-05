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
Phase 58.94d - Timer Conflict Frontend Renderer
```

Current documentation consolidation state:

```text
Phase 58.90c - Documentation Consolidation
```

Current implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

---

## Latest Verified Implementation Slice

Phase 58.94d connects the already verified RESTfulAPI timer conflict discovery endpoint to the Timer module in the web frontend.

Stable scope:

- preserve the existing live timer list rendering
- fetch `/api/vdr/timers/conflicts/live` after rendering timers
- prepend a Timer-Konflikte panel above the timer cards
- show conflict count, source, conflict time, timer indices, percentages and concurrent timer indices
- show explicit states for loading, unavailable conflict source and no conflicts

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

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start handling is verified.
- Phase 58.39 verifies bounded live EPG input for channel cards.
- Phase 58.90b verifies stable channel sorting on desktop and touch devices.
- Phase 58.94c verifies RESTfulAPI timer conflict discovery with live count=2 and total=2.

### Guarded or deliberately incomplete areas

- SearchTimer production changes remain gated and closed by default.
- Recording operation write probes remain explicitly gated.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy remain planned beyond the current access-mode foundation.
- Web, Windows, Android, iOS and TV frontends remain planned product layers; the current web frontend is a Phase 58 foundation, not the final client product.

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
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)

## Phase 58.94c: RESTfulAPI Timer Conflict Discovery

Status: stable/live verified

Implemented:

- Added VdrTimerConflict and VdrTimerConflictReport domain objects.
- Added RestfulApiTimerConflictMapper for /searchtimers/conflicts.json.
- Added RestfulApiVdrAdapter::getTimerConflictReport().
- Added VdrService::getTimerConflictReport().
- Added VdrSnapshotReadJsonSerializer::serializeTimerConflictReport().
- Added VdrController live conflict endpoint.
- Added API routes:
  - /api/vdr/timer-conflicts/live
  - /api/vdr/timers/conflicts/live

Live verification:

- Source: restfulapi-epgsearch
- available: true
- count: 2
- total: 2
- Conflict entries expose timerIndex, percentage and concurrentTimerIndices.

Architecture decision:

RESTfulAPI is the primary timer conflict source. SVDRP epgsearch LSCC remains a validation and fallback path for later phases.

## Phase 58.94d: Timer Conflict Frontend Renderer

Status: GitHub patch applied; local runtime verification still required after pull/install.

Implemented:

- Added `web/frontend/timer-conflicts.js` as a dedicated frontend renderer for timer conflict reports.
- Preserved the existing `renderTimerList()` flow and decorated it instead of replacing the timer list implementation.
- Fetches `/api/vdr/timers/conflicts/live` after the live timer list is rendered.
- Prepends a `Timer-Konflikte` panel above the timer cards.
- Shows loading, unavailable source, no-conflict and active-conflict states.
- Renders conflict time, timer index, percentage, concurrent timer indices and remote server if present.
- Loads the renderer through the existing frontend bootstrap path.

Verification still required locally:

- `node --check web/frontend/timer-conflicts.js`
- `node --check web/frontend/channel-logos.js`
- install updated frontend assets to `/usr/share/vdr-suite/web/frontend/`
- reload the web frontend and verify that the Timer tab shows the conflict panel when `/api/vdr/timers/conflicts/live` returns count > 0.
