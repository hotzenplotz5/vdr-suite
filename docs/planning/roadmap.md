# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)

---

## Current Position

```text
Completed implementation state
Phase 47.57 - SearchTimer series recording enrichment

Documentation consolidation step
Phase 46.38 - Roadmap and Milestone Refresh

Next major implementation milestone
Phase 47.58 - SearchTimer blacklist enrichment
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

## Completed Milestones

### Core Platform Foundation

Status: Completed.

Primary result:
- Establishes the initial VDR-Suite platform foundation with database schema, repositories, services, dashboard aggregation, REST-facing boundaries and daemon-oriented runtime structure.

Key outcomes:
- SQLite schema and repository foundations.
- Recording and job domain foundations.
- Dashboard services and JSON serialization.
- REST and daemon foundations.
- Documentation and phase tracking conventions.

Representative phase range:
- Phase 1.x through Phase 7.x.

---

### VDR Backend Foundation

Status: Completed.

Primary result:
- Introduces VDR domain objects, adapter boundaries and RESTfulAPI integration foundations.

Key outcomes:
- VDR status, channels, events, timers and recordings as domain models.
- IVdrAdapter and adapter factory foundation.
- RESTfulAPI mapper layer.
- VdrService and overview service.
- Backend-neutral controller boundaries.

Representative phase range:
- Phase 8.x.

---

### Multi-Backend and Snapshot Runtime Foundation

Status: Completed.

Primary result:
- Turns the daemon into a snapshot-backed runtime that can represent one or more VDR backends.

Key outcomes:
- Snapshot cache and snapshot read boundaries.
- Backend registry foundation.
- Backend-aware snapshots.
- Multi-backend polling preparation.
- Backend identity propagation into domain data.
- Backend-aware read APIs.

Representative phase range:
- Phase 9.x through Phase 29.x.

---

### Live Runtime and Change Feed Foundation

Status: Completed.

Primary result:
- Adds transport-independent live update infrastructure based on snapshot change detection.

Key outcomes:
- Change detection service.
- Snapshot change feed.
- Live update event model.
- Server-sent-events transport foundation.
- Live transport service and controller boundaries.

Representative phase range:
- Phase 8.x through Phase 29.x.

---

### Recording Query Foundation

Status: Completed.

Primary result:
- Adds structured recording query support over snapshot-backed recordings.

Key outcomes:
- Recording query model.
- Recording matcher and service.
- JSON result contract.
- REST controller and router wiring.
- Backend-aware recording query filtering.

Representative phase range:
- Phase 29.x.

---

### Recording Action Foundation

Status: Completed.

Primary result:
- Adds validation and execution foundations for recording actions while preserving safety boundaries.

Key outcomes:
- Recording action validation request parsing.
- Safety validation and execution result contracts.
- Recording action REST boundary.
- Timer action execution groundwork.
- Runtime diagnostics integration.

Representative phase range:
- Phase 30.x through Phase 36.x.

---

### EPG Search Foundation

Status: Completed.

Primary result:
- Adds selective EPG search over backend-neutral EPG query boundaries.

Key outcomes:
- EPG search request model.
- EPG matcher.
- EPG search result model.
- JSON result contract.
- Service and controller boundaries.
- REST validation and documentation.

Representative phase range:
- Phase 45.x.

---

### Content Classification Foundation

Status: Completed.

Primary result:
- Establishes source-aware content classification as the architectural foundation for genre, ratings, keywords, tags and future policy.

Key outcomes:
- ADR-0028 content classification architecture.
- Genre domain foundation.
- Genre resolution and evidence handling.
- Genre JSON contracts.
- Canonical genre registry and localization foundation.
- Content rating groundwork.

Representative phase range:
- Phase 46.0 through Phase 46.15.

---

### Person Metadata Foundation

Status: Completed.

Primary result:
- Adds source-aware person metadata as a reusable foundation for actors, characters, directors, writers, producers, moderators, guests and composers.

Key outcomes:
- Person domain model.
- Person roles and metadata sources.
- Person resolution result and JSON contract.
- Person REST boundary.
- Person query model.
- Person query matcher.
- Person search service.
- Person query JSON contract.
- Person query REST boundary and router wiring.

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
- Recording-person result model.
- Recording-person search service.
- Recording-person JSON contract.
- REST controller boundary.
- Router and daemon wiring.
- Snapshot-backed search over current recordings.
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

Key outcomes:
- characterName query support.
- Case-insensitive partial matching against person character names.
- Actor-versus-character API semantics.
- Real VDR validation that RESTfulAPI actor role data maps to Person.characterName.
- API documentation with example character searches.

Completed phases:
- Phase 46.35 - Recording Character Search.
- Phase 46.36 - Recording Character Search API Documentation.

---

## Current Milestone

### EPG Person Search Foundation

Status: In Progress.

Goal:
- Reuse the person metadata search architecture for EPG events.

Current result:
- The first EPG person search result model exists and preserves matched person metadata together with EPG event and backend context.

Completed phases:
- Phase 46.37 - EPG Person Search Result Model.

Planned next implementation steps:
- EPG person search service.
- EPG person search JSON contract.
- EPG person search REST boundary.
- Snapshot-backed or query-backed EPG person search routing.
- Documentation and validation against real EPG metadata availability.

Open design questions:
- Whether real EPG person metadata is available from current adapters.
- Whether EPG person search should initially use structured metadata only or derive candidates from event descriptions.
- Whether derived person extraction belongs in VDR-Suite core or a future metadata enrichment layer.

---

## Planned Major Milestones

### Phase 47 - SearchTimer Foundation

Status: Planned.

Goal:
- Add persistent search rules that can evaluate EPG data and later trigger recording-oriented actions.

Expected outcomes:
- SearchTimer domain model.
- SearchTimer repository and persistence.
- SearchTimer query and matching engine.
- REST API for creating, listing, updating and deleting search timers.
- Validation contracts and tests.
- Foundation for epgsearch-style workflows.

Reason for priority:
- SearchTimer is the next major step after EPG search, recording query and person search because it turns search from a manual API into an automation foundation.

---

### Phase 48 - Unified Search Foundation

Status: Planned.

Goal:
- Provide one search surface across recordings, EPG events, people, characters, genres and backend identity.

Expected outcomes:
- Unified search request model.
- Unified search result model.
- Search domains for recordings, EPG, people, characters and classifications.
- Backend-aware query behavior.
- API contract suitable for future web and TV clients.

---

### Phase 49 - Profiles, Permissions and Policy Foundation

Status: Planned.

Goal:
- Add user-facing policy foundations for multi-backend and multi-client use.

Expected outcomes:
- Profile domain model.
- Backend permission model.
- Read-only and full-control backend policy.
- Content rating / FSK policy integration.
- Foundation for user preferences and parental controls.

Reason for priority:
- Multi-backend support already exists technically, but user-level policy is required before exposing destructive operations broadly.

---

### Phase 50 - Backend Management Foundation

Status: Planned.

Goal:
- Turn backend configuration from a static foundation into a manageable runtime concept.

Expected outcomes:
- Backend management API.
- Backend capability visibility.
- Backend health and diagnostics.
- Backend connection validation.
- Preparation for frontend backend administration.

---

### Phase 51 - Live Plugin Parity Foundation

Status: Planned.

Goal:
- Approach the practical information quality of the VDR Live plugin while keeping VDR-Suite multi-backend and API-first.

Expected outcomes:
- Broader EPG detail coverage.
- Timer creation workflows.
- SearchTimer workflows.
- Recording metadata visibility.
- Backend-aware frontend-ready API surfaces.

---

### Phase 52 - SearchTimer Automation Foundation

Status: Planned.

Goal:
- Extend SearchTimer from stored search rules to automated evaluation and action preparation.

Expected outcomes:
- Scheduled SearchTimer evaluation.
- Match history.
- Duplicate detection.
- Candidate timer creation.
- Safe execution and validation hooks.

---

### Phase 53 - Recommendation Foundation

Status: Planned.

Goal:
- Build recommendation primitives on top of explicit metadata, search history, recording metadata and classification evidence.

Expected outcomes:
- Recommendation candidate model.
- Evidence-based ranking inputs.
- Profile-aware recommendation preparation.
- Explainable recommendation output.

---

### Phase 54 - Cross-Backend Search and Federation

Status: Planned.

Goal:
- Make multi-backend search first-class across recordings, EPG and metadata.

Expected outcomes:
- Cross-backend result aggregation.
- Backend-specific result context.
- Conflict and duplicate handling.
- Remote backend visibility rules.

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
