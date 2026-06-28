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

### Real VDR Acceptance and Runtime Lifecycle Foundation

Status: Completed.

Purpose:
- Establish repeatable real VDR acceptance checks for safe and dry-run API coverage.
- Make daemon lifecycle behavior operator-safe for duplicate starts and normal SIGTERM shutdown.
- Preserve a new-chat handoff checklist so local runtime, acceptance, documentation and GitHub Actions checks are not forgotten.

Completed phases:
- Phase 55.5c - Real VDR acceptance manifest foundation.
- Phase 55.5d - Acceptance manifest route validation.
- Phase 55.5e - Acceptance JSON report output.
- Phase 55.5g - Expected JSON values in acceptance runner.
- Phase 55.5h - Acceptance manifest safe-read expansion.
- Phase 55.5i - SearchTimer plan dry-run acceptance probe.
- Phase 55.5j - Safe query acceptance coverage.
- Phase 55.5k - Real VDR acceptance cold-recording timeout stabilization.
- Phase 55.5l - Daemon startup/shutdown hardening.
- Phase 55.5m - Documentation consolidation and roadmap alignment.

Key outcomes:
- Real VDR acceptance currently verifies 20/20 safe and dry-run probes.
- Duplicate daemon startup on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- New-chat handoff documentation now records the required local, real VDR and GitHub Actions evidence before declaring runtime phases complete.

---

### Documentation Consolidation

Status: Completed.

Purpose:
- Refresh the high-level project documentation before starting the next major milestone.
- Make completed foundations visible as milestones instead of only as individual phases.
- Align roadmap, dashboard and completed phase history.

Completed phases:
- Phase 46.38 - Roadmap and Milestone Refresh.
- Phase 46.39 - Project Status Dashboard Refresh.

Completed consolidation work:
- Completed phase milestone restructuring.
- Current status refresh.
- README refresh.
- Roadmap, dashboard and project progress alignment.
- New-chat handoff verification checklist.

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

## Phase 55.5m - Documentation consolidation and roadmap alignment

Status: Completed.

Summary:
- Consolidated README, roadmap, project progress, current status, project dashboard, development index and completed-phase history after the Phase 55.5 real VDR acceptance and daemon lifecycle hardening work.
- Moved the tracked latest completed implementation marker to Phase 55.5m across the documentation files monitored by `check_phase_consistency.py`.
- Moved the tracked next implementation focus to Phase 55.6 recording operations audit and safety policy.
- Preserved the new-chat handoff checklist for local runtime, real VDR acceptance, documentation and GitHub Actions verification.
- Kept this phase documentation-only with no runtime behavior, API behavior, backend mutation or recording operation change.

---

## Phase 55.5l - Daemon startup/shutdown hardening

Status: Completed.

Summary:
- Changed HTTP listener bind failure handling so a duplicate daemon start on an occupied port exits cleanly with status 1 instead of throwing an uncaught exception and producing a core dump.
- Added guardrail coverage for bind failure behavior.
- Changed the HTTP listener loop to poll for stop requests and break cleanly on signal interruption.
- Verified against a real local daemon that SIGTERM stops the process without `kill -9` and releases port 18080.
- Verified the final real VDR acceptance run after hardening with 20/20 probes passing.

---

## Phase 55.5k - Real VDR acceptance cold-recording timeout stabilization

Status: Completed.

Summary:
- Added per-probe timeout support to the real VDR acceptance runner.
- Increased the cold recording query acceptance timeout so large real recording catalogs do not cascade into unrelated probe timeouts.
- Preserved the default timeout for ordinary probes.
- Added timeoutSeconds to the JSON report output for each probe result.
- Verified that the cold real VDR acceptance run passed 20/20 probes.

---

## Phase 55.5j - Safe query acceptance coverage

Status: Completed.

Summary:
- Extended the real VDR acceptance manifest with safe query probes for recording query, EPG search, EPG now/next and EPG time-window endpoints.
- Kept all new probes safe and non-mutating.
- Exposed the practical runtime cost of large recording catalogs during cold acceptance runs.

---

## Phase 55.5i - SearchTimer plan dry-run acceptance probe

Status: Completed.

Summary:
- Added a dry-run SearchTimer workflow planning acceptance probe.
- Verified that create planning returns a valid dry-run plan with create/readback steps and explicit operator confirmation requirements.
- Kept backend mutation out of acceptance scope.

---

## Phase 55.5h - Acceptance manifest safe-read expansion

Status: Completed.

Summary:
- Expanded the real VDR acceptance manifest with safe read probes for channels, timers, recordings, events and SearchTimer lists.
- Kept the acceptance manifest focused on safe and dry-run coverage.

---

## Phase 55.5g - Expected JSON values in acceptance runner

Status: Completed.

Summary:
- Added expectedJsonValues support to the real VDR acceptance runner.
- Strengthened SearchTimer workflow validation acceptance from HTTP-200-only to response-value verification.
- Verified validation semantics for operation, execution mode, workflow steps and safety flags.

---

## Phase 55.5e - Acceptance JSON report output

Status: Completed.

Summary:
- Added optional JSON report output to the real VDR acceptance runner.
- Captured manifest name, base URL, risk level, totals, duration and per-probe results.

---

## Phase 55.5d - Acceptance manifest route validation

Status: Completed.

Summary:
- Added validation that acceptance manifest routes are present in ApiRouter.
- Prevented stale acceptance entries from silently drifting away from routed API paths.

---

## Phase 55.5c - Real VDR acceptance manifest foundation

Status: Completed.

Summary:
- Added the real VDR acceptance manifest and runner foundation.
- Added initial safe and dry-run probes for dashboard, VDR overview, status, health, backend registry, capabilities, runtime summary and SearchTimer workflow paths.
- Wired manifest validation into the local CI-fast/VDR test groups.

---

## Phase 55.5b - Native SearchTimer preview capability gate

Status: Completed.

Summary:
- Added a native SearchTimer preview capability flag.
- Exposed native-preview availability through capability reporting while keeping default unsupported behavior.
- Preserved suite EPG cache preview as the safe default preview engine.

---

## Phase 55.5a - SearchTimer preview engine contract

Status: Completed.

Summary:
- Added the SearchTimer preview engine contract.
- Exposed the preview engine in JSON responses so clients can distinguish suite-cache preview from future native epgsearch preview.
- Preserved `suite-epg-cache` as the default preview engine.

---

## Phase 55.4e - Daemon runtime shutdown reset guardrail

Status: Completed.

Summary:
- Completed daemon runtime shutdown reset coverage for recently added runtime components.
- Added a guardrail that checks shutdown reset coverage for runtime-owned services/controllers.
- Wired the shutdown reset guardrail into the local test groups.

