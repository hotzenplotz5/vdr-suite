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

## Phase 47.12 - SearchTimer route data source wiring

Status: Completed.

Summary:
- Added SearchTimer route data source wiring.
- Introduced a backend-neutral SearchTimer data source boundary.
- Changed the router path away from an empty SearchTimerResult placeholder.
- Verified SearchTimer route behavior through API router tests.

---

## Phase 47.13 - SearchTimer daemon backend provider

Status: Completed.

Summary:
- Added daemon-side SearchTimer backend provider wiring.
- Attached RestfulApiSearchTimerAdapter to BackendRuntimeContext.
- Created SearchTimerService, SearchTimerResultJsonSerializer and SearchTimerController in DaemonRuntime.
- Linked SearchTimer runtime sources into the daemon build.

---

## Phase 47.14 - SearchTimer backend contract documentation

Status: Completed.

Summary:
- Documented the current SearchTimer backend contract.
- Captured the implemented route, controller, data-source and RESTfulAPI adapter chain.
- Defined the current minimal SearchTimer domain model.
- Listed known epgsearch and Live-style SearchTimer expansion gaps.
- Deferred domain expansion until real backend payload validation.

---
## Phase 47.15 - SearchTimer real payload validation

Status: Completed.

Summary:
- Added a local SearchTimer payload capture helper.
- Documented the real payload validation workflow.
- Kept SearchTimer domain expansion gated behind real backend payload review.
- Linked SearchTimer payload validation from the development index.

---
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

## Phase 49.2 - EPGSearch service test consolidation

Status: Completed.

Summary:
- Consolidated active EPGSearch service coverage around `EpgSearchQuery`.
- Added channel interval and duration-window service coverage.
- Covered backend-scoped query metadata without asserting match backend propagation.
- Kept paging and sorting out of the domain service test because they are API/request concerns.
- Moved the next focus to legacy EPGSearch test retirement.

---
## Phase 49.1 - EPGSearch matcher test consolidation

Status: Completed.

Summary:
- Ran the old underscore-style matcher test and confirmed it no longer matches the current architecture.
- Identified that `EpgSearchMatcher` now uses `EpgSearchQuery`, not `EpgSearchRequest`.
- Migrated the no-search-field regression into the active compact matcher test.
- Kept fuzzy matching out of scope because fuzzy semantics are modeled but not implemented yet.
- Moved the next focus to service test consolidation.

---
## Phase 49.0 - EPGSearch test coverage audit

Status: Completed.

Summary:
- Audited the EPGSearch test landscape after the request-to-query mapper.
- Identified old underscore-style EPGSearch tests and newer compact epgsearch-style tests.
- Confirmed that endpoint regression should wait until test consolidation avoids duplicate coverage.
- Moved the next focus to EPGSearch test consolidation.

---
## Phase 48.9 - EPGSearch request-to-query mapper

Status: Completed.

Summary:
- Added `EpgSearchRequestMapper`.
- Mapped API-facing `EpgSearchRequest` into backend-neutral `EpgSearchQuery`.
- Mapped query text, backend id, single-channel scope and field selection.
- Updated `EpgController::search(...)` so `EpgSearchService` receives the domain query.
- Kept paging, sorting and request-window metadata outside the domain query.

---
## Phase 48.8 - EPGSearch query alignment audit

Status: Completed.

Summary:
- Audited the existing EPGSearch controller and router integration.
- Confirmed that `GET /api/epg/search` and `EpgController::search(...)` already exist.
- Clarified the boundary between API-facing `EpgSearchRequest` and backend-neutral `EpgSearchQuery`.
- Moved the next focus to an explicit request-to-query mapper.

---
## Phase 48.7 - EPGSearch result serializer

Status: Completed.

Summary:
- Added `EpgSearchResultJsonSerializer`.
- Serialized result metadata, matches, backend identity, matched fields and nested event details.
- Added focused serializer tests including JSON escaping and content descriptor arrays.
- Moved the next focus to the EPGSearch controller.

---
## Phase 48.6 - EPGSearch matcher filter expansion

Status: Completed.

Summary:
- Expanded `EpgSearchMatcher` with backend-neutral filters supported by current `VdrEvent` fields.
- Added channel interval filtering, duration-window filtering and content-descriptor filtering.
- Kept extended EPG categories, channel groups, favorites, time windows, day-of-week and fuzzy semantics out of scope until their backend semantics are audited.
- Moved the next focus to EPGSearch result serialization.

