# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Phase Map](phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)
- [Phase 57 Local Server Permission Model](phase-57-local-server-permission-model.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Current Position

```text
Completed implementation state
Phase 56 - Library Boundary, Packaging and Developer Documentation

Current implementation focus
Phase 57 - Multi-Site Backend Administration and Permissions
```

---

## Purpose

This roadmap describes the current direction of VDR-Suite without duplicating the full phase history.

The compact source of truth for phase-range coverage is [Phase Map](phase-map.md).

The primary human entry point for the current repository state is [Current State](../CURRENT.md).

Detailed chronological implementation history belongs to [Completed Phases](../development/completed-phases.md).

Product parity and frontend gap planning belongs to [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md).

The first local/private server permission boundary for Phase 57 belongs to [Phase 57 Local Server Permission Model](phase-57-local-server-permission-model.md).

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
- Phase 55.6: Recording Operations Audit and Safety Policy.
- Phase 56: Library Boundary, Packaging and Developer Documentation.

---

## Recently Completed Milestone

### Phase 56 - Library Boundary, Packaging and Developer Documentation

Status: Completed.

Goal:
- Separate reusable library and module boundaries from daemon/application surfaces.
- Define the packaging and install layout contract before introducing real distribution packaging.
- Rebuild the documentation entry point so current state is easier to find than historical phase notes.

Completed outcomes:
- Documented the core/API boundary and kept the REST API as the application-facing boundary.
- Removed the transitional recording-action source aggregate and split source groups by responsibility.
- Added staged install support through `make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr`.
- Added initial daemon, config and CLI manpages and staged them through the install target.
- Documented install manifest ownership, package prerequisites and Phase 56 completion.
- Added `docs/CURRENT.md` as the primary current-state entry point.

---

## Planned Major Milestones

### Phase 57 - Multi-Site Backend Administration and Permissions

Status: Next.

Goal:
- Support multi-site operation with backend-specific permissions and read-only secondary-site mode.

Planning input:
- [Phase 57 Local Server Permission Model](phase-57-local-server-permission-model.md)

Planned outcomes:
- Model multiple VDR backend sites explicitly.
- Add backend administration surfaces without bypassing the existing adapter boundary.
- Represent read/write capabilities per backend.
- Keep secondary-site read-only access enforceable by policy.
- Prepare frontend-visible backend selection and permission state.

---

### Phase 58 - Frontend and Live Parity

Status: Planned.

Goal:
- Build frontend-ready everyday recording, timer, channel and EPG views after safe API and permission foundations.

Planning input:
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)

The frontend should first expose existing backend capability and reveal real gaps through vertical product slices.

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

- [Current State](../CURRENT.md) is the first human entry point for current repository truth.
- [Phase Map](phase-map.md) is the compact source of truth for phase-range coverage.
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md) records feature parity and frontend gap planning.
- [Phase 57 Local Server Permission Model](phase-57-local-server-permission-model.md) fixes the local/private server permission boundary before Phase 57.1 code changes.
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
