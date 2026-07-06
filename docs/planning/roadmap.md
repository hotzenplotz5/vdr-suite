# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Phase Map](phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Current Position

```text
Completed major project block
Phase 57 - Multi-Site Backend Administration and Permissions

Latest completed implementation slice
Phase 59.06b - Recording Details and EPG Vertical Drag UX

Next planned implementation slice
Phase 59.07a - Live, RESTfulAPI and EPGSearch Parity Inventory Refresh

Current implementation focus
Phase 58 - Frontend and Live Parity
```

---

## Purpose

This roadmap describes the current direction of VDR-Suite without duplicating the full phase history.

The compact source of truth for phase-range coverage is [Phase Map](phase-map.md).

The primary human entry point for the current repository state is [Current State](../CURRENT.md).

Detailed chronological implementation history belongs to [Completed Phases](../development/completed-phases.md).

Product parity and frontend gap planning belongs to [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md).

---

## Phase Map Summary

See [Phase Map](phase-map.md) for the canonical compact table.

Completed foundation ranges:

- Phase 1.x-7.x: Core Platform.
- Phase 8.x: VDR Backend.
- Phase 9.x-29.x: Multi-Backend Runtime.
- Phase 30.x-36.x: Recording Actions.
- Phase 37.x-44.x: Recording Runtime Hardening.
- Phase 45.x: EPG Search.
- Phase 46.x: Metadata and People.
- Phase 47.x-49.x: SearchTimer Backend.
- Phase 50.0-50.50: SearchTimer Workflow.
- Phase 51.x: Live Parity Discovery.
- Phase 52.x: SearchTimer Automation Planning.
- Phase 53.x: SearchTimer Completion Audit.
- Phase 54.x: SearchTimer Preview Runtime.
- Phase 55.x: Adapter, acceptance and documentation hardening.
- Phase 56: Library Boundary, Packaging and Developer Documentation.
- Phase 57: Multi-Site Backend Administration and Permissions.
- Phase 58.0-58.90b: Frontend/live-parity slices including event hints, channel move API and stable channel sorter.

---

## Recently Completed Implementation Slice

### Phase 59.06b - Recording Details and EPG Vertical Drag UX

Status: Completed frontend hardening and UX slice.

Completed outcomes:

- Recording module keeps `/api/vdr/recordings/query` as its data source
- Recording rendering is bounded for large real catalogs
- Recording folders are rendered as a hierarchical tree
- nested folders are no longer flattened into the root view
- folder recordings use explicit 20-item paging
- previous and next page controls are visible inside folders
- single-recording leaf folders are displayed as recordings instead of fake folders
- recording entries open an in-module detail view
- visible-channel EPG rendering reuses one event index
- EPG time-window, program-view and channel-window controls use deterministic helper paths
- vertical EPG channel scrolling exposes drag affordance and a visible drag hint
- frontend ownership contracts guard the current frontend module model
- `web/frontend/api/client-api.js` remains the DOM-free frontend API boundary
- `web/frontend/app.js` remains the current frontend module owner

---

## Next Planned Implementation Slice

### Phase 59.07a - Live, RESTfulAPI and EPGSearch Parity Inventory Refresh

Status: Planned.

Goal:

- Reconcile source code, RESTfulAPI capability knowledge and existing documentation before adding more Live-parity features.

Planned scope:

- compare Live, epgsearch and RESTfulAPI capability surfaces against VDR-Suite source
- mark implemented backend-neutral VDR-Suite capabilities as done
- downgrade stale `MISSING` or `CHECK` documentation entries where code now exists
- keep UI gaps separate from backend/domain gaps
- identify remaining real VDR evidence gaps
- define the next implementation target from the refreshed parity matrix

---

## Recently Completed Major Milestone

### Phase 57 - Multi-Site Backend Administration and Permissions

Status: Completed.

Completed outcomes:

- backend access modes
- backend registry permission hints
- recording action access handling
- timer action access handling
- SearchTimer access handling
- frontend-visible backend permission state

---

## Planned Major Milestones

### Phase 58 - Frontend and Live Parity

Status: In progress.

Goal:

- Build frontend-ready everyday recording, timer, channel and EPG views after the backend permission foundation.

Planning input:

- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)

Phase 58.0 start audit:

- Inventory frontend read models before adding UI code.
- Map existing REST endpoints to frontend views.
- Confirm backend selector data from /api/backends.
- Confirm write hints for recording, timer and SearchTimer buttons.
- Keep read-only backend views visible but disable write actions.
- Use Live parity data to expose real feature gaps.

Frontend-relevant endpoints:

- /api/backends
- /api/vdr/overview
- /api/vdr/recordings
- /api/vdr/timers
- /api/vdr/searchtimers
- /api/vdr/epg/search
- /api/vdr/live-parity
- /api/vdr/channels/move

---

### Phase 59 - Suite Metadata Database and External Providers

Status: Planned.

Goal:

- Build a suite-owned metadata database while using external scraper/catalog providers behind boundaries.

---

### Phase 60 - Recommendation and Content Knowledge Graph

Status: Vision.

Goal:

- Build recommendation and graph primitives after metadata and frontend foundations mature.

---

## Roadmap Maintenance Rules

- [Current State](../CURRENT.md) is the first human entry point for current repository truth.
- [Phase Map](phase-map.md) is the compact source of truth for phase-range coverage.
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md) records feature parity and frontend gap planning.
- This roadmap describes direction and should not duplicate the detailed completed phase log.
- Detailed chronological implementation history belongs in [Completed Phases](../development/completed-phases.md).
- Project status snapshots belong in [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).
- Planned phase numbers must not conflict with completed phase ranges.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Project Overview](../project-overview.md)
