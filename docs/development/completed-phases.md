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

## Phase 49.30 - EPGSearch native fuzzy validation consolidation

Status: Completed.

Summary:
- Added a consolidated Makefile helper target for the native fuzzy validation helper set.
- Consolidated the native fuzzy validation sequence into one documented operational flow.
- Documented the operator refresh, capability report and persisted restore helper order.
- Clarified the mutation boundary: only operator refresh creates a temporary SearchTimer probe.
- Confirmed capability report and persisted restore validation are read-only after refresh/restart.
- Captured the current native fuzzy backend capability conclusion.
- Listed remaining scope outside the validated backend capability path, including UI/client workflow, full Live-style option surface, conflict/result views and multi-backend frontend polish.

---

## Phase 49.29 - EPGSearch native fuzzy persisted capability restore validation

Status: Completed.

Summary:
- Added startup-restore validation coverage for persisted native fuzzy capability probe results.
- Added a safe VDR-Suite persisted-restore helper that checks startup restore diagnostics and the public capability report.
- Verified that a successful operator refresh persists the native fuzzy probe result.
- Verified that a daemon restart restores the persisted native fuzzy capability state.
- Confirmed `/api/runtime/diagnostics` reports the `epgsearch-native-fuzzy` startup-restore measurement with `statusCode=200` and persisted item count.
- Confirmed `/api/vdr/capabilities` still reports `epg.search.fuzzy.native` as supported, available and availableNow after daemon restart.
- Preserved missing-persisted-result behavior without enabling native fuzzy support.

---

## Phase 49.28 - EPGSearch native fuzzy capability report validation

Status: Completed.

Summary:
- Added `BackendRegistryCapabilityResolver` so capability reports read the current backend capability state dynamically.
- Switched daemon capability report generation away from a startup-only capability snapshot.
- Added regression coverage proving that backend capability updates are reflected in capability resolution.
- Added a safe VDR-Suite capability report validation helper for `/api/vdr/capabilities`.
- Verified the real operator refresh followed by `GET /api/vdr/capabilities`.
- Confirmed that `epg.search.fuzzy.native` reports `supported=true`, `availability=available` and `availableNow=true` after a successful native fuzzy refresh.
- Preserved unavailable behavior for missing backends.

---

## Phase 49.27 - EPGSearch native fuzzy operator refresh operational validation

Status: Completed.

Summary:
- Added a VDR-Suite operator refresh validation helper for the native fuzzy refresh endpoint.
- Verified the real VDR-Suite operator refresh endpoint against yaVDR through /api/epgsearch/native-fuzzy/refresh.
- Confirmed that RESTfulAPI accepts native fuzzy SearchTimer mode=5 and tolerance=2, preserves both values on readback and deletes the temporary probe cleanly.
- Fixed RESTfulAPI SearchTimer create handling for HTTP 200 responses with an empty body by falling back to /searchtimers.json readback and exact query matching.
- Added isolated command executor regression coverage for direct Id responses, empty-body readback fallback and ambiguous readback failure.
- Confirmed the operator refresh result persists the probe result, updates backend capability state and reports epg.search.fuzzy.native=true.
- Added a safe dry-run-by-default operator refresh helper for future real-backend validation.

---

## Phase 49.26 - EPGSearch native fuzzy operator refresh routing validation

Status: Completed.

Summary:
- Added router-level validation for explicit native fuzzy operator refresh routes.
- Validated `/api/epgsearch/native-fuzzy/refresh`.
- Validated `/api/vdr/epgsearch/native-fuzzy/refresh`.
- Added a fake SearchTimer command/data runtime for API router validation.
- Verified that an operator-triggered refresh creates a temporary probe SearchTimer, reads it back, deletes it again and leaves no probe timer behind.
- Verified that native fuzzy capability state is updated from the refresh result.
- Verified the unavailable-controller safety path returns HTTP 503.
- Kept validation local and deterministic without contacting a real VDR.

---