---


## Phase 54.3e - SearchTimer preview EPG input status contract

Status: Completed.

Summary:
- Added ADR-0034 for SearchTimer warm EPG cache and change invalidation.
- Documented the measured RESTfulAPI full EPG dump cost of about 30 MB and about 28 seconds for a 14-day EPG window.
- Decided that interactive SearchTimer preview must not fetch the full RESTfulAPI EPG dump per request.
- Defined backend-scoped warm EPG cache readiness, stale-state and future SSE invalidation rules.
- Updated roadmap, project status and development documentation to make Phase 54.3 the warm EPG cache implementation step.

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

## Phase 54.0 - SearchTimer runtime mutation policy wiring

- SearchTimer Create/Update/Delete services are now wired into the daemon runtime.
- Public direct SearchTimer mutation is still blocked by a closed runtime mutation policy executor.
- The API path can now distinguish service wiring from production mutation enablement.
- This phase intentionally does not enable real backend SearchTimer mutation.

## Phase 53.8 - SearchTimer title-only workflow completion audit

Status: Completed.

Summary:
- Closed the SearchTimer title-only workflow chain with a completion audit.
- Documented direct REST and workflow-path title-only preservation.
- Confirmed production mutation remains gated and no scheduler/runtime automation was enabled.
- Handed off to the cross-backend search and federation foundation.

---

## Phase 53.7 - SearchTimer title-only workflow execution dispatch contract

Status: Completed.

Summary:
- Verified controlled workflow create dispatch preserves title-only compare fields into the injected command executor.
- Verified controlled workflow update dispatch preserves title-only compare fields into the injected command executor.
- Kept production backend mutation gated by execution mode, policy, guard and kill switch controls.

---

## Phase 53.6 - SearchTimer title-only workflow command mapper contract

Status: Completed.

Summary:
- Mapped preserved workflow plan compare fields into SearchTimerCreateRequest.
- Mapped preserved workflow plan compare fields into SearchTimerUpdateRequest.
- Added title-only command request mapper coverage for workflow create and update.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

---

## Phase 53.5 - SearchTimer title-only workflow request contract

Status: Completed.

Summary:
- Added title-only compare-field preservation to SearchTimerWorkflowRequest.
- Parsed compareTitle/compareSubtitle/compareSummary/compareCategories in workflow validation requests.
- Preserved compare fields in SearchTimerWorkflowExecutionPlan::fromRequest.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

---

## Phase 53.4 - SearchTimer title-only update controller contract

Status: Completed.

Summary:
- Strengthened SearchTimer REST controller update-path coverage for title-only JSON bodies.
- Captured the update request handed to the test command executor.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false survive the controller handoff.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

---

## Phase 53.3 - SearchTimer title-only update parser contract

Status: Completed.

Summary:
- Strengthened SearchTimer update request parser coverage for title-only JSON bodies.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false are preserved.
- Verified subtitle/summary search flag combinations are preserved.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

---

## Phase 53.2 - SearchTimer title-only REST controller contract

Status: Completed.

Summary:
- Strengthened SearchTimer REST controller create-path coverage for title-only JSON bodies.
- Captured the create request handed to the test command executor.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false survive the controller handoff.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

---

## Phase 53.1 - SearchTimer title-only request parser contract

Status: Completed.

Summary:
- Strengthened SearchTimer create request parser coverage for title-only JSON bodies.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false are preserved.
- Verified subtitle/summary search flag combinations are preserved.
- Added a dedicated parser test target to the local test group.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

---

## Phase 53.0 - SearchTimer title-only RESTfulAPI field mapping

Status: Completed.

Summary:
- Fixed RESTfulAPI SearchTimer field mapping for title/subtitle/description search flags.
- Mapped compareTitle to use_title.
- Mapped compareSubtitle to use_subtitle.
- Mapped compareSummary to use_description.
- Added executor-level request body tests for title-only and subtitle/summary search field combinations.
- Kept the phase free of scheduler, automation execution and backend write policy changes.

---

## Phase 52.9 - SearchTimer automation safety review

Status: Completed.

Summary:
- Added SearchTimerAutomationSafetyReview and SearchTimerAutomationSafetyReviewResult.
- Consolidated dry-run, scheduling, execution and mutation safety boundaries.
- Confirmed preview-only safety while keeping scheduling runtime, automatic execution and backend mutation explicitly blocked.
- Covered safe preview and invalid-review paths with a targeted unit test.
- Kept the phase free of daemon scheduler runtime, background loops, RESTfulAPI writes and epgsearch mutation.

---

## Phase 52.8 - SearchTimer automation daemon scheduling plan

Status: Completed.

Summary:
- Added SearchTimerAutomationDaemonSchedulingPlan for future daemon automation scheduling boundaries.
- Modeled disabled and preview-only scheduling plans, intervals, candidate limits, snapshot freshness, duplicate review requirements, safety reasons and audit trail.
- Enforced scheduler-disabled, dry-run-only, no-mutation, no-timer-creation, no-backend-write, no-automatic-execution and explicit handoff invariants.
- Covered clamping, validation, audit trail and safety invariants with a targeted unit test.
- Kept the phase free of daemon scheduler runtime, background loops, RESTfulAPI writes and epgsearch mutation.

---

## Phase 52.7 - SearchTimer automation REST preview contract

Status: Completed.

Summary:
- Added SearchTimerAutomationPreviewController for read-only automation dry-run previews.
- Added GET /api/searchtimers/automation/preview and /api/vdr/searchtimers/automation/preview routing.
- Wired the preview controller into ApiRouter and DaemonRuntime.
- Preserved dry-run-only, no-mutation, no-timer-creation, no-backend-write, no-automatic-execution and explicit handoff invariants.
- Kept the phase free of matching execution, scheduling and backend mutation.

---

## Phase 52.6 - SearchTimer automation read-only service boundary

Status: Completed.

Summary:
- Added SearchTimerAutomationReadOnlyService as the read-only SearchTimer automation service boundary.
- Aggregated supplied evaluation plans, match candidates, duplicate detections and candidate timer proposals into dry-run results.
- Added warnings and audit entries for boundary-level validation and count mismatches.
- Preserved dry-run-only, no-mutation, no-timer-creation, no-backend-write, no-automatic-execution and explicit handoff invariants.
- Kept the phase free of matching execution, REST endpoints, daemon scheduling and backend mutation.

---

## Phase 52.5 - SearchTimer automation dry-run result serializer

Status: Completed.

