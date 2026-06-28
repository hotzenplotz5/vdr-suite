# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Current Position

```text
Completed implementation state
Phase 55.5n - Roadmap historical coverage alignment

Documentation consolidation step
Phase 55.5n - Roadmap historical coverage alignment

Next major implementation milestone
Phase 55.6 - Recording operations audit and safety policy
```

---

## Purpose

This roadmap describes the completed milestones, current milestone, planned major milestones and long-term vision of VDR-Suite.

It is intended to answer four questions:

- What is already implemented?
- Which completed phases belong to which milestone?
- What is currently in progress?
- What is planned next?

Detailed chronological implementation history belongs to [Completed Phases](../development/completed-phases.md).

Current build and project state belongs to [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).

---

## Roadmap Status Terms

| Status | Meaning |
| --- | --- |
| Completed | The milestone has an implemented and tested foundation in the repository. |
| In Progress | The milestone has begun and still needs further implementation before it can be considered a foundation. |
| Planned | The milestone is a concrete intended implementation direction. |
| Vision | The milestone is strategically important but intentionally deferred until earlier foundations exist. |

---

## Implementation Timeline Coverage

This section is a milestone coverage map. It is not the detailed chronological phase log.

Detailed phase-by-phase history remains in [Completed Phases](../development/completed-phases.md).

| Phase range | Roadmap coverage |
| --- | --- |
| Phase 1.x-7.x | Core platform, database, repositories, services, REST boundaries and daemon foundation. |
| Phase 8.x | VDR domain objects, adapter boundaries, mock/external backends and RESTfulAPI integration foundation. |
| Phase 9.x-29.x | Multi-backend runtime, backend-aware snapshots, snapshot cache, change feed, live transport and backend-aware API foundation. |
| Phase 30.x-36.x | Recording action validation and guarded execution foundation. |
| Phase 37.x-44.x | Recording action runtime completion, real-backend validation, regression coverage, action safety review and transition toward selective runtime/query hardening. |
| Phase 45.x | Selective EPG search foundation and EPG REST API surface. |
| Phase 46.x | Content classification, metadata domain foundations, person metadata, recording-person search and recording-character search. |
| Phase 47.x-49.x | SearchTimer backend foundation, RESTfulAPI compatibility, real VDR validation, Live/EPGSearch inventory, regex/fuzzy search semantics and native fuzzy capability validation. |
| Phase 50.0-50.50 | SearchTimer user workflow, dry-run/prepare/execute planning, safety gates, real-execution policy, mandatory readback verification and controlled test execution. |
| Phase 51.x | Live plugin parity discovery foundation. |
| Phase 52.x | SearchTimer automation read-only planning, dry-run models, preview endpoint, scheduling plan and safety review. |
| Phase 53.x | SearchTimer title-only REST/workflow preservation and completion audit. |
| Phase 54.x | SearchTimer preview runtime, mutation policy wiring and warm EPG cache architecture. |
| Phase 55.0-55.4e | Feature parity and adapter audit, RESTfulAPI SearchTimer contract fixes, JSON/form-body fixes, discovery provider/runtime wiring and daemon shutdown reset guardrail. |
| Phase 55.5a-55.5m | SearchTimer preview engine contract, native preview capability, real VDR acceptance manifest, acceptance expansion, daemon lifecycle hardening and documentation consolidation. |
| Phase 55.5n | Roadmap historical coverage alignment. |
| Phase 55.6 | Recording operations audit and safety policy. |

---

## Completed Milestones

### Core Platform Foundation

Status: Completed.

Primary result:
- Establishes the initial VDR-Suite platform foundation with database schema, repositories, services, dashboard aggregation, REST-facing boundaries and daemon-oriented runtime structure.

Representative phase range:
- Phase 1.x through Phase 7.x.

---

### VDR Backend Foundation

Status: Completed.

Primary result:
- Introduces VDR domain objects, adapter boundaries and RESTfulAPI integration foundations.

Representative phase range:
- Phase 8.x.

---

### Multi-Backend and Snapshot Runtime Foundation

Status: Completed.

Primary result:
- Turns the daemon into a snapshot-backed runtime that can represent one or more VDR backends.

Representative phase range:
- Phase 9.x through Phase 29.x.

---

### Live Runtime and Change Feed Foundation

Status: Completed.

Primary result:
- Adds transport-independent live update infrastructure based on snapshot change detection.

Representative phase range:
- Phase 8.x through Phase 29.x.

---

### Recording Query Foundation

Status: Completed.

Primary result:
- Adds structured recording query support over snapshot-backed recordings.

Representative phase range:
- Phase 29.x.

---

