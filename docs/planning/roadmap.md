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

## Roadmap Decision Summary

The broad architecture source audit is complete enough to stop auditing plugins without a concrete implementation question.

The current roadmap order is:

1. complete the second architecture decision package with ADR-0042 through ADR-0045;
2. update the affected architecture diagrams and turn the accepted ADRs into an explicit implementation dependency map;
3. implement Phase 60.15 as a bounded Recording metadata preparation slice;
4. start Phase 61 only after the suite-owned metadata identities, provenance and artwork contracts are stable;
5. assign later runtime phases for Backend Agents, secure multi-site transport and timer orchestration only after their ADR dependencies are complete;
6. keep Phase 62 as a later product vision, not as near-term implementation work.

This order prevents the frontend, metadata store, remote Agents and mutation workflows from defining incompatible identities or transport assumptions independently.

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

## Immediate Architecture Decision Package

The next repository work should complete the following ADR package before Phase 60.15 begins:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
```

Required outcomes:

- one explicit revision and idempotency vocabulary for all guarded mutations;
- one asynchronous execution model for claims, retries, cancellation, compensation and stale-worker fencing;
- separation of user timer intent, backend assignment and backend-native timer state;
- a canonical EPG event identity that includes backend, channel, time and provider provenance;
- clarified dependencies between Recording metadata, timer automation, remote Agents and change feeds;
- updated architecture diagrams after the decisions are accepted;
- a concrete implementation dependency map before new runtime phase numbers are assigned.

These ADRs define contracts first. They do not by themselves complete runtime behavior.

---

## Next Planned Implementation Slice

### Phase 60.15 - Recording Metadata and Poster Preparation

Status: Planned after the immediate ADR package and diagram update.

Goal:

- Prepare Recording metadata, artwork and poster handling on top of the polished lazy Recording browser without prematurely implementing the full Phase 61 metadata platform.

Planned scope:

- define which Recording metadata fields belong in the suite model
- distinguish VDR technical metadata from provider, sidecar and imported metadata
- define stable provider-neutral artwork references instead of exposing local file paths
- prepare UI placeholders for poster and artwork data without requiring provider data immediately
- keep the existing lazy Recording folder flow stable while adding metadata hooks
- avoid coupling scraper-specific behavior directly into the Recording browser UI
- define the next backend/frontend contract for Recording metadata enrichment
- align the implementation with ADR-0038 provider, provenance and artwork boundaries
- keep provider refresh, enrichment jobs and external lookup latency outside synchronous Recording list rendering

Entry conditions:

- ADR-0038 remains the accepted metadata ownership boundary
- ADR-0042 and ADR-0043 define the mutation and asynchronous execution vocabulary needed by later enrichment jobs
- canonical identity decisions do not require Recording browser data to be rewritten again immediately
- no direct dependency on TVScraper, scraper2vdr or an external provider database is introduced into the frontend

Exit criteria:

- the Recording API can represent technical, normalized and provider-derived metadata as separate concerns
- artwork is referenced through suite-owned asset identity or an explicitly temporary placeholder contract
- provenance and provider identity can be added without breaking the Recording browser contract
- the frontend remains functional when no enriched metadata or artwork is available
- the lazy folder and Recording detail flows remain regression-tested

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

Broad plugin auditing is now considered complete for roadmap purposes. Further source audits should be targeted to a concrete feature, adapter or risk and should produce an explicit implementation decision.

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

Status: Planned major milestone after Phase 60.15.

Goal:

- Build a suite-owned normalized metadata database while using external scraper, catalog, sidecar and plugin-backed providers behind explicit boundaries.

Architecture basis:

- ADR-0025
- ADR-0036
- ADR-0038
- ADR-0042
- ADR-0043
- ADR-0045

Expected areas:

- canonical metadata entities and assignments
- provider provenance, evidence and confidence
- suite-owned artwork asset service
- backend-neutral Recording enrichment
- sidecar and imported metadata adapters
- provider refresh and enrichment jobs
- cache invalidation and stale-data handling
- EPG-only fallback behavior
- readback and observability for enrichment results

Provisional implementation order without assigned subphase numbers:

1. metadata identity and schema foundation;
2. provider and provenance contracts;
3. artwork asset storage and delivery;
4. Recording enrichment read model;
5. asynchronous provider refresh and retry behavior;
6. frontend integration beyond Phase 60.15 placeholders;
7. migration, invalidation and operational hardening.

Phase 61 must not make any external provider or plugin database the authoritative VDR-Suite data model.

---

### Multi-Site Agent and Secure Transport Implementation

Status: Architecture accepted, runtime milestone number not yet assigned.

Architecture basis:

- ADR-0039
- ADR-0040
- ADR-0041
- ADR-0042
- ADR-0043

Expected areas:

- Backend Agent enrollment and identity
- authenticated Control Plane to Agent sessions
- backend generation, lease and heartbeat handling
- capability and health publication
- server-enforced read-only and permission propagation
- safe command dispatch with revision, idempotency and fencing
- remote-site observability and audit events
- protected transport without exposing VDR plugin ports publicly

This implementation track is required before remote write operations are considered production-ready. A concrete phase number is assigned only after the dependency map and transport contract are complete.

---

### Timer Intent and Multi-Backend Orchestration

Status: Architecture decision pending ADR-0044.

Expected areas:

- user-visible timer intent separated from backend-native timer records
- backend assignment and reassignment rules
- conflict detection and capability-aware planning
- stale assignment and backend generation handling
- readback verification after native timer mutation
- explicit ownership and audit trail across local and remote VDR backends

No production orchestration phase begins before ADR-0044 is accepted.

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
- mature Phase 61 enrichment and invalidation behavior

Phase 62 is intentionally not part of the immediate implementation sequence.

---

## Cross-Cutting Decision Gates

### Identity Gate

Before a domain is persisted or synchronized across backends, its stable suite identity and backend-native identity must be explicit.

Applies to:

- recordings
- EPG events
- timers and timer intents
- metadata entities and assignments
- artwork assets
- Backend Agents and backend generations

### Mutation Gate

No new real mutation path is considered complete without:

- server-side authorization
- capability validation
- revision or equivalent stale-state protection
- idempotency behavior
- readback verification
- audit-visible outcome

### Asynchronous Work Gate

Slow provider, filesystem, network or cross-site work must not execute inside synchronous list rendering or long-held VDR locks.

Such work belongs in the job model defined by ADR-0043.

### Remote-Site Gate

Remote sites must communicate through authenticated Agent boundaries. Public VDR plugin ports, implicit LAN trust and direct database coupling are not accepted deployment architecture.

### Frontend Gate

The frontend consumes suite domain contracts and asset identities. It must not become the owner of scraper-specific rules, local filesystem paths or backend-native identifiers.

---

## Later Architecture Topics

Later ADR packages may cover:

- Streaming Gateway and media sessions
- Legacy OSD compatibility bridge
- public API versioning and error contracts
- audit and security event model
- metadata merge and conflict resolution policy
- backup, migration and disaster recovery for suite-owned state

No runtime phase number is assigned to these items until the ADR and dependency order is stable.

Further plugin or source audits are only added when one of these concrete topics requires source evidence that is not already available.

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
- Architecture dependencies should be settled before assigning new runtime phase numbers.
- Broad source audits should not continue without a concrete product, adapter or risk question.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Project Overview](../project-overview.md)
