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
Phase 58.95 - Frontend Ownership Contracts
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

Phase 58.95 adds an explicit ownership contract for the script-based Phase 58 web frontend so feature patches do not drift into unrelated helper files.

Stable scope:

- document `index.html`, `app.js`, `channel-logos.js`, `channel-browser.js` and `style.css` ownership boundaries
- define the Timer conflict integration rule as Timer-module-owned behavior
- make `channel-logos.js` helper-only and explicitly forbid dynamic script loading there
- add a static frontend ownership contract check in `tools/check_frontend_ownership_contracts.py`
- guard script order and helper boundaries before further frontend modularization

Previous verified slice:

- Phase 58.94d connected the verified RESTfulAPI timer conflict discovery endpoint to the Timer module in the web frontend.
- Phase 58.94c verified RESTfulAPI timer conflict discovery with live `count=2` and `total=2`.

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

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start handling is verified.
- Phase 58.39 verifies bounded live EPG input for channel cards.
- Phase 58.90b verifies stable channel sorting on desktop and touch devices.
- Phase 58.94c verifies RESTfulAPI timer conflict discovery with live count=2 and total=2.
- Phase 58.95 documents frontend ownership boundaries and adds a static guard against helper-file module drift.

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