Summary:
- Added SearchTimerAutomationDryRunResult as the read-only SearchTimer automation dry-run aggregate.
- Added SearchTimerAutomationDryRunResultJsonSerializer for frontend-visible dry-run JSON.
- Serialized evaluation plan state, match candidates, duplicate detections, candidate timer proposals, warnings, errors and audit trail.
- Enforced dry-run-only, no-mutation, no-timer-creation, no-backend-write, no-automatic-execution and explicit handoff invariants.
- Kept the phase free of services, REST endpoints, daemon scheduling and backend mutation.

---

## Phase 52.4 - SearchTimer automation candidate timer proposal model

Status: Completed.

Summary:
- Added SearchTimerAutomationCandidateTimerProposal as the read-only SearchTimer automation timer proposal model.
- Modeled proposed start/end time, margins, directory, priority, lifetime, duplicate risk, existing timer/recording references and block reasons.
- Enforced dry-run-only, no-mutation, no-timer-creation, no-backend-write, no-automatic-execution and explicit handoff invariants.
- Covered proposal construction, duplicate blocking, validation, clamping and safety behavior with a targeted unit test.
- Kept the phase free of proposal services, serializers, REST endpoints, daemon scheduling and backend mutation.

---

## Phase 52.3 - SearchTimer automation duplicate detection model

Status: Completed.

Summary:
- Added SearchTimerAutomationDuplicateDetection as the read-only SearchTimer automation duplicate risk model.
- Modeled duplicate risk level, existing timer reference, existing recording reference, title similarity, overlap and duplicate reasons.
- Enforced dry-run-only, no-mutation, no-automatic-decision and no-timer-proposal invariants.
- Covered validation, risk behavior, clamping and safety behavior with a targeted unit test.
- Kept the phase free of duplicate services, automatic decisions, candidate timer creation, REST endpoints, daemon scheduling and backend mutation.

---

## Phase 52.2 - SearchTimer automation match candidate model

Status: Completed.

Summary:
- Added SearchTimerAutomationMatchCandidate as the read-only SearchTimer automation candidate model.
- Modeled backend id, SearchTimer id, EPG event id, event metadata, match score and match reasons.
- Enforced dry-run-only, no-mutation, no-timer-proposal and duplicate-check-required invariants.
- Covered validation, score clamping, time clamping and safety behavior with a targeted unit test.
- Kept the phase free of matching execution, duplicate decisions, candidate timer creation, REST endpoints, daemon scheduling and backend mutation.

---

## Phase 52.1 - SearchTimer automation evaluation plan model

Status: Completed.

Summary:
- Added SearchTimerAutomationEvaluationPlan as the first read-only SearchTimer automation model.
- Modeled backend id, candidate limit and read-only inclusion flags.
- Enforced dry-run-only, no-mutation, no-scheduled-execution and explicit execution handoff invariants.
- Covered validation and safety behavior with a targeted unit test.
- Kept the phase free of EPG matching, duplicate detection, candidate timer creation, REST endpoints, daemon scheduling and backend mutation.

---

## Phase 52.0 - SearchTimer automation foundation planning

Status: Completed.

Summary:
- Started SearchTimer automation as a planning-only architecture phase.
- Defined the automation boundary before scheduled evaluation, matching, duplicate analysis, candidate timer proposal or execution.
- Separated evaluation planning, candidate matching, duplicate analysis, candidate proposal, validation and execution handoff.
- Preserved the no-real-timer-create, no-real-timer-update, no-real-timer-delete, no-epgsearch-mutation and no-hidden-production-enablement boundary.
- Handed off to Phase 52.1 SearchTimer automation evaluation plan model.

---

## Phase 51.10 - Live parity discovery foundation completion

Status: Completed.

Summary:
- Closed the Live parity discovery foundation.
- Documented the completed Phase 51 discovery stack from source audit through RESTfulAPI provider contract.
- Froze the pre-transport read-only discovery boundary before Phase 52.
- Confirmed the stable discovery API surface and JSON response shape.
- Preserved the no-HTTP-execute, no-epgsearch-fetching and no-mutation boundary.
- Handed off to Phase 52.0 SearchTimer automation foundation planning.

---

## Phase 51.9 - Live parity discovery RESTfulAPI provider contract

Status: Completed.

Summary:
- Added RestfulApiSearchTimerDiscoveryProvider as the RESTfulAPI-facing discovery provider contract.
- Defined /searchtimers/discovery.json as the upstream endpoint contract.
- Implemented safe pre-transport empty catalog behavior.
- Verified configured backend fallback and explicit backend override.
- Preserved the no-IHttpClient, no-HTTP-execute, no-JSON-parsing, no-epgsearch-fetching and no-mutation boundary.

---

## Phase 51.8 - Live parity discovery HTTP smoke contract

Status: Completed.

Summary:
- Strengthened TestHttpServer coverage for the daemon-exposed SearchTimer discovery route.
- Verified /api/searchtimers/discovery?backend=http-server.
- Verified /api/vdr/searchtimers/discovery default backend fallback.
- Verified /api/vdr/searchtimers/discovery?backend=ferienhaus backend propagation.
- Verified the exact safe empty-provider JSON response body.
- Verified POST /api/searchtimers/discovery remains unavailable through the generic not-found response.
- Preserved the no-RESTfulAPI-transport, no-epgsearch-fetching, no-mutation boundary.

---

## Phase 51.7 - Live parity discovery daemon wiring

Status: Completed.

Summary:
- Added SearchTimerDiscoveryStaticProvider as a safe empty discovery provider for daemon wiring.
- Wired SearchTimerDiscoveryStaticProvider, SearchTimerDiscoveryService, SearchTimerDiscoveryJsonSerializer and SearchTimerDiscoveryController into DaemonRuntime.
- Injected SearchTimerDiscoveryController into ApiRouter during daemon initialization.
- Added TestHttpServer smoke coverage for /api/searchtimers/discovery with backend query propagation and zero discovery counts.
- Preserved the no-RESTfulAPI-transport, no-epgsearch-fetching, no-mutation boundary.

---

## Phase 51.6 - Live parity discovery router contract

Status: Completed.

Summary:
- Added read-only ApiRouter route handling for /api/searchtimers/discovery and /api/vdr/searchtimers/discovery.
- Added backend query parameter handling with default backend fallback.
- Added missing SearchTimerDiscoveryController 503 behavior.
- Added test-api-router coverage with an in-memory discovery provider.
- Wired SearchTimerDiscoveryController into the daemon link set while preserving the no-daemon-provider, no-transport, no-mutation boundary.

---

## Phase 51.5 - Live parity discovery controller service integration

Status: Completed.