## Phase 49.25 - EPGSearch native fuzzy operator refresh API

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyOperatorRefreshController`.
- Added POST API endpoint for explicit native fuzzy operator refresh.
- Added JSON body parsing for backend/backendId, query/probeQuery, tolerance and safety flags.
- Added JSON response summary for probe, persistence and backend capability update state.
- Wired operator refresh controller through `ApiRouter` and daemon runtime.
- Added controller and router linkage tests.
- Deferred runtime validation/front-end operator action to Phase 49.26.

---

## Phase 49.24 - EPGSearch native fuzzy operator refresh workflow

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyOperatorRefreshService`.
- Operator refresh creates a temporary native fuzzy SearchTimer probe.
- Operator refresh reads the probe SearchTimer back and validates native mode/tolerance preservation.
- Operator refresh deletes the temporary probe SearchTimer by default.
- Probe result is persisted in the native fuzzy capability repository.
- Backend native fuzzy capability is updated from the probe result.
- Missing backends do not trigger a probe.
- Deferred public REST/operator trigger endpoint to Phase 49.25.

---

## Phase 49.23 - EPGSearch native fuzzy stale probe administration API

Status: Completed.

Summary:
- Added REST controller for stale native fuzzy probe administration.
- Added GET endpoints for listing stale persisted native fuzzy probe results.
- Added POST endpoints for deleting stale persisted native fuzzy probe results.
- Fresh persisted probe results remain untouched.
- API is local persistence administration only and performs no VDR/SearchTimer mutation.
- Wired controller through `ApiRouter` and daemon runtime.
- Deferred operator refresh/reprobe workflow to Phase 49.24.

---

## Phase 49.22 - EPGSearch native fuzzy stale probe administration

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyStaleProbeAdministrationService`.
- Repository can now list all persisted native fuzzy probe results.
- Repository can delete a persisted probe result by backend id.
- Administration service can list stale and future-timestamp probe results.
- Administration service can delete stale and future-timestamp probe results.
- Fresh persisted probe results are kept untouched.
- Administration is schema-safe and creates the persistence schema when missing.
- Administration does not contact VDR and does not mutate SearchTimer objects.
- Deferred REST administration endpoint to Phase 49.23.

---

## Phase 49.21 - EPGSearch native fuzzy restore freshness policy

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyCapabilityFreshnessPolicy`.
- Repository now exposes persisted probe metadata including `updatedAt` and computed `ageSeconds`.
- Startup restore now applies persisted results only when they are fresh.
- Default freshness window is seven days.
- Stale persisted positive results cannot enable native fuzzy capability.
- Stale persisted results restore native fuzzy as unavailable for the existing backend.
- Future timestamps are not trusted.
- Restore diagnostics now expose stale ignored result count.
- Freshness policy remains read-only and does not create SearchTimer probe objects.
- Deferred stale probe administration to Phase 49.22.

---

## Phase 49.20 - EPGSearch native fuzzy restore diagnostics

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyStartupRestoreDiagnostics`.
- Added JSON serialization for startup restore diagnostics.
- Restore diagnostics expose schema readiness, backend count, persisted result count, update count and native true/false counts.
- Restore diagnostics provide machine-readable status and human-readable reason strings.
- Daemon startup restore now records a runtime diagnostics measurement.
- Runtime measurement uses component `epgsearch-native-fuzzy` and operation `startup-restore`.
- Startup restore diagnostics remain read-only and do not create SearchTimer probe objects.
- Deferred freshness/expiry policy to Phase 49.21.

---
## Phase 49.19 - EPGSearch native fuzzy startup restore integration

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyStartupRestoreService`.
- Startup restore creates the persistence schema if needed.
- Startup restore reloads persisted probe results for existing backends.
- Successful persisted results restore native fuzzy capability availability.
- Failed persisted results restore native fuzzy as unavailable.
- Missing persisted results leave backend capabilities unchanged.
- Persisted results for missing backends do not create backend nodes.
- Daemon startup applies persisted restore before capability report construction.
- Capability report now uses restored default backend capabilities.
- Startup restore remains read-only/non-mutating and does not create SearchTimer probe objects.

---
## Phase 49.18 - EPGSearch native fuzzy persisted capability restore

Status: Completed.

Summary:
- Added `EpgSearchNativeFuzzyCapabilityRestoreService`.
- Restore service loads persisted probe results by backend id.
- Successful persisted probe results restore `epg.search.fuzzy.native=true` for existing backends.
- Failed/incomplete persisted probe results restore native fuzzy as unavailable.
- Missing persisted results do not update backend state.
- Persisted results for missing backends do not create backend nodes.
- Restore behavior is backend-scoped for multi-backend operation.
- Deferred automatic daemon startup invocation to Phase 49.19.

