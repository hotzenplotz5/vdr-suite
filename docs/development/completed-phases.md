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
- preserve the detailed chronological phase history

Current status belongs to:

- [Current Project Status](current-status.md)

Future planning belongs to:

- [Roadmap](../planning/roadmap.md)
- [Milestones](../planning/milestones.md)

---

## Completed Milestones Overview

### Documentation Consolidation

Status: In Progress.

Purpose:
- Refresh the high-level project documentation before starting the next major milestone.
- Make completed foundations visible as milestones instead of only as individual phases.
- Align roadmap, dashboard and completed phase history.

Completed phases:
- Phase 46.38 - Roadmap and Milestone Refresh.
- Phase 46.39 - Project Status Dashboard Refresh.

Remaining planned documentation work:
- Completed phase milestone restructuring.
- Current status refresh.
- README refresh.

---

### Person Metadata Foundation

Status: Completed.

Purpose:
- Establish source-aware person metadata for actors, characters, directors, writers, producers, moderators, guests and composers.

Completed phases:
- Phase 46.13 - Person Domain Foundation.
- Phase 46.14 - Person Resolution Model.
- Phase 46.15 - Person JSON Contract.
- Phase 46.16 - Person REST Boundary.
- Phase 46.17 - Person API Documentation.
- Phase 46.18 - Person Query Model.
- Phase 46.19 - Person Query Matcher.
- Phase 46.20 - Person Query JSON Contract.
- Phase 46.21 - Person Search Service.
- Phase 46.22 - Person Query REST Boundary.
- Phase 46.23 - Person Query Router Wiring.
- Phase 46.24 - Person Query Documentation.

Key outcomes:
- Person domain model.
- Person roles and metadata sources.
- Person resolution result and JSON contract.
- Person REST boundary.
- Person query, matcher, service and JSON contract.
- Router-wired person query API.

---

### Recording Person Search Foundation

Status: Completed.

Purpose:
- Connect real recording metadata to person search while preserving recording context and backend identity.

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

Key outcomes:
- RESTfulAPI additional_media actor import.
- VdrRecording.persons metadata container usage.
- Recording-person search result model.
- Recording-person search service.
- JSON and REST API contracts.
- Snapshot-backed, backend-aware route wiring.
- Real yaVDR validation of actor metadata availability.

---

### Recording Character Search Foundation

Status: Completed.

Purpose:
- Extend recording-person search from actor names to played character names.

Completed phases:
- Phase 46.35 - Recording Character Search.
- Phase 46.36 - Recording Character Search API Documentation.

Key outcomes:
- characterName query filter.
- Case-insensitive partial matching against character names.
- API distinction between actor name and played character.
- Documentation of real TVScraper role-to-character mapping.

---

### EPG Person Search Foundation

Status: In Progress.

Purpose:
- Reuse person metadata search architecture for EPG events.

Completed phases:
- Phase 46.37 - EPG Person Search Result Model.

Planned next steps:
- EPG person search service.
- EPG person JSON contract.
- EPG person REST boundary.
- Real metadata validation.

---

## Detailed Phase History

## Phase 47.5 - SearchTimer in-memory service

Status: Completed.

Summary:
- Implements SearchTimerService::list with backend, state, text and pagination filtering over in-memory SearchTimer collections.

## Phase 47.4 - SearchTimer service interface

Status: Completed.

Summary:
- Adds SearchTimerService as the backend-neutral service boundary for listing SearchTimers from a supplied collection.

## Phase 47.3 - SearchTimer result model

Status: Completed.

Summary:
- Adds SearchTimerResult for backend-neutral SearchTimer listing with returned count, total count, limit and offset.

## Phase 47.2 - SearchTimer query model

Status: Completed.

Summary:
- Adds SearchTimerQuery with backend, state, text, limit and offset filters for backend-neutral SearchTimer listing.

## Phase 47.1 - SearchTimer domain model

Status: Completed.

Summary:
- Adds backend-aware SearchTimerId, SearchTimerState and SearchTimer as the first backend-neutral SearchTimer domain model.

## Phase 47.0 - Document backend-neutral SearchTimer architecture

Status: Completed.

Summary:
- Adds ADR-0029 for backend-neutral SearchTimer architecture and documents the RESTfulAPI epgsearch capability baseline.

## Phase 46.42 - README Refresh

Status: Completed.

Summary:
- Refreshes the repository README as the project entry point and aligns it with the current milestone-oriented documentation structure.

## Phase 46.41 - Current Project Status Refresh

Status: Completed.

Summary:
- Refreshes the current project status document to reflect completed person metadata, recording-person search, recording character search and the next SearchTimer milestone.

## Phase 46.40 - Completed Phase Milestone Overview

Status: Completed.

Summary:
- Adds a milestone-oriented overview to completed-phases.md while preserving the detailed phase history.

## Phase 46.39 - Project Status Dashboard Refresh

Status: Completed.

Summary:
- Refreshes the project status dashboard so current foundations and planned milestones are visible at a glance.
- Adds person metadata, recording person search, recording character search and EPG person search status to the dashboard.

## Phase 46.38 - Roadmap and Milestone Refresh

Status: Completed.

Summary:
- Replaces the minimal roadmap with a milestone-oriented roadmap covering completed foundations, current milestone, planned major milestones and long-term vision.