Summary:
- Added a service-backed SearchTimerDiscoveryController constructor.
- Added getDiscovery(backendId) to delegate read-only discovery to SearchTimerDiscoveryService.
- Preserved the direct catalog-based getDiscovery path for tests and compatibility.
- Added controller coverage for catalog path, service path and missing-service behavior.
- Documented the controller/service integration and preserved the no-router, no-transport, no-mutation boundary.

---

## Phase 51.4 - Live parity discovery service contract

Status: Completed.

Summary:
- Added ISearchTimerDiscoveryProvider as the read-only provider boundary for discovery catalogs.
- Added SearchTimerDiscoveryService to delegate backend-aware discovery to an injected provider.
- Added a targeted in-memory provider unit test for backendId propagation and discovery list preservation.
- Wired the service into VDR_SRC and local VDR test grouping.
- Documented the service/provider contract and preserved the no-controller-rewire, no-router, no-transport, no-mutation boundary.

---

## Phase 51.3 - Live parity discovery REST controller contract

Status: Completed.

Summary:
- Added SearchTimerDiscoveryController as a read-only REST response boundary.
- Returned ApiResponse with statusCode 200, contentType application/json and SearchTimerDiscoveryJsonSerializer body.
- Added a targeted controller unit test for status, content type and JSON body contract.
- Wired the test into Makefile and local VDR test grouping.
- Documented the controller contract and preserved the no-router, no-transport, no-mutation boundary.

---

## Phase 51.2 - Live parity discovery JSON contract

Status: Completed.

Summary:
- Added SearchTimerDiscoveryJsonSerializer.
- Serialized backendId, counts, Extended EPG info, channel groups, blacklists and recording directories.
- Added a targeted serializer unit test.
- Wired the serializer into VDR_SRC and local VDR test grouping.
- Documented the JSON contract and preserved the read-only phase boundary.
- Kept the phase free of REST routes, backend transport and backend mutation.

---

## Phase 51.1 - Live parity discovery domain foundation

Status: Completed.

Summary:
- Added the first read-only Live parity discovery domain foundation.
- Introduced SearchTimerDiscoveryExtendedEpgInfo for epgsearch extended EPG info lists.
- Introduced SearchTimerDiscoveryChannelGroup for epgsearch channel group discovery.
- Introduced SearchTimerDiscoveryBlacklist for epgsearch blacklist discovery.
- Introduced SearchTimerDiscoveryRecordingDirectory for SearchTimer recording directory discovery.
- Introduced SearchTimerDiscoveryCatalog to preserve backend identity and aggregate discovery lists.
- Added a targeted unit test and Makefile target.
- Fixed daemon source wiring for SearchTimerWorkflowValidationRequestParser.
- Kept the phase domain-only with no REST route, no adapter transport and no backend mutation.

---

## Phase 51.0 - Live plugin parity source audit and gap matrix

Status: Completed.

Summary:
- Started the Live Plugin Parity Foundation milestone.
- Audited the ownership split between VDR core, epgsearch, RESTfulAPI, Live and VDR-Suite.
- Confirmed that RESTfulAPI integration and extension remains preferred over a VDR fork.
- Documented the first Live parity gap matrix for SearchTimer workflow semantics, helper lists, timer conflicts, EPG details, timer details and recording details.
- Kept production mutation closed.
- Set Phase 51.1 as the next implementation focus for a read-only Live parity discovery API domain foundation.

---


## Phase 50.50 - SearchTimer workflow foundation completion documentation

Status: Completed.

Summary:
- Closed the SearchTimer User Workflow Foundation milestone.
- Documented completed SearchTimer workflow capabilities, safety boundaries, REST contract and test coverage.
- Updated project progress source to mark SearchTimer User Workflow as completed.
- Prepared the transition to Phase 51 Live plugin parity foundation work.
- Kept production mutation closed.

---

## Phase 50.49 - SearchTimer workflow end-to-end verified execution test

Status: Completed.

Summary:
- Added a dedicated SearchTimer end-to-end verified execution test.
- Verified create, update and delete workflows from planning through controlled executor invocation to backend readback verification.
- Verified final workflow failure when required readback fails after a successful executor result.
- Added the test target to local VDR test grouping.
- Kept production mutation closed.

---

## Phase 50.48 - SearchTimer workflow verified execution REST contract

Status: Completed.

Summary:
- Updated REST/router test expectations for delete readback follow-up semantics.
- Asserted REST-visible verified execution fields in controller and router execution responses.
- Documented `executorResultSuccessful`, `backendReadbackVerificationAttached`, `backendReadbackVerified`, nested `backendReadbackVerification`, final `success` and `dispatchStage` semantics.
- Kept production mutation closed.

---

## Phase 50.47 - SearchTimer workflow readback services dispatch integration

Status: Completed.

Summary:
- Integrated SearchTimer create, update and delete readback verification services into the command dispatch path.
- Added readback data source injection to `SearchTimerWorkflowCommandDispatchOptions`.
- Attached backend readback verification results after successful controlled executor results.
- Forced final workflow success to false when required backend readback verification fails.
- Made delete workflows require a readback follow-up step.
- Extended command dispatch, execution plan and execution service tests.
- Kept production mutation closed.

---

## Phase 50.46 - SearchTimer workflow delete absence verification service

Status: Completed.

Summary:
- Added `SearchTimerWorkflowDeleteAbsenceVerificationService`.
- Verified delete success by proving backend-native id absence through `ISearchTimerDataSource`.
- Reported failed delete, missing backend id, missing backend-native id, unavailable data source, still-visible deleted id and ambiguous deleted-id candidates.
- Added focused unit coverage and a local Make target.
- Added the target to the VDR local test group.
- Kept production mutation closed.

---
## Phase 50.45 - SearchTimer workflow delete absence verification plan

Status: Completed.

Summary:
- Defined the delete absence verification plan for future SearchTimer delete workflows.
- Established that delete executor success alone is not enough for verified delete success.
- Defined backend-native id absence as the core verification criterion.
- Defined failure cases for missing backend id, missing backend-native id, unavailable readback source, still-present deleted id and ambiguous readback.
- Deferred implementation to the next phase.
- Kept production mutation closed.

---
## Phase 50.44 - SearchTimer workflow verified execution result integration

Status: Completed.

Summary:
- Integrated backend readback verification into `SearchTimerWorkflowExecutionResult`.
- Added `backendReadbackVerificationAttached` and `backendReadbackVerified` result semantics.
- Embedded `SearchTimerWorkflowBackendReadbackVerificationResult` into the workflow execution result.
- Propagated failed required readback verification into final workflow success and errors.
- Extended the execution result JSON contract with nested readback verification fields.
- Extended the execution result JSON serializer test coverage.
- Kept production mutation closed.