---
## Phase 48.5 - EPGSearch matcher extraction

Status: Completed.

Summary:
- Extracted EPGSearch text matching from `EpgSearchService` into `EpgSearchMatcher`.
- Added focused matcher tests for title, subtitle, description and case sensitivity.
- Kept `EpgSearchService` as a thin orchestration layer over event lists and query results.
- Moved the next focus to matcher filter expansion.

---
## Phase 48.4 - EPGSearch service interface

Status: Completed.

Summary:
- Added a backend-neutral `EpgSearchService` boundary.
- Implemented `search(events, query) -> EpgSearchResult` for in-memory event lists.
- Covered basic text matching, field selection and case-sensitive matching.
- Kept the phase intentionally free of REST endpoints, adapters and real backend execution.

---
## Phase 48.3 - EPGSearch result model audit

Status: Completed.

Summary:
- Audited the existing EPGSearch result domain model.
- Confirmed that `EpgSearchMatch` already wraps `VdrEvent`, backend identity and matched fields.
- Confirmed that `EpgSearchResult` already provides matches, total count, returned count, limit and offset.
- Avoided duplicate model creation and moved the next focus to a backend-neutral EPGSearch service interface.

---
## Phase 48.2 - Backend-neutral EPGSearch query model

Status: Completed.

Summary:
- Added a backend-neutral EPGSearch query model.
- Covered search modes, fuzzy tolerance, field selection, channel scopes, time/duration/day filters, extended EPG info, content descriptors and favorites-only scope.
- Added a focused unit test for the query model.
- Kept the phase intentionally domain-only with no REST endpoint or adapter execution.

---
## Phase 48.1 - EPGSearch capability matrix

Status: Completed.

Summary:
- Mapped Live/EPGSearch service capabilities to RESTfulAPI and VDR-Suite capability areas.
- Identified high-priority gaps: EPGSearch query semantics, extended EPG categories, channel groups, blacklists, SearchTimer query preview and timer conflict reporting.
- Confirmed that SearchTimer CRUD is already strong and real-VDR tested.
- Moved the next focus to a backend-neutral EPGSearch query model.

---
## Phase 48.0 - Live / EPGSearch feature inventory

Status: Completed.

Summary:
- Started the Live goldstandard analysis after completing the real VDR regression foundation.
- Inventoried EPGSearch service capabilities used by Live, including search, SearchTimer CRUD, query preview, extended EPG categories, channel groups, blacklists, directories, timer conflicts and expression evaluation.
- Identified the highest-value remaining gaps after Phase 47: EPGSearch semantics, extended EPG metadata, conflict visibility and SearchTimer query preview parity.
- Moved the next focus to an EPGSearch capability matrix.

---
## Phase 48.0 - Live / EPGSearch feature inventory

Status: Completed.

Summary:
- Started the Live goldstandard analysis after completing the real VDR regression foundation.
- Inventoried EPGSearch service capabilities used by Live, including search, SearchTimer CRUD, query preview, extended EPG categories, channel groups, blacklists, directories, timer conflicts and expression evaluation.
- Identified the highest-value remaining gaps after Phase 47: EPGSearch semantics, extended EPG metadata, conflict visibility and SearchTimer query preview parity.
- Moved the next focus to an EPGSearch capability matrix.

---
## Phase 47.71 - Unified real VDR regression command

Status: Completed.

Summary:
- Added `make real-vdr-regression` as a unified real VDR regression command.
- The command builds and runs read-only regression, SearchTimer real smoke and Timer lifecycle real smoke.
- Required `VDR_SUITE_TIMER_CHANNEL` to avoid unsafe or invalid timer lifecycle runs.
- Kept recording move/delete helpers excluded from the unified command until a safe test-recording fixture exists.

---
## Phase 47.70 - Harden real recording action smoke helpers

Status: Completed.

Summary:
- Hardened real recording move/delete smoke helpers with VDR-SUITE-TEST marker enforcement.
- Added /recordings.json readback checks before destructive execution.
- Added /recordings.json readback checks after execution to verify source disappearance and target presence.
- Kept explicit --execute plus VDR_SUITE_ALLOW_REAL_RECORDING_ACTION=YES gates for real mutations.
- Moved the next focus to a unified real VDR regression command.

---
## Phase 47.69 - Real recording action regression audit

Status: Completed.