## Phase 46.37 - EPG Person Search Result Model

Status: Completed.

Summary:
- Adds the EPG person search result model that preserves matched person metadata together with EPG event and backend context.

## Phase 46.36 - Recording Character Search API Documentation

Status: Completed.

Summary:
- Documents recording character search through the characterName query parameter, including actor-versus-character semantics, examples, matching rules, and real VDR metadata implications.

## Phase 46.35 - Recording Character Search

Status: Completed.

Summary:
- Adds characterName filtering to person queries and routes it through recording-person search for character lookup over real recording actor metadata.

## Phase 46.34 - Real VDR Person Metadata Validation

Status: Completed.

Summary:
- Documents real yaVDR RESTfulAPI recording metadata validation: actors are available in additional_media, while director, writer and producer fields are not present in the inspected payload.

## Phase 46.33 - Recording Person Search API Documentation

Status: Completed.

Summary:
- Documents the snapshot-backed recording-person search API, query parameters, response shape, backend filter, and current source limitations.

## Phase 46.32 - Snapshot-backed Recording Person Search Wiring

Status: Completed.

Summary:
- Connects the routed recording-person search endpoint to snapshot-backed recordings, including optional backend filtering.

## Phase 46.31 - Recording Person Search Router Wiring

Status: Completed.

Summary:
- Wires recording-person search into ApiRouter and daemon runtime while keeping the routed collection empty until snapshot-backed search is connected.

## Phase 46.30 - Recording Person Search REST Boundary

Status: Completed.

Summary:
- Adds a REST-facing controller boundary for recording-person search without wiring it into ApiRouter yet.

## Phase 46.29 - Recording Person Search JSON Contract

Status: Completed.

Summary:
- Adds a JSON serializer for recording-person search results with recording context and matched person metadata.

## Phase 46.28 - Recording Person Search Service

Status: Completed.

Summary:
- Adds a generic recording person search service that matches person queries against recording-attached person metadata while preserving recording context.

## Phase 46.27 - Recording Person Search Result Model

Status: Completed.

Summary:
- Adds a recording-person search result model that preserves both matched person metadata and the recording context.

## Phase 46.26 - Recording Additional Media Person Import

Status: Completed.

Summary:
- Imports RESTfulAPI additional_media actors into VdrRecording person collections as scraper-sourced actor metadata.

## Phase 46.25 - Real VDR Person Metadata Validation

Status: Completed.

Summary:
- Documents real VDR person metadata findings and identifies RESTfulAPI additional_media actors and modern TVScraper characters as the validated import path.

## Phase 46.24 - Person Query Documentation

Status: Completed.

Summary:
- Documents the routed person query API, parameters, validation, response contract, current empty data source, and recording-search limitations.

## Phase 46.23 - Person Query Router Wiring

Status: Completed.

Summary:
- Wires person query routing through ApiRouter and DaemonRuntime with empty result data until real metadata sources are connected.

## Phase 46.22 - Person Query REST Boundary

Status: Completed.

Summary:
- Extends the person REST controller with validated person query parameters backed by PersonSearchService and PersonQueryResultJsonSerializer.

## Phase 46.21 - Person Search Service

Status: Completed.

Summary:
- Adds a person search service that filters person collections with PersonQueryMatcher and returns paged query results.

## Phase 46.20 - Person Query JSON Contract

Status: Completed.

Summary:
- Adds a person query result model and JSON serializer for paged person search results.

## Phase 46.19 - Person Query Matcher

Status: Completed.

Summary:
- Adds a person query matcher for optional name, normalized name, role, source, and provider reference filters.

## Phase 46.18 - Person Query Model

Status: Completed.

Summary:
- Adds the first person query domain model with optional filters for name, normalized name, role, source, and provider reference.

## Phase 46.17 - Person API Documentation

Status: Completed.

Summary:
- Added person API documentation.
- Documented the person JSON contract and REST-facing controller boundary.
- Clarified that ApiRouter wiring, provider integration, persistence and search remain out of scope.

## Phase 46.16 - Person REST Boundary

Status: Completed.

Summary:
- Added PersonController as the first REST-facing person metadata boundary.
- Returned person resolution results as ApiResponse.
- Preserved the existing JSON contract through PersonResolutionJsonSerializer.
- Added isolated controller coverage without routing or provider integration.

## Phase 46.15 - Person JSON Contract

Status: Completed.

Summary:
- Added PersonResolutionJsonSerializer.
- Serialized resolved state, primary person and all person evidence.
- Exposed source, role, original name, normalized name, character name, confidence and provider reference.
- Kept REST, provider integration and search out of scope.

## Phase 46.14 - Person Resolution Model

Status: Completed.

Summary:
- Added PersonResolver.
- Added PersonResolutionResult.
- Preserved all person evidence while selecting a primary person.
- Preferred confidence, manual user entries, provider references and deterministic source priority.
- Kept JSON, REST, provider integration and search out of scope.

## Phase 46.13 - Person Domain Foundation

Status: Completed.

Summary:
- Added Person as the first source-aware person metadata domain object.
- Added PersonRole for actor, director, writer, producer, moderator, guest, composer and generic roles.
- Added PersonCollection as a multi-evidence container for future cast and crew metadata.
- Kept resolver, JSON, REST, provider integration and search out of scope.

## Phase 46.12 - Content Rating API Documentation

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