---
## Phase 50.43 - SearchTimer workflow update readback verification service

Status: Completed.

Summary:
- Added `SearchTimerWorkflowUpdateReadbackVerificationService`.
- Verified successful update readback through `ISearchTimerDataSource`.
- Matched updated SearchTimers by backend id, backend-native id, name, query and state.
- Reported unavailable data source, missing readback, missing backend-native id, unchanged content, wrong native id and ambiguous matches through the backend readback verification result model.
- Added a focused unit test and local test target.
- Added the target to the VDR local test group.
- Kept the service readback-only and independent from production mutation.

---
## Phase 50.42 - SearchTimer workflow create readback verification service

Status: Completed.

Summary:
- Added `SearchTimerWorkflowCreateReadbackVerificationService`.
- Verified successful create readback through `ISearchTimerDataSource`.
- Matched created SearchTimers by backend id, backend-native id when available, name, query and state.
- Reported unavailable data source, missing readback, wrong native id and ambiguous matches through the backend readback verification result model.
- Added a focused unit test and local test target.
- Added the target to the VDR local test group.
- Kept the service readback-only and independent from production mutation.

---
## Phase 50.41 - SearchTimer workflow backend readback verification result model

Status: Completed.

Summary:
- Added `SearchTimerWorkflowBackendReadbackVerificationResult` as the first backend readback verification domain result.
- Captured required, attempted, successful, matched and ambiguous states.
- Captured expected backend id, expected backend-native id and observed backend-native id.
- Added warnings, errors and audit trail support.
- Added explicit result states for not-required, unavailable, verified, failed and ambiguous readback.
- Added a focused unit test and local test target.
- Added the target to the VDR local test group.

---
## Phase 50.40 - SearchTimer workflow mandatory backend readback verification plan

Status: Completed.

Summary:
- Defined the mandatory backend readback verification plan for future SearchTimer production write execution.
- Established that a successful backend executor result alone is not enough when `requiresBackendReadback=true`.
- Defined create readback expectations for backend identity, backend-native id, query, state and relevant match options.
- Defined update readback expectations for missing, unchanged, partially applied or wrong-backend results.
- Deferred delete absence verification to a separate follow-up because it needs different ambiguity and timing rules.
- Identified the next required implementation step: a backend readback verification result model.

---
## Phase 50.39 - SearchTimer workflow local API smoke harness execution report

Status: Completed.

Summary:
- Documented the successful local VDR-Suite API smoke harness execution.
- Recorded that the harness ran on 127.0.0.1:18080.
- Recorded that the smoke script reached /api/searchtimers/real-test through the local VDR-Suite harness.
- Recorded HTTP 200, JSON parse success and RESULT: OK.
- Recorded the expected production-policy-gate-closed dispatch stage.
- Recorded executorInvocationAttempted=false and confirmed that no backend mutation was performed.

---
## Phase 50.38 - SearchTimer workflow local VDR-Suite API smoke harness

Status: Completed.

Summary:
- Added a local HTTP harness for the VDR-Suite SearchTimer real-test safety endpoint.
- Added a helper build target for the harness.
- Added an end-to-end local harness run target that starts the harness on 127.0.0.1:18080 and runs the Phase 50.36 smoke script against it.
- Kept backend mutation blocked through the production policy gate.
- Documented the distinction between direct RESTfulAPI lifecycle testing and VDR-Suite API safety-endpoint testing.

---
## Phase 50.37 - SearchTimer workflow yaVDR smoke-test execution report

Status: Completed.

Summary:
- Documented the first successful yaVDR-side direct RESTfulAPI SearchTimer real lifecycle smoke test.
- Recorded that RESTfulAPI on 127.0.0.1:8002 accepted SearchTimer create, readback, update, readback, delete and cleanup readback.
- Recorded that final cleanup returned an empty SearchTimer list.
- Documented that 127.0.0.1:8080 was VDR-Rectools/PHP, not the VDR-Suite API.
- Clarified that the VDR-Suite real-test endpoint still requires a local API harness or daemon runtime before live endpoint testing.

---
## Phase 50.36 - SearchTimer workflow yaVDR smoke-test script

Status: Completed.

Summary:
- Added tools/run_searchtimer_yavdr_real_test.py.
- Added a local self-test mode that validates expected response semantics without contacting a server.
- Added a run mode for POST /api/searchtimers/real-test.
- Added operator-visible output for status, final dispatch stage, warnings and audit trail.
- Added Makefile helper target searchtimer-yavdr-real-test-smoke-helper.
- Kept the script non-mutating by targeting the VDR-Suite real-test endpoint.

---

## Phase 50.35 - SearchTimer workflow yaVDR real-test mode

Status: Completed.

Summary:
- Added the non-mutating SearchTimer workflow yaVDR real-test endpoint.
- Added POST /api/searchtimers/real-test and POST /api/vdr/searchtimers/real-test.
- Reused the production safety chain while keeping the production policy gate closed.
- Returned normal workflow execution JSON with operator-visible warnings.
- Preserved executorInvocationAttempted=false in real-test mode.
- Preserved the audit trail so operators can see the final blocker.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.34 - SearchTimer workflow production policy gate

Status: Completed.

Summary:
- Added SearchTimerWorkflowProductionPolicyGate.
- Added productionPolicyGateConfigured to SearchTimerWorkflowCommandDispatchOptions.
- Updated real execution policy to require a production policy gate after switch, allowlist and permission.
- Updated real execution readiness review and JSON output with production policy allowed state.
- Updated production executor hardening plan to mark production policy gate as satisfied.
- Kept the production policy gate closed by default.
- Kept real VDR backend mutation out of scope.
- Prepared the next step: controlled yaVDR real-test mode.

---

## Phase 50.33 - SearchTimer workflow per-backend write permission gate

Status: Completed.

Summary:
- Added SearchTimerWorkflowBackendWritePermissionGate.
- Added backendWritePermissionGateEnabled and permitted backend IDs to SearchTimerWorkflowCommandDispatchOptions.
- Updated real execution policy to require per-backend write permission after backend write allowlist.
- Updated real execution readiness review and JSON output with backend write permission state.
- Updated production executor hardening plan to mark per-backend write permission as satisfied.
- Verified that non-permitted backend writes are blocked.
- Verified that permitted backend writes advance only to the next production gate.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.32 - SearchTimer workflow backend write allowlist

Status: Completed.