Summary:
- Audited existing real VDR recording action smoke helpers for move and delete.
- Confirmed that both helpers already have preview-only defaults and explicit real-execution safety gates.
- Identified missing regression checks for source disappearance, target appearance and marker-restricted execution.
- Moved the next focus to hardening the real recording action smoke helpers before including them in a unified real VDR regression suite.

---
## Phase 47.68 - Add real VDR read-only regression helper

Status: Completed.

Summary:
- Added a read-only real VDR regression helper for core RESTfulAPI endpoints.
- The helper verifies info, status, channels, events, recordings, timers and searchtimers without modifying the VDR.
- Added a Makefile target for building and help-checking the read-only real VDR regression helper.

---
## Phase 47.67 - Real VDR Timer lifecycle validation

Status: Completed.

Summary:
- Added a real VDR Timer lifecycle smoke helper for create, readback, update, delete and delete verification.
- The helper creates only an inactive marked test Timer and refuses to run without --run.
- The helper supports configurable host, port, channel and day through environment variables.
- Added a Makefile target for building and help-checking the real Timer lifecycle helper.

---
## Phase 47.67 - Real VDR Timer lifecycle validation

Status: Completed.

Summary:
- Added a real VDR Timer lifecycle smoke helper for create, readback, update, delete and delete verification.
- The helper creates only an inactive marked test Timer and refuses to run without --run.
- The helper supports configurable host, port, channel and day through environment variables.
- Added a Makefile target for building and help-checking the real Timer lifecycle helper.

---
## Phase 47.66 - Real VDR regression coverage audit

Status: Completed.

Summary:
- Audited existing real VDR smoke helpers and identified non-SearchTimer VDR coverage gaps.
- Confirmed that connectivity and SearchTimer have helper coverage, while status, channels, events, recordings and timers need a dedicated read-only regression helper.
- Documented safety rules for read-only, write and destructive recording-action real VDR tests.
- Moved the next focus to a safe read-only real VDR regression helper.

---
## Phase 47.65 - SearchTimer full payload real VDR validation

Status: Completed.

Summary:
- Extended the real VDR SearchTimer smoke helper to create and update SearchTimers with the full enriched payload.
- Added real VDR readback checks for series, blacklist, match, extended EPG, validity and action option groups.
- Kept the helper safe by requiring --run before modifying a real VDR.
- Moved the next focus to documenting real VDR compatibility findings after executing the full payload helper.

---
## Phase 47.64 - SearchTimer completeness re-audit

Status: Completed.

Summary:
- Re-audited SearchTimer read/write completeness after the Phase 47.60 through Phase 47.63 write-enrichment sequence.
- Confirmed that the write-side gaps from Phase 47.59 are closed for match, extended EPG, validity and action option groups.
- Replaced the historical feature-gap document with a clean resolution matrix.
- Added a dedicated completeness re-audit document and moved the next focus to full payload real VDR validation.

---
## Phase 47.63 - SearchTimer action option write enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for pause-on-recordings, switch timing, sound unmute behavior and automatic deletion limits.
- Extended create/update parsers for SearchTimer action option request fields.
- Extended the RESTfulAPI command executor JSON body with pause_on_recs, switch_min_before, unmute_sound_on_switch, del_recs_after_days, del_after_count_recs and del_after_days_of_first_rec.
- Covered parser defaults and command executor body generation with targeted tests.

---
## Phase 47.62 - SearchTimer validity window write enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for use-in-favorites and active validity windows.
- Extended create/update parsers for validity-window request fields.
- Extended the RESTfulAPI command executor JSON body with use_in_favorites, use_as_searchtimer_from and use_as_searchtimer_til.
- Covered parser defaults and command executor body generation with targeted tests.

---
## Phase 47.61 - SearchTimer extended EPG write enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for extended EPG usage, extended EPG info payload, missing category behavior and content descriptors.
- Extended create/update parsers for extended EPG request fields.
- Extended the RESTfulAPI command executor JSON body with use_ext_epg_info, ext_epg_info, ignore_missing_epg_cats and content_descriptors.
- Covered parser defaults and command executor body generation with targeted tests.

---
## Phase 47.60 - SearchTimer match option write enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for match mode, case handling, tolerance and summary match behavior.
- Extended create/update parsers for match option request fields.
- Extended the RESTfulAPI command executor JSON body with mode, match_case, tolerance and summary_match.
- Covered parser defaults and command executor body generation with targeted tests.

---
## Phase 47.59 - SearchTimer feature gap analysis

Status: Completed.