---
## Phase 49.17 - EPGSearch native fuzzy capability persistence

Status: Completed.

Summary:
- Added SQLite schema support for persisted native fuzzy probe results.
- Added `EpgSearchNativeFuzzyCapabilityRepository`.
- Persisted probe results are keyed by backend id.
- Successful probe results roundtrip through SQLite and still enable the detector outcome.
- Failed/incomplete probe results roundtrip through SQLite and keep native fuzzy unavailable.
- Multiple backend probe results are isolated from each other.
- Deferred persisted runtime restore to Phase 49.18.

---
## Phase 49.16 - EPGSearch native fuzzy runtime capability wiring

Status: Completed.

Summary:
- Added backend capability update support to `BackendRegistry`.
- Added service-level capability update support to `BackendRegistryService`.
- Wired successful native fuzzy probe results into existing backend runtime capability state.
- Confirmed `epg.search.fuzzy.native` becomes available only when the successful probe result is applied.
- Confirmed failed probe results keep native fuzzy disabled.
- Confirmed missing backend updates fail safely and do not create new backends.
- Confirmed backend identity and runtime flags are preserved during capability updates.
- Deferred persistence of probe results to Phase 49.17.

---
## Phase 49.15 - EPGSearch native fuzzy capability autodetection

Status: Completed.

Summary:
- Added a deterministic native fuzzy capability probe result model.
- Added `EpgSearchNativeFuzzyCapabilityDetector`.
- Detector enables `epg.search.fuzzy.native` only when create, readback, mode preservation, tolerance preservation and cleanup all succeeded.
- Detector preserves unrelated base capabilities and keeps fallback capability independent.
- Added unit coverage for successful and failed probe outcomes.
- Extended the real-backend validator to emit a machine-readable capability probe result and native fuzzy capability signal after successful validation.
- Deferred runtime/backend registry capability wiring to Phase 49.16.

---
## Phase 49.14 - EPGSearch native fuzzy real-backend validation

Status: Completed.

Summary:
- Added a safe real-backend validation harness for native fuzzy SearchTimer passthrough.
- Validator defaults to dry-run and sends no request unless `--execute` is provided.
- Validator can create a temporary SearchTimer with native epgsearch `mode=5` and `tolerance=<int>`.
- Validator reads the SearchTimer back from `/searchtimers.json` and verifies native mode and tolerance.
- Validator deletes the temporary SearchTimer after validation unless `--keep-created` is explicitly provided.
- Added a runbook for dry-run, execution, authentication, success output and failure interpretation.
- Deferred runtime capability autodetection to Phase 49.15.

---
## Phase 49.13 - EPGSearch native fuzzy adapter passthrough

Status: Completed.

Summary:
- Added public SearchTimer body aliases for `mode` and `tolerance`.
- Mapped `mode=fuzzy` to the epgsearch-compatible numeric mode 5.
- Mapped `tolerance=<int>` to SearchTimer match tolerance.
- Preserved existing numeric `matchMode` and `matchTolerance` compatibility.
- Verified RESTfulAPI SearchTimer mapper readback of native fuzzy mode 5 plus tolerance.
- Verified RESTfulAPI SearchTimer command executor passthrough for create and update.
- Deferred real-backend native fuzzy validation to Phase 49.14.

---
## Phase 49.12 - EPGSearch native fuzzy capability mapping

Status: Completed.

Summary:
- Added explicit fallback and native fuzzy EPGSearch capabilities.
- Added `epg.search.fuzzy.fallback` to the default capability report.
- Added `epg.search.fuzzy.native` to the default capability report.
- Exposed fallback fuzzy support separately from native epgsearch fuzzy support.
- Marked the snapshot read-only capability set as fallback-fuzzy capable.
- Kept native fuzzy unavailable unless a backend explicitly advertises it.
- Added resolver, report builder, report service, capability set and controller coverage.

---
## Phase 49.11 - EPGSearch fuzzy fallback matcher

Status: Completed.

Summary:
- Enabled explicit `mode=fuzzy` at the REST/controller boundary.
- Added `tolerance=<int>` forwarding from the router to the controller.
- Rejected negative and non-integer fuzzy tolerance values with HTTP 400.
- Added fuzzy tolerance transport from `EpgSearchRequest` to `EpgSearchQuery`.
- Implemented a deterministic backend-neutral boolean fuzzy fallback matcher.
- Kept fuzzy ranking and native epgsearch passthrough as later capability work.
- Added request, matcher, controller and router regression coverage.