Summary:
- Added SearchTimerWorkflowBackendWriteAllowlist.
- Added backendWriteAllowlistEnabled and allowed backend IDs to SearchTimerWorkflowCommandDispatchOptions.
- Updated real execution policy to require a backend write allowlist after the production enablement switch.
- Updated real execution readiness review and JSON output with backend write allowlist state.
- Updated production executor hardening plan to mark backend write allowlist as satisfied.
- Verified that non-allowlisted backend writes are blocked.
- Verified that allowlisted backend writes advance only to the next production gate.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.31 - SearchTimer workflow real execution enablement switch

Status: Completed.

Summary:
- Added SearchTimerWorkflowRealExecutionEnablementSwitch.
- Added a disabled-by-default production real execution enablement switch contract.
- Added productionRealExecutionEnabled to SearchTimerWorkflowCommandDispatchOptions.
- Updated real execution policy to require the production enablement switch for non-controlled production execution.
- Updated real execution readiness review and JSON output with productionRealExecutionEnabled.
- Updated production executor hardening plan to mark the enable switch contract as satisfied.
- Kept production real execution blocked by remaining hardening requirements.
- Kept REST production execution out of scope.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.30 - SearchTimer workflow production executor hardening plan

Status: Completed.

Summary:
- Added SearchTimerWorkflowProductionExecutorHardeningPlan.
- Added SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer.
- Added a machine-readable production hardening checklist.
- Marked existing dry-run/prepare/execute separation, opt-in, executor injection, guard/kill-switch, controlled test path, audit trail and readiness review as satisfied.
- Marked production enable switch, backend write allowlist, per-backend permission, production policy gate, mandatory readback verification, failure compensation and REST production boundary as missing.
- Verified that readyForProductionExecution remains false.
- Kept all hardening-plan paths non-mutating.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.29 - SearchTimer workflow real backend execution readiness review

Status: Completed.

Summary:
- Added SearchTimerWorkflowRealExecutionReadinessReview.
- Added SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer.
- Added machine-readable readiness fields for plan executability, write-operation state, execute-mode state, confirmation, opt-in, executor injection, controlled-test-only state and production policy availability.
- Verified that prepare-mode plans are not production ready.
- Verified that a production-like injected executor path is still not production ready because the production policy gate is unavailable.
- Verified that the controlled test executor path is explicitly not production real execution.
- Kept all readiness review paths non-mutating.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.28 - SearchTimer workflow controlled invocation audit trail

Status: Completed.

Summary:
- Added executorInvocationAuditTrail to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Added audit trail construction to execute-mode command dispatch.
- Recorded policy, guard, kill-switch, invocation-attempt and result-mapping decisions.
- Added a controlled invocation audit trail unit test.
- Verified the controlled test executor path records a complete successful audit trail.
- Verified the ordinary denied executor path records why execution did not proceed.
- Preserved REST as non-mutating.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.27 - SearchTimer workflow controlled test executor invocation path

Status: Completed.

Summary:
- Added controlledTestExecutorInvocationEnabled to SearchTimerWorkflowCommandDispatchOptions.
- Added a controlled test-only dispatch path that can open the policy and kill-switch for an injected fake executor.
- Verified that create, update and delete workflow plans can invoke a controlled test executor and map the result to executed=true.
- Verified that the ordinary injected executor path remains policy-denied and does not call the executor.
- Kept the controlled test path out of REST.
- Preserved the standard REST path as non-mutating with executorInvocationAttempted=false, executorResultMapped=false and executed=false.
- Kept real VDR backend mutation out of scope.

---

## Phase 50.26 - SearchTimer workflow executor invocation kill-switch contract

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutorInvocationKillSwitch as the final contract boundary before any future executor invocation.
- Added executorInvocationKillSwitchOpen and executorInvocationKillSwitchPassed to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Integrated the closed kill-switch into execute-mode dispatch after guarded invocation evaluation.
- Verified that the closed kill-switch blocks a guard-passed decision.
- Verified that an explicitly opened kill-switch can allow the next step in a direct unit test without invoking a real executor.
- Kept standard REST and dispatch paths non-mutating with executorInvocationAttempted=false, executorResultMapped=false and executed=false.
- Preserved the synthetic mapper path for representing a future successful executor result.
- Kept backend mutation out of scope.

---

## Phase 50.25 - SearchTimer workflow executor invocation result mapping skeleton

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutorResultMapper for future create, update and delete executor results.
- Added executorResultMapped and executorResultSuccessful to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Verified that a synthetic successful executor result can map to executed=true outside the standard dispatch path.
- Verified failed executor results map to executor-result-failed.
- Kept standard dispatch and REST paths non-mutating with executorInvocationAttempted=false, executorResultMapped=false and executed=false.
- Preserved realExecutionEnabled=false and dryRunOnly=true in normal workflow execution.
- Kept backend mutation out of scope.

---

## Phase 50.24 - SearchTimer workflow guarded executor invocation contract

Status: Completed.

Summary:
- Added SearchTimerWorkflowGuardedExecutorInvocation as the contract boundary before any future executor call.
- Added executorInvocationGuardPassed and executorInvocationAttempted to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Integrated guarded invocation evaluation into execute-mode dispatch after command request mapping and real-execution policy evaluation.
- Kept executorInvocationAttempted=false in all paths.
- Verified the guard rejects non-execute, unmapped, missing opt-in, missing executor and policy-denied states.
- Verified a synthetic allowed policy decision can satisfy the guard contract without calling the executor.
- Preserved realExecutionEnabled=false, executed=false and dryRunOnly=true.
- Kept backend mutation out of scope.

---

## Phase 50.23 - SearchTimer workflow real executor injection skeleton

Status: Completed.

Summary:
- Added an injectable ISearchTimerCommandExecutor pointer to SearchTimerWorkflowCommandDispatchOptions.
- Added executorInjected to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Extended the real-execution policy to require both executor opt-in and an injected executor before reaching the policy-denied stage.
- Changed REST execute-with-opt-in behavior to stop at real-executor-injection-required because REST does not inject a real executor.
- Verified that an injected executor is detected but never called while policy remains denied.
- Preserved realExecutionPolicyAllowed=false, realExecutionEnabled=false, executed=false and dryRunOnly=true.
- Kept backend mutation out of scope.

---

## Phase 50.22 - SearchTimer workflow real executor policy boundary

Status: Completed.

Summary:
- Added SearchTimerWorkflowRealExecutionPolicy as a central policy boundary before any future real backend execution.
- Added SearchTimerWorkflowRealExecutionPolicyDecision for explicit allowed, stage, message and error semantics.
- Added realExecutionPolicyAllowed to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Routed executionMode=execute with executor opt-in through the policy boundary.
- Changed opt-in execute responses from real-execution-disabled to real-execution-policy-denied while real execution remains disabled.
- Preserved realExecutionEnabled=false, executed=false and dryRunOnly=true.
- Kept ISearchTimerCommandExecutor calls and backend mutation out of scope.
- Added direct policy coverage plus dispatch, serializer, controller and router assertions.

