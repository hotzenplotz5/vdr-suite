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
Phase 60.14k - Recording Detail UX Polish

Next planned implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation

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

### Phase 60.14k - Recording Browser UX Polish

Status: Completed frontend UX and runtime behavior polish slice.

Completed outcomes:

- Recording folder views show breadcrumb-style context for the current lazy folder
- single-recording leaf folders open directly into the Recording detail view
- Recording folder cache entries are deduplicated by normalized recording path for product views
- Recording list titles show local titles instead of repeated folder paths
- Recording detail titles are simplified and no longer repeat folder context
- Recording timestamp labels use `Aufnahme` instead of the ambiguous `Start`
- technical path, ID and size fields are hidden behind `Technische Details anzeigen`
- Recording action controls are hidden behind `Aktionen anzeigen`
- runtime verification proved single-recording navigation, deduplicated folder counts and polished detail cards in the browser

---

## Next Planned Implementation Slice

### Phase 60.15 - Recording Metadata and Poster Preparation

Status: Planned.

Goal:

- Prepare Recording metadata, artwork and poster handling on top of the now-polished lazy Recording browser.

Planned scope:

- define which Recording metadata fields belong in the Suite model
- distinguish VDR technical metadata from imported Rectools or scraper metadata
- prepare UI placeholders for poster and artwork data without requiring them immediately
- keep the existing lazy Recording folder flow stable while adding metadata hooks
- avoid coupling scraper-specific behavior directly into the Recording browser UI
- define the next backend/frontend contract for Recording metadata enrichment

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
