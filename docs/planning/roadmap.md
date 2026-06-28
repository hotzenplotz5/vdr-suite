# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Phase Map](phase-map.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Current Position

```text
Completed implementation state
Phase 55.5o - Phase map and roadmap simplification

Current implementation focus
Phase 55.6 - Recording operations audit and safety policy
```

---

## Purpose

This roadmap describes the current direction of VDR-Suite without duplicating the full phase history.

The compact source of truth for phase-range coverage is [Phase Map](phase-map.md).

Detailed chronological implementation history belongs to [Completed Phases](../development/completed-phases.md).

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
- Phase 55.0-55.4e: Adapter and Runtime Hardening.
- Phase 55.5a-55.5n: Acceptance and Documentation.
- Phase 55.5o: Phase Map and Roadmap.

---

## Current Milestone

### Phase 55.6 - Recording Operations Audit and Safety Policy

Status: Next.

Goal:
- Audit live recording operations before enabling or expanding real write-capable paths.

Planned outcomes:
- Confirm which recording operations remain read-only, dry-run-only or destructive.
- Define operator confirmation, permission, backend allowlist and safety-policy requirements.
- Keep destructive real VDR probes explicitly opt-in.
- Preserve the current real VDR acceptance suite as safe/dry-run by default.
- Prepare later recording operations implementation without silently opening mutation paths.

---

## Planned Major Milestones

### Phase 56 - Library Boundary and Packaging

Status: Planned.

Goal:
- Separate reusable libraries from daemon/application surfaces.

---

### Phase 57 - Multi-Site Backend Administration and Permissions

Status: Planned.

Goal:
- Support multi-site operation with backend-specific permissions and read-only secondary-site mode.

---

### Phase 58 - Frontend and Live Parity

Status: Planned.

Goal:
- Build frontend-ready everyday recording, timer, channel and EPG views after safe API and permission foundations.

---

### Phase 59 - Suite Metadata Database and External Providers

Status: Planned.

Goal:
- Build a suite-owned metadata database while using mature external scraper/catalog providers behind boundaries.

---

### Phase 60 - Recommendation and Content Knowledge Graph

Status: Vision.

Goal:
- Build recommendation and graph primitives after metadata and frontend foundations mature.

---


## Roadmap Maintenance Rules

- [Phase Map](phase-map.md) is the compact source of truth for phase-range coverage.
- This roadmap describes direction and should not duplicate the detailed completed phase log.
- Detailed chronological implementation history belongs in [Completed Phases](../development/completed-phases.md).
- Project status snapshots belong in [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).
- Planned phase numbers must not conflict with completed phase ranges.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