---

## Phase 50.21 - SearchTimer workflow executor opt-in REST contract

Status: Completed.

Summary:
- Exposed the executor opt-in boundary through the REST execution contract.
- Added REST parsing for executorOptIn, executorOptInEnabled, executorOptInProvided, enableExecutor and allowExecutor.
- Wired parsed REST opt-in into SearchTimerWorkflowCommandDispatchOptions.
- Preserved the existing explicit operator confirmation parsing.
- Verified that executionMode=execute without opt-in blocks at executor-opt-in-required.
- Verified that executionMode=execute with opt-in reaches real-execution-disabled while still not executing backend writes.
- Preserved realExecutionEnabled=false, executed=false and dryRunOnly=true.
- Kept ISearchTimerCommandExecutor calls and backend mutation out of scope.

---

## Phase 50.20 - SearchTimer workflow executor opt-in boundary

Status: Completed.

Summary:
- Added SearchTimerWorkflowCommandDispatchOptions as an explicit dispatch option boundary.
- Added executorOptInProvided to SearchTimerWorkflowExecutionResult and the execution-result JSON contract.
- Kept the existing bool confirmation dispatch entry point while adding an options-based dispatch entry point.
- Changed executionMode=execute without opt-in to block at executor-opt-in-required.
- Preserved realExecutionEnabled=false, executed=false and dryRunOnly=true even when executor opt-in is provided.
- Kept ISearchTimerCommandExecutor calls and backend mutation out of scope.
- Updated dispatch, serializer, controller and router coverage for executor opt-in semantics.
- Documented the executor opt-in boundary before future real execution wiring.

---

## Phase 50.19 - SearchTimer workflow execution mode contract

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutionMode with dryRun, prepare and execute semantics.
- Added execution mode storage to workflow requests, execution plans and execution results.
- Parsed executionMode and mode request fields in the workflow request parser.
- Serialized executionMode in execution-plan and execution-result JSON.
- Kept prepare as the default mode to preserve existing guarded dispatch behavior.
- Made dryRun avoid command-request mapping while remaining successful and dry-run-only.
- Made execute remain blocked while real backend command dispatch is disabled.
- Preserved realExecutionEnabled=false, executed=false and dryRunOnly=true for the guarded path.
- Updated parser, plan JSON, dispatch, result JSON, controller and router coverage.

---

## Phase 50.18 - SearchTimer workflow dispatch result semantics

Status: Completed.

Summary:
- Added explicit dispatch result semantics to SearchTimerWorkflowExecutionResult.
- Added commandRequestMapped, realExecutionEnabled and dispatchStage fields.
- Serialized the new fields in the execution-result JSON contract.
- Marked dispatch stages for validation blocking, confirmation blocking, read-only no-dispatch, command-request mapping and mapping failure.
- Preserved executed=false and dryRunOnly=true for the guarded dispatch skeleton.
- Updated dispatch, serializer, controller and router coverage for the new result semantics.
- Documented the dispatch result semantics for client behavior.

---

## Phase 50.17 - SearchTimer workflow dispatch REST wiring

Status: Completed.

Summary:
- Wired the guarded SearchTimerWorkflowCommandDispatchService into the REST execution path.
- Kept POST /api/searchtimers/execute and POST /api/vdr/searchtimers/execute on the same execution-result JSON contract.
- Preserved explicit operator confirmation handling from the existing REST execution boundary.
- Preserved dry-run-only execution semantics with executed=false.
- Kept ISearchTimerCommandExecutor calls and backend mutation out of the REST path.
- Updated controller and router coverage to verify dispatch skeleton messages.
- Added the dispatch service to the VDR source set used by router builds.

---

## Phase 50.16 - SearchTimer workflow execution command dispatch skeleton

Status: Completed.

Summary:
- Added SearchTimerWorkflowCommandDispatchService as a guarded command dispatch skeleton.
- Used SearchTimerWorkflowCommandRequestMapper to map write-operation plans to command requests.
- Preserved explicit operator confirmation before accepting write-operation dispatch plans.
- Kept ISearchTimerCommandExecutor calls and backend mutation out of scope.
- Kept accepted dispatch skeleton results dry-run-only with executed=false.
- Covered create, update, delete, read-only and invalid dispatch-plan behavior with a targeted test.
- Linked the dispatch skeleton target into the local VDR test group.

---

## Phase 50.15 - SearchTimer workflow execution command boundary

Status: Completed.

Summary:
- Added SearchTimerWorkflowCommandRequestMapper as the first workflow-to-command request boundary.
- Mapped executable create plans to SearchTimerCreateRequest.
- Mapped executable update plans to SearchTimerUpdateRequest.
- Mapped executable delete plans to SearchTimerDeleteRequest.
- Preserved backend identity, backend-native identity, name, query and active state according to operation needs.
- Kept ISearchTimerCommandExecutor calls and backend mutation out of scope.
- Added targeted command-request mapper coverage and linked the target into the local VDR test group.

---

## Phase 50.14 - SearchTimer workflow execution REST documentation

Status: Completed.

Summary:
- Expanded the SearchTimer workflow execution REST documentation for the guarded /execute skeleton.
- Documented canonical and /api/vdr execution endpoints.
- Documented request fields, operation aliases, read-only operations and write operations.
- Documented explicit operator confirmation aliases and accepted truthy values.
- Documented execution-result JSON semantics for blocked, invalid and accepted skeleton results.
- Documented the safety boundary that /execute does not call ISearchTimerCommandExecutor and does not mutate a backend.
- Moved the next focus to the SearchTimer workflow execution command boundary.

---

## Phase 50.13 - SearchTimer workflow execution REST skeleton

Status: Completed.

Summary:
- Added controller support for executing SearchTimer workflow plans through the guarded skeleton.
- Added POST /api/searchtimers/execute and POST /api/vdr/searchtimers/execute routes.
- Reused the workflow request parser, planning service, execution skeleton and execution-result JSON serializer.
- Required explicit operator confirmation before write plans are accepted by the skeleton.
- Kept ISearchTimerCommandExecutor and real backend mutation out of the REST skeleton.
- Covered blocked, accepted and invalid execution skeleton responses in controller tests.
- Covered both canonical and /api/vdr execution routes in router tests.

---