Summary:
- Documented the SearchTimer read/write feature gap after blacklist enrichment.
- Confirmed that match, extended EPG, validity and action option groups are already present on the read side.
- Confirmed that those groups are still missing from create/update requests and the command executor write body.
- Established Phase 47.60 as match option write enrichment before extended EPG, validity and action option write phases.

---
## Phase 47.58 - SearchTimer blacklist enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for blacklist mode and blacklist ids.
- Preserved RESTfulAPI epgsearch field names for blacklist_mode and blacklist_ids.
- Covered parser and command executor JSON body behavior with targeted tests.
- Deferred extended EPG and cleanup action behavior to later phases.

---
## Phase 47.57 - SearchTimer series recording enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for series recording and retention behavior.
- Preserved RESTfulAPI epgsearch field names for use_series_recording, keep_recs, del_mode and search_timer_action.
- Covered parser and command executor JSON body behavior with targeted tests.
- Deferred blacklist, extended EPG and cleanup action behavior to later phases.

---
## Phase 47.56 - SearchTimer real VDR compatibility report

Status: Completed.

Summary:
- Documented the first successful real VDR SearchTimer end-to-end validation.
- Captured RESTfulAPI runtime behavior where create and update do not reliably return ids in the response body.
- Recorded the production update behavior that preserves the requested backend-native id when RESTfulAPI returns HTTP 200 without an id.
- Documented validated SearchTimer fields and the invalid dayofweek value discovered during real testing.

---
## Phase 47.55 - SearchTimer real VDR smoke test tool

Status: Completed.

Summary:
- Added a real VDR SearchTimer smoke-test helper under apps/tools.
- The helper creates, reads back, updates and deletes a temporary SearchTimer through RESTfulAPI when explicitly run with --run.
- The Makefile target only builds the helper and prints --help, so normal tests do not modify a real VDR.
- The tool prints a PASS/FAIL report for create, readback, field checks, update and cleanup.

---

## Phase 47.54 - SearchTimer repeat handling enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for repeat suppression and repeat comparison behavior.
- Preserved RESTfulAPI epgsearch field names for avoid_repeats, allowed_repeats, repeats_within_days and compare_* options.
- Covered parser and command executor JSON body behavior with targeted tests.
- Kept series recording, action cleanup, blacklist and extended EPG behavior deferred to later phases.

---

## Phase 47.53 - SearchTimer time and duration constraint enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for time, duration and day-of-week constraints.
- Preserved RESTfulAPI epgsearch field names for use_time, start_time, stop_time, use_duration, duration_min, duration_max, use_dayofweek and dayofweek.
- Covered parser and command executor JSON body behavior with targeted tests.
- Kept repeat handling, blacklist behavior and extended EPG behavior deferred to later phases.

---

## Phase 47.52 - SearchTimer channel constraint enrichment

Status: Completed.

Summary:
- Added SearchTimer create/update request fields for channel constraint transport.
- Preserved RESTfulAPI epgsearch field names for use_channel, channels, channel_min and channel_max.
- Kept channel_min and channel_max as request strings because RESTfulAPI resolves backend channel identifiers.
- Covered parser and command executor JSON body behavior with targeted tests.

---

## Phase 47.51 - SearchTimer create/update safe body enrichment foundation

Status: Completed.

Summary:
- Added safe scalar SearchTimer create/update request fields for directory, priority, lifetime, margins and VPS.
- Extended create/update request parsers for the safe enrichment fields.
- Included the safe fields in RESTfulAPI SearchTimer POST and PUT JSON bodies.
- Covered parser and command executor JSON body behavior with targeted tests.

---

## Phase 47.50 - SearchTimer epgsearch Live compatibility analysis

Status: Completed.

Summary:
- Documented the compatibility boundary between VDR, epgsearch, Live-derived SearchTimer semantics, RESTfulAPI and VDR-Suite.
- Captured why SearchTimer create/update enrichment must be phased by safe rule groups instead of implemented as one broad JSON-body patch.
- Identified safe first write-side enrichment fields for the next implementation phase.
- Established Phase 47.51 as SearchTimer create/update safe body enrichment foundation.

---

## Phase 47.48 - SearchTimer preview service test

Status: Completed.

Summary:
- Strengthened SearchTimer preview service regression coverage.
- Verified SearchTimer preview matching, limit handling and JSON statistics output.
- Removed duplicate SearchTimerPreviewService source linkage from the preview test target.

---