---
## Phase 49.10 - EPGSearch fuzzy mode decision

Status: Completed.

Summary:
- Added ADR-0033 for EPGSearch fuzzy mode semantics.
- Decided that public fuzzy search uses mode=fuzzy plus tolerance=<int>.
- Aligned fuzzy mode with the LIVE/epgsearch SearchTimer model.
- Decided that native epgsearch adapters may map fuzzy to mode 5 plus tolerance.
- Decided that VDR-Suite may provide a backend-neutral boolean fallback matcher.
- Deferred public ranking/scoring semantics.
- Deferred implementation to Phase 49.11.

---
## Phase 49.9 - EPGSearch regex mode implementation

Status: Completed.

Summary:
- Enabled explicit `mode=regex` at the REST/controller boundary.
- Mapped `mode=regex` to `EpgSearchMode::RegularExpression`.
- Implemented regex matching in `EpgSearchMatcher`.
- Returned HTTP 400 for invalid regex patterns.
- Preserved default phrase/contains behavior.
- Added router and controller regressions for valid and invalid regex mode.
- Kept fuzzy mode as a separate future decision.

---
## Phase 49.8 - EPGSearch regex mode safety decision

Status: Completed.

Summary:
- Added ADR-0032 for EPGSearch regex mode safety.
- Decided that regex is only enabled explicitly via `mode=regex`.
- Decided that invalid regex input must return HTTP 400.
- Decided that invalid regex input must not crash the process or fall back to phrase matching.
- Kept default phrase/contains behavior and deterministic exact/all/any modes stable.
- Deferred regex execution implementation to Phase 49.9.
- Kept fuzzy search as a separate future decision.

---
## Phase 49.7 - deterministic EPGSearch modes

Status: Completed.

Summary:
- Implemented deterministic EPGSearch modes for exact, all-words and any-word matching.
- Added REST/controller handling for `mode=exact`, `mode=all`/`mode=allWords` and `mode=any`/`mode=anyWord`.
- Kept default phrase/contains behavior unchanged.
- Added controller regressions for exact, all-words, any-word and invalid mode handling.
- Added router regression for `mode=exact` and invalid `mode` handling.
- Deferred regular-expression and fuzzy modes to separate decisions.

---
## Phase 49.6 - EPGSearch search-mode baseline regression

Status: Completed.

Summary:
- Added router-level regression coverage for current EPGSearch default text search behavior.
- Added controller-level regression coverage for current EPGSearch default text search behavior.
- Verified that uppercase `TATORT` matches the local `Tatort` EPG event.
- Verified that unrelated EPG events are excluded by the text query.
- Documented that advanced modes already exist in the query model but are not yet mapped or executed.
- Moved the next focus to EPGSearch search-mode implementation decision.

---
## Phase 49.5 - EPGSearch parameter regression expansion

Status: Completed.

Summary:
- Added router-level regression coverage for invalid `/api/epg/search` parameters.
- Verified `timespan <= 0` returns HTTP 400.
- Verified negative `limit` and `offset` return HTTP 400.
- Verified invalid `sort` and `order` values return HTTP 400.
- Preserved the positive endpoint regression from Phase 49.4.
- Moved the next focus to EPGSearch search-mode regression.

---
## Phase 49.4 - EPGSearch endpoint regression

Status: Completed.

Summary:
- Added explicit router-level regression coverage for `GET /api/epg/search`.
- Covered query, backend, channel, time-window, paging and sort/order parameters at the route boundary.
- Verified the current nested `matches[].event` JSON structure.
- Confirmed the route flows through router, controller, request mapper, query service and serializer.
- Moved the next focus to EPGSearch parameter regression expansion.

---
## Phase 49.3 - EPGSearch legacy test retirement

Status: Completed.

Summary:
- Removed obsolete request-era EPGSearch matcher and service tests.
- Removed their Makefile targets.
- Kept `test-epg-search-request` because `EpgSearchRequest` remains the API/controller request model.
- Preserved active matcher and service coverage through compact `test_epgsearch_*` tests.
- Moved the next focus to explicit EPGSearch endpoint regression.

---
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