## Phase 50.12 - SearchTimer workflow execution result JSON contract

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutionResultJsonSerializer for guarded workflow execution results.
- Serialized success, executed, blocked, dry-run, confirmation and readback flags.
- Serialized operation, primary step, follow-up step, backend identity, message, warnings and errors.
- Preserved the safety distinction between accepted skeleton results and real backend execution.
- Covered blocked, accepted and invalid result JSON with a targeted unit test.

---

## Phase 50.11 - SearchTimer workflow execution skeleton

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutionResult as the first execution-result model for workflow plans.
- Added SearchTimerWorkflowExecutionService as a guarded skeleton that consumes execution plans.
- Blocked invalid and non-executable plans.
- Required explicit operator confirmation before accepting write-operation plans.
- Kept all accepted skeleton results dry-run-only with executed=false.
- Kept real backend mutation and command execution out of scope.
- Covered read-only, write-confirmation and invalid-plan behavior with a targeted unit test.

---

## Phase 50.10 - SearchTimer workflow planning REST endpoint

Status: Completed.

Summary:
- Added SearchTimerController::planSearchTimerWorkflow as a planning-only REST boundary.
- Added POST /api/searchtimers/plan and POST /api/vdr/searchtimers/plan router wiring.
- Returned SearchTimerWorkflowExecutionPlan JSON without executing backend writes.
- Preserved HTTP 200 planning responses for invalid workflow requests with non-executable plans.
- Covered controller and router behavior with targeted tests.

---

## Phase 50.9 - SearchTimer workflow planning JSON contract

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutionPlanJsonSerializer to serialize workflow execution plans for clients.
- Exposed planned operation, primary step, follow-up step, execution-work flags and read-only/write classifications.
- Exposed explicit operator confirmation and backend readback requirements.
- Preserved backend identity, backend-native identity, name, query and active state in the JSON contract.
- Kept the contract planning-only without executing backend writes.
- Covered the serializer with a targeted unit test.

---

## Phase 50.8 - SearchTimer workflow planning service

Status: Completed.

Summary:
- Added SearchTimerWorkflowPlanningService to turn SearchTimerWorkflowRequest instances into SearchTimerWorkflowExecutionPlan instances.
- Reused the workflow validation service before planning executable steps.
- Preserved non-executable plans for invalid requests.
- Preserved read-only versus write-operation classification and explicit operator confirmation requirements.
- Kept backend writes out of scope.
- Covered planning behavior with a targeted unit test.

---

## Phase 50.7 - SearchTimer workflow execution plan model

Status: Completed.

Summary:
- Added SearchTimerWorkflowExecutionPlan as a backend-neutral planning model for SearchTimer user workflow intent.
- Added execution steps for list, preview, create, readback, update and delete.
- Preserved backend identity, backend-native identity, name, query and active state in the plan.
- Marked write plans as requiring explicit operator confirmation.
- Added readback follow-up steps for create and update plans without executing backend writes.
- Covered the model with a targeted unit test.

---

## Phase 50.6 - SearchTimer workflow validation request parser extraction

Status: Completed.

Summary:
- Extracted SearchTimer workflow validation request-body parsing from SearchTimerController into SearchTimerWorkflowValidationRequestParser.
- Added a dedicated parser header, implementation and targeted unit test.
- Preserved operation/action aliases, backend identity, backend-native identity, name, query and active field parsing.
- Kept SearchTimerController responsible for validation orchestration and JSON response construction.
- Linked the parser into SearchTimer controller and API router tests.

---

## Phase 50.5 - SearchTimer workflow validation request documentation

Status: Completed.

Summary:
- Added SearchTimer workflow validation API documentation for request fields, operation names and response fields.
- Documented POST /api/searchtimers/validate and POST /api/vdr/searchtimers/validate.
- Added examples for list, preview, create, readback, update, delete and invalid update validation.
- Documented the validate-only safety boundary and client behavior before real create, update or delete execution.
- Linked the new API contract from the development documentation index.

---

## Phase 50.4 - SearchTimer workflow validation REST endpoint

Status: Completed.

Summary:
- Added a SearchTimer workflow validation controller entry point for JSON request bodies.
- Wired POST /api/searchtimers/validate and POST /api/vdr/searchtimers/validate in ApiRouter.
- Accepted workflow operation, backend identity, backend-native identity, name, query and active intent fields.
- Returned the stable SearchTimer workflow validation JSON contract without executing create, update or delete writes.
- Covered controller-level and router-level validation behavior with targeted tests.

---

## Phase 50.3 - SearchTimer workflow validation JSON contract

Status: Completed.

Summary:
- Added SearchTimerWorkflowValidationResultJsonSerializer for the SearchTimer workflow validation result.
- Exposed valid state, operation, read-only/write classification, readback-after-write intent, backend identity, warnings and errors as a stable JSON contract.
- Added operation string mapping for list, preview, create, readback, update, delete and unknown.
- Reused the local SearchTimer JSON serializer style with quoted string escaping and compact JSON output.
- Covered valid create, invalid update and JSON escaping behavior with a targeted unit test.

---

## Phase 50.2 - SearchTimer workflow validation service

Status: Completed.

Summary:
- Added SearchTimerWorkflowValidationResult as a structured validation result for user workflow intent.
- Added SearchTimerWorkflowValidationService to validate SearchTimer workflow requests before transport mapping or backend writes.
- Preserved backend identity, backend-native identity, read-only/write classification and readback-after-write intent in the validation result.
- Added errors for missing workflow operation, backend id, backend-native id, name and query according to the selected workflow operation.
- Added warnings for write operations and create/update readback recommendations.
- Covered the validation service with a targeted unit test.

---

## Phase 50.1 - SearchTimer workflow request model

Status: Completed.

Summary:
- Added a backend-neutral SearchTimerWorkflowRequest model for manual user workflow intent.
- Represented list, preview, create, readback, update and delete operations independently from raw RESTfulAPI transport fields.
- Added validation helpers for required backend identity, backend-native identity, name and query fields.
- Distinguished read-only workflow steps from write operations and marked create/update as requiring readback after write.
- Covered the workflow request model with a targeted unit test.

---

## Phase 50.0 - SearchTimer user workflow foundation

Status: Completed.

Summary:
- Documented the SearchTimer user workflow foundation as the start of the Phase 50 milestone.
- Defined the stable manual workflow for list, preview, create, readback, update and delete behavior.
- Established safety rules around explicit operator intent, backend identity, real VDR readback and capability visibility.
- Kept automation, conflict resolution, profile policy and full Live UI parity out of the initial workflow scope.
- Set Phase 50.1 as the SearchTimer workflow request model follow-up.

---

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

