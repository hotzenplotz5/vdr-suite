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
Phase 59.04c - Recording Folder Paging Controls
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

Phase 59.04c completes the first Recording frontend performance hardening slice after the frontend client API recovery chain.

Stable scope:

- Phase 59.03 batches EPG cache window loading for the visible channel set.
- Phase 59.03b serves `/frontend/api/client-api.js` through the daemon HTTP frontend whitelist.
- Phase 59.03c removes the broken recording live-only client route.
- Phase 59.03d loads recordings through `/api/vdr/recordings/query`.
- Phase 59.04a bounds Recording module rendering for large real catalogs.
- Phase 59.04b renders recordings as a hierarchical folder tree instead of a flat folder map.
- Phase 59.04c adds explicit Recording folder paging controls with 20 recordings per page.
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

### Guarded or deliberately incomplete areas

- SearchTimer production changes remain gated and closed by default.
- Recording operation write probes remain explicitly gated.
- Recording query loading is restored and the first Recording frontend performance slice is complete through bounded rendering, folder-tree navigation and 20-item paging; deeper recording detail/action UI remains a follow-up.
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