### Recording Action Foundation

Status: Completed.

Primary result:
- Adds validation and execution foundations for recording actions while preserving safety boundaries.

Representative phase range:
- Phase 30.x through Phase 36.x.

---

### EPG Search Foundation

Status: Completed.

Primary result:
- Adds selective EPG search over backend-neutral EPG query boundaries.

Representative phase range:
- Phase 45.x.

---

### Content Classification Foundation

Status: Completed.

Primary result:
- Establishes source-aware content classification as the architectural foundation for genre, ratings, keywords, tags and future policy.

Representative phase range:
- Phase 46.0 through Phase 46.15.

---

### Person Metadata Foundation

Status: Completed.

Primary result:
- Adds source-aware person metadata as a reusable foundation for actors, characters, directors, writers, producers, moderators, guests and composers.

Completed phases:
- Phase 46.16 - Person REST Boundary.
- Phase 46.17 - Person API Documentation.
- Phase 46.18 - Person Query Model.
- Phase 46.19 - Person Query Matcher.
- Phase 46.20 - Person Query JSON Contract.
- Phase 46.21 - Person Search Service.
- Phase 46.22 - Person Query REST Boundary.
- Phase 46.23 - Person Query Router Wiring.
- Phase 46.24 - Person Query Documentation.

---

### Recording Person Search Foundation

Status: Completed.

Primary result:
- Connects real recording metadata to person search and preserves both recording context and matched person metadata.

Key outcomes:
- RESTfulAPI additional_media actor import into VdrRecording.persons.
- Backend-aware recording-person search.
- Real yaVDR validation of TVScraper actor metadata.

Completed phases:
- Phase 46.25 - Real VDR Person Metadata Validation.
- Phase 46.26 - Recording Additional Media Person Import.
- Phase 46.27 - Recording Person Search Result Model.
- Phase 46.28 - Recording Person Search Service.
- Phase 46.29 - Recording Person Search JSON Contract.
- Phase 46.30 - Recording Person Search REST Boundary.
- Phase 46.31 - Recording Person Search Router Wiring.
- Phase 46.32 - Snapshot-backed Recording Person Search Wiring.
- Phase 46.33 - Recording Person Search API Documentation.
- Phase 46.34 - Real VDR Person Metadata Validation.

---

### Recording Character Search Foundation

Status: Completed.

Primary result:
- Extends recording-person search from actor names to played character names using real TVScraper metadata.

Completed phases:
- Phase 46.35 - Recording Character Search.
- Phase 46.36 - Recording Character Search API Documentation.

---

### SearchTimer Backend and Capability Foundation

Status: Completed.

Primary result:
- Establishes the backend-facing SearchTimer foundation and validates native fuzzy backend capability behavior through real VDR-Suite and RESTfulAPI workflows.

Representative phase range:
- Phase 47.x through Phase 49.x.

---

### SearchTimer User Workflow Foundation

Status: Completed.

Primary result:
- Turns the backend-facing SearchTimer and native fuzzy capability foundation into a practical, tested user workflow foundation.

Representative phase range:
- Phase 50.0 through Phase 50.50.

---

## Recently Completed Technical Milestones

### Real VDR Acceptance Foundation

Status: Completed.

Primary result:
- Adds a repeatable real VDR acceptance manifest and runner for safe and dry-run API validation against a live daemon.

Key outcomes:
- Safe and dry-run acceptance scope currently passes 20/20 probes.
- Acceptance route validation prevents stale manifest paths.
- Per-probe timeout support handles cold large-recording queries without hiding unrelated probe behavior.
- JSON report output records machine-readable acceptance evidence.

Representative phase range:
- Phase 55.5c through Phase 55.5k.

---

### Daemon Runtime Lifecycle Hardening

Status: Completed.

Primary result:
- Makes daemon startup and shutdown behavior operator-safe for the current simple HTTP listener runtime.

