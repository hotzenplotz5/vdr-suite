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

Current umbrella implementation track
Phase 58 - Frontend and Live Parity

Latest completed implementation slice
Phase 60.14k - Recording Detail UX Polish

Next planned implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation
```

Current accepted architecture package:

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
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
- Phase 58.0-58.90b: Frontend and Live-parity foundation slices.
- Phase 59.00-59.15e: Client API consolidation and frontend module boundaries.
- Phase 60.1-60.14k: Frontend platform, lazy Recording cache and Recording detail UX.

Phase 58 remains the broad frontend and Live-parity product track. The already-used 59.x and 60.x ranges are implementation slices under the continued frontend platform work and are not reused for future major milestones.

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

- Prepare Recording metadata, artwork and poster handling on top of the polished lazy Recording browser.

Planned scope:

- define which Recording metadata fields belong in the suite model
- distinguish VDR technical metadata from provider, sidecar and imported metadata
- prepare UI placeholders for poster and artwork data without requiring them immediately
- keep the existing lazy Recording folder flow stable while adding metadata hooks
- avoid coupling scraper-specific behavior directly into the Recording browser UI
- define the next backend/frontend contract for Recording metadata enrichment
- align the implementation with ADR-0038 provider, provenance and artwork boundaries

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

This milestone provides a server-enforced read-only foundation. It is not yet full user and role RBAC.

---

## Current Architecture Follow-Up

The source audits of VDR Core, epgsearch, Live, RESTfulAPI, Streamdev, TVScraper, scraper2vdr, osd2web, epg2vdr and epgd produced the first accepted architecture package.

### ADR-0038 - Suite Metadata Database and External Provider Strategy

- suite-owned normalized metadata database
- multiple provider inputs
- provenance, evidence and confidence
- suite-owned artwork asset identities

### ADR-0039 - Backend Agent and Control Plane Boundary

- Control Plane owns central orchestration and public APIs
- Backend Agents own local native adapters and execution
- no direct central database protocol for agents
- remote sites use an Agent boundary instead of public VDR plugin ports

### ADR-0040 - Backend Lifecycle, Generation, Lease and Health

- stable backend identity
- per-runtime backend generation
- leases and heartbeats
- explicit lifecycle and health state
- fencing of stale Agent commands and results

### ADR-0041 - Authentication, Agent Trust and Multi-Site Transport

- separate client and Agent authentication
- explicit Agent enrollment and revocation
- protected remote-site transport
- no implicit trust based only on LAN or VPN reachability

These ADRs define architecture direction. They do not mark the corresponding runtime implementation as complete.

---

## Planned Major Milestones

### Phase 61 - Suite Metadata Database and External Providers

Status: Planned.

Goal:

- Build a suite-owned normalized metadata database while using external scraper, catalog, sidecar and plugin-backed providers behind explicit boundaries.

Architecture basis:

- ADR-0025
- ADR-0036
- ADR-0038

Expected areas:

- metadata entities and assignments
- provider provenance and confidence
- artwork asset service
- backend-neutral Recording enrichment
- cache invalidation and provider refresh
- EPG-only fallback behavior

---

### Phase 62 - Recommendation and Content Knowledge Graph

Status: Vision.

Goal:

- Build recommendation and graph primitives after metadata and frontend foundations mature.

Prerequisites:

- stable suite metadata identities
- provider provenance
- people and character relationships
- cross-backend Recording metadata
- explainable recommendation evidence

---

## Later Architecture Milestones

The next ADR package is expected to define:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
```

Later packages may cover:

- Streaming Gateway and media sessions
- Legacy OSD compatibility bridge
- public API versioning and error contracts
- audit and security event model

No runtime phase number is assigned to these items until the ADR and dependency order is stable.

---

## Roadmap Maintenance Rules

- [Current State](../CURRENT.md) is the first human entry point for current repository truth.
- [Phase Map](phase-map.md) is the compact source of truth for phase-range coverage.
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md) records feature parity and frontend gap planning.
- This roadmap describes direction and should not duplicate the detailed completed phase log.
- Detailed chronological implementation history belongs in [Completed Phases](../development/completed-phases.md).
- Project status snapshots belong in [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).
- Planned phase numbers must not conflict with completed phase or implementation-slice ranges.
- Completed phase history must not be renumbered to free future planning numbers.
- Accepted ADRs define architecture direction but do not imply completed implementation.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Project Overview](../project-overview.md)
