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
Phase 59.02b - EPG Cache Status uses Web Client API Wrapper
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

Phase 59.02b routes EPG cache status loading through the
DOM-free web Client API wrapper.

Stable scope:

- keep fetchEpgCacheStatusForBackend() in web/frontend/app.js
- keep EPG rendering in web/frontend/app.js
- move /api/epg/cache/status access behind fetchClientEpgCacheStatus()
- preserve backend and _ cache-busting query parameters
- preserve no-store loading
- preserve __statusError fallback behavior
- keep EPG cache window loading unchanged
- keep EPG Timeline Channel loading through fetchClientChannels()
- do not extract an EPG UI module yet

Previous verified slices:

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

### Guarded or deliberately incomplete areas

- SearchTimer production changes remain gated and closed by default.
- Recording operation write probes remain explicitly gated.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
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