## Phase 47.47 - SearchTimer command executor runtime wiring

Status: Completed.

Summary:
- Wired RestfulApiSearchTimerCommandExecutor into DaemonRuntime.
- Linked SearchTimer command, parser and preview sources into the daemon build.
- Verified daemon build with SearchTimer command execution sources.

---

## Phase 47.46 - RESTfulAPI SearchTimer update executor

Status: Completed.

Summary:
- Added RESTfulAPI-backed SearchTimer update execution.
- Sent PUT requests to /searchtimers/<id> with JSON request bodies.
- Preserved backend-aware SearchTimer result creation after successful updates.

---

## Phase 47.45 - RESTfulAPI SearchTimer command executor

Status: Completed.

Summary:
- Added RESTfulAPI SearchTimer command executor foundation.
- Implemented create, update and delete command execution through RESTfulAPI.
- Added isolated command executor coverage for HTTP method, URL and JSON body contracts.

---

## Phase 47.44 - SearchTimer delete route

Status: Completed.

Summary:
- Routed SearchTimer delete operations through the REST API layer.
- Connected delete requests to the SearchTimer command execution boundary.
- Preserved backend-native SearchTimer identity for deletion.

---

## Phase 47.43 - SearchTimer delete API foundation

Status: Completed.

Summary:
- Added backend-neutral SearchTimer delete request, service and result model foundation.
- Added JSON response handling for delete execution results.
- Prepared the route layer for SearchTimer deletion.

---

## Phase 47.42 - SearchTimer update API foundation

Status: Completed.

Summary:
- Added backend-neutral SearchTimer update request, service and result model foundation.
- Added request parsing and JSON response handling for updates.
- Prepared RESTfulAPI-backed SearchTimer update execution.

---

## Phase 47.41 - SearchTimer create API foundation

Status: Completed.

Summary:
- Added backend-neutral SearchTimer create request, service and result model foundation.
- Added request parsing and JSON response handling for create operations.
- Prepared RESTfulAPI-backed SearchTimer creation.

---

## Phase 47.40 - SearchTimer documentation synchronization

Status: Completed.

Summary:
- Synchronized SearchTimer implementation documentation after the initial foundation phases.
- Kept the SearchTimer milestone aligned with backend-neutral architecture and RESTfulAPI integration.
- Established the follow-up path for SearchTimer create, update, delete and runtime execution.

---

## Phase 47.16 - SearchTimer domain model expansion

Status: Completed.

Summary:
- Expanded SearchTimer with recording options for directory, priority and lifetime.
- Added schedule options for start margin, stop margin and VPS usage.
- Mapped stable real RESTfulAPI payload fields into the SearchTimer domain.
- Kept complex filter, repeat and blacklist fields for later typed phases.

---

## Phase 47.11 - SearchTimer endpoint contract

Status: Completed.

Summary:
- Extends SearchTimerController and ApiRouter with backend, state, text, limit and offset query contract for SearchTimer endpoints.

## Phase 47.10 - SearchTimer router integration

Status: Completed.

Summary:
- Adds /api/searchtimers and /api/vdr/searchtimers router paths with optional SearchTimerController handling and unavailable-response semantics.

## Phase 47.9 - SearchTimer controller

Status: Completed.

Summary:
- Adds SearchTimerController for returning backend-neutral SearchTimer JSON responses from SearchTimerResult data.

## Phase 47.8 - SearchTimer JSON serializer

Status: Completed.

Summary:
- Adds SearchTimerResultJsonSerializer for backend-neutral SearchTimer list JSON output.

## Phase 47.7 - SearchTimer RESTfulAPI list adapter

Status: Completed.

Summary:
- Adds RestfulApiSearchTimerAdapter for loading /searchtimers.json via IHttpClient, mapping epgsearch payloads and applying backend-neutral SearchTimer queries.

## Phase 47.6 - SearchTimer RESTfulAPI mapper

Status: Completed.

Summary:
- Adds RestfulApiSearchTimerMapper for mapping RESTfulAPI epgsearch searchtimers JSON into backend-neutral SearchTimer objects.

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

---

## Phase 47.39 - SearchTimer foundation progress

Status: Completed.

Summary:
- Synchronizes the documented project state with the latest verified SearchTimer implementation phase.
- Keeps the current completed phase aligned across README, current status, dashboard, roadmap and development index.
- Leaves the next implementation focus within the SearchTimer track.

Verification:
- make test-docs
- make test-phase