Key outcomes:
- Duplicate daemon starts on an occupied HTTP port exit with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9`.
- Port 18080 is released after normal shutdown.
- Guardrails cover HTTP bind failure handling and daemon runtime shutdown resets.

Representative phase range:
- Phase 55.4e and Phase 55.5l.

---

### Documentation Consolidation and Handoff Verification

Status: Completed.

Primary result:
- Aligns high-level project documentation with the verified Phase 55.5 runtime and acceptance state.

Key outcomes:
- README, roadmap, project state, current status, project dashboard, development index and completed-phase history now point at the same completed and next milestones.
- New-chat handoff rules preserve required local, real VDR and GitHub Actions checks.

Representative phase:
- Phase 55.5m.

---

## Current Milestone

### Phase 55.6 - Recording Operations Audit and Safety Policy

Status: Planned.

Goal:
- Audit live recording operations before enabling or expanding real write-capable paths.

Planned outcomes:
- Confirm which recording operations remain read-only, dry-run-only or destructive.
- Define operator confirmation, permission, backend allowlist and safety-policy requirements.
- Keep destructive real VDR probes explicitly opt-in.
- Preserve the current real VDR acceptance suite as safe/dry-run by default.
- Prepare the later recording operations implementation without silently opening mutation paths.

---

## Planned Major Milestones

### Phase 56 - Library Boundary and Packaging Strategy

Status: Planned.

Goal:
- Separate reusable suite libraries from daemon/application surfaces before the project grows more clients and packaging targets.

Expected outcomes:
- Clear `libs/` versus `apps/` ownership boundary.
- Packaging-oriented build layout without changing the current GNU Make foundation blindly.
- Stable internal API boundaries for daemon, tools and future frontend-facing services.
- Documentation of what remains application glue and what becomes reusable library code.

---

### Phase 57 - Multi-Site Backend Administration and Permissions

Status: Planned.

Goal:
- Support multi-site VDR operation with different permissions per backend, including a read-only secondary-site mode.

Expected outcomes:
- Backend administration API.
- Backend capability visibility and health diagnostics.
- Backend connection validation.
- Backend permission model.
- Read-only secondary backend mode.
- Client-visible backend selection and policy state.
- Preparation for frontend backend administration.

---

### Phase 58 - Frontend and Live Parity for Everyday Usage

Status: Planned.

Goal:
- Close the remaining everyday usability gaps between VDR-Suite and classic VDR frontends while keeping VDR-Suite API-first and multi-backend-ready.

Expected outcomes:
- Practical frontend-ready recording, timer, channel and EPG views.
- Read-only helper surfaces where mutation is not yet safe.
- Improved diagnostics for missing backend features.
- Frontend-ready backend and permission state.
- Web/desktop/mobile client foundations remain downstream of safe API and permission foundations.

---

### Phase 59 - Suite Metadata Database and External Scraper Provider Integration

Status: Planned.

Goal:
- Build a VDR-Suite-owned normalized metadata database while evaluating and using mature external scraper or catalog providers instead of reinventing scraper behavior by default.

Strategic rule:

```text
Use external scrapers and catalog providers when they solve metadata acquisition well.
Normalize useful results into the VDR-Suite metadata database.
Keep provider-specific behavior behind provider boundaries.
```

Expected outcomes:
- Recording metadata capability model.
- Suite-owned normalized metadata database.
- Backend-scoped metadata provider registry.
- Provider evaluation matrix for plugin-backed and external catalog-backed metadata.
- TVScraper provider boundary.
- scraper2vdr provider boundary.
- External catalog provider boundary for movie and TV metadata sources.
- EPG-only fallback provider.
- Normalized recording metadata aggregate.
- Cast, character, director, writer, guest, genre, rating, keyword and external-ID mapping.
- Artwork, poster and backdrop reference model.
- Metadata origin, evidence and confidence model.
- Read-only API for enriched recording metadata.
- Frontend-ready response contracts.

Related planning:
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0037: Suite Metadata Database and External Scraper Strategy](../adr/ADR-0037-suite-metadata-database-and-external-scraper-strategy.md)

---

### Phase 60 - Recommendation and Content Knowledge Graph Foundations

Status: Planned.

Goal:
- Build recommendation and content graph primitives on top of explicit metadata, search history, recording metadata and classification evidence.

Expected outcomes:
- Recommendation candidate model.
- Evidence-based ranking inputs.
- Profile-aware recommendation preparation.
- Explainable recommendation output.
- Relationship-based browsing across recordings, EPG events, people, characters, genres, ratings, backends and user profiles.

---

## Long-Term Vision

### Content Knowledge Graph

Status: Vision.

Goal:
- Connect recordings, EPG events, people, characters, genres, ratings, backends, search timers and user profiles into a structured metadata graph.

Potential outcomes:
- Relationship-based browsing.
- Actor-to-character-to-recording navigation.
- Genre and rating based filtering.
- Backend-aware availability views.
- Recommendation and discovery foundations.

This is intentionally deferred until SearchTimer, unified search, profiles and metadata enrichment are more mature.

---

## Roadmap Maintenance Rules

- This file describes milestones and direction.
- Detailed chronological implementation history belongs in [Completed Phases](../development/completed-phases.md).
- Project status snapshots belong in [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).
- When a major feature block is completed, it should be promoted into a completed milestone here.
- When a planned milestone becomes active, it should move into the current milestone section.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
