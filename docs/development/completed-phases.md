# VDR-Suite Completed Phases

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)

---

## Purpose

This file tracks completed implementation phases.

It has two purposes:

- provide a milestone-oriented overview of completed work
- preserve the detailed chronological phase history for the current implementation line

Current status belongs to:

- [Current Project Status](current-status.md)

Future planning belongs to:

- [Roadmap](../planning/roadmap.md)
- [Milestones](../planning/milestones.md)

---

## Completed Milestones Overview

### Core Platform Foundation

Status: Completed.

Summary:
- Database, repository, service, dashboard, REST and daemon foundations are implemented.

### VDR Backend Foundation

Status: Completed.

Summary:
- VDR domain objects, adapter boundaries, RESTfulAPI mappers and VDR service boundaries are implemented.

### Multi-Backend and Snapshot Runtime Foundation

Status: Completed.

Summary:
- Backend registry, backend-aware snapshots, snapshot cache and backend-aware polling foundations are implemented.

### Live Runtime and Change Feed Foundation

Status: Completed.

Summary:
- Snapshot change feed, live update event model and SSE transport foundation are implemented.

### Query, Action and Metadata Foundations

Status: Completed.

Summary:
- EPG query/search, recording query/action, content classification, person metadata, recording-person search and recording-character search foundations are implemented.

### SearchTimer Backend and User Workflow Foundations

Status: Completed.

Summary:
- SearchTimer backend route/provider, real payload validation, native fuzzy capability validation and manual workflow foundation are implemented.

### SearchTimer Runtime Mutation Policy Foundation

Status: Completed.

Summary:
- Runtime SearchTimer mutation services exist but remain guarded by explicit policy boundaries.

---

## Phase 54.1 - SearchTimer preview comparison-option fix

Status: Completed.

Summary:
- Fixed SearchTimerPreviewService so preview respects compareTitle, compareSubtitle and compareSummary.
- Fixed preview limit and offset behavior.
- Added targeted test coverage for title-only, subtitle-only, summary-only, limit and offset behavior.
- Verified against real yaVDR EPG input that title-only "Amerika" matching returns the expected SearchTimer preview result.
- Kept backend mutation out of scope.

---

## Phase 54.2 - SearchTimer warm EPG cache architecture

Status: Completed.

Summary:
- Added ADR-0034 for SearchTimer warm EPG cache and change invalidation.
- Documented the measured RESTfulAPI full EPG dump cost of about 30 MB and about 28 seconds for a 14-day EPG window.
- Decided that interactive SearchTimer preview must not fetch the full RESTfulAPI EPG dump per request.
- Defined backend-scoped warm EPG cache readiness, stale-state and future SSE invalidation rules.
- Updated roadmap, project status, development index and dashboard documentation to make Phase 54.3 the warm EPG cache implementation step.

---

## Earlier Phase Groups

The repository history and phase-specific development documents retain detailed context for earlier work.

Important earlier phase groups:

- Phase 1.x through Phase 7.x - Core platform, database, service, REST and daemon foundations.
- Phase 8.x through Phase 29.x - VDR backend, runtime, snapshot, change feed, live transport and multi-backend foundations.
- Phase 30.x through Phase 36.x - Recording action validation and execution foundations.
- Phase 45.x - EPG search foundation.
- Phase 46.x - Content classification, person metadata, recording-person and recording-character foundations.
- Phase 47.x through Phase 49.x - SearchTimer backend foundation and native fuzzy capability validation.
- Phase 50.x - SearchTimer user workflow, validation, planning, guarded execution and readback verification foundations.
- Phase 51.x - Live plugin parity discovery foundation.
- Phase 52.x - SearchTimer automation preview-only foundation.
- Phase 53.x - SearchTimer title-only workflow chain preservation and completion audit.
- Phase 54.0 - SearchTimer runtime mutation policy wiring.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
