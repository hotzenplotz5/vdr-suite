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
Phase 54.3e - SearchTimer preview EPG input status contract

Documentation consolidation step
Phase 54.3e - SearchTimer preview EPG input status contract

Next major implementation milestone
Phase 54.3 - SearchTimer warm EPG cache implementation
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

### SearchTimer Backend and Capability Foundation

Status: Completed.

Primary result:
- Establishes the backend-facing SearchTimer foundation and validates native fuzzy backend capability behavior through real VDR-Suite and RESTfulAPI workflows.

Key outcomes:
- SearchTimer route and daemon backend provider wiring.
- SearchTimer backend contract documentation.
- Real payload capture and validation workflow.
- Expanded SearchTimer domain model for stable recording and schedule options.
- Native fuzzy operator refresh validation.
- Capability report validation for epg.search.fuzzy.native.
- Persisted native fuzzy restore validation after daemon restart.
- Consolidated native fuzzy validation helper workflow and mutation boundaries.

Representative phase range:
- Phase 47.x through Phase 49.x.

---
### SearchTimer User Workflow Foundation

Status: Completed.

Primary result:
- Turns the backend-facing SearchTimer and native fuzzy capability foundation into a practical, tested user workflow foundation.

Key outcomes:
- SearchTimer workflow request, validation, planning, dispatch, controlled execution and readback verification are implemented.
- REST-facing verified execution response contracts are documented and tested.
- Production mutation remains closed until a later explicitly gated phase.
- Phase 50.50 closes the foundation cleanly before Live parity work starts.

Representative phase range:
- Phase 50.0 through Phase 50.50.

---
## Current Milestone

### Phase 51 - Live Plugin Parity Foundation

Status: Completed.

Goal:
- Approach the practical information quality of the VDR Live plugin while keeping VDR-Suite multi-backend and API-first.

Phase 51.0 result:
- Source-audited the ownership split between VDR core, epgsearch, RESTfulAPI, Live and VDR-Suite.
- Established a Live parity gap matrix for information quality, helper lists, timer conflicts and SearchTimer preview parity.
- Confirmed that RESTfulAPI integration and extension remains the preferred path over a VDR fork.
- Kept production mutation closed.

Phase 51.1 result:
- Added the read-only Live parity discovery domain foundation.
- Modeled Extended EPG info, channel groups, blacklists and recording directories as backend-neutral domain objects.
- Fixed daemon source wiring for SearchTimerWorkflowValidationRequestParser.
- Kept the phase domain-only with no REST route, no adapter transport and no backend mutation.

Phase 51.2 result:
- Added a stable JSON serializer for the Live parity discovery catalog.
- Serialized backendId, counts, Extended EPG info, channel groups, blacklists and recording directories.
- Kept the phase read-only with no REST route, no backend transport and no backend mutation.

Phase 51.3 result:
- Added SearchTimerDiscoveryController as a read-only ApiResponse boundary.
- Exposed a supplied SearchTimerDiscoveryCatalog through the stable discovery JSON serializer.
- Covered statusCode, contentType and JSON body contract with a targeted controller test.
- Kept the phase free of ApiRouter wiring, backend transport and backend mutation.

Phase 51.4 result:
- Added ISearchTimerDiscoveryProvider as the read-only discovery provider boundary.
- Added SearchTimerDiscoveryService to delegate backend-aware discovery to the provider.
- Covered the service contract with an in-memory provider unit test.
- Kept the phase free of controller rewiring, ApiRouter wiring, backend transport and backend mutation.

Phase 51.5 result:
- Added a service-backed SearchTimerDiscoveryController constructor.
- Added getDiscovery(backendId) to delegate discovery to SearchTimerDiscoveryService.
- Preserved the direct catalog-based controller path.
- Covered catalog path, service path and missing-service behavior with the controller test.
- Kept the phase free of ApiRouter wiring, backend transport and backend mutation.

Phase 51.6 result:
- Added read-only ApiRouter route contract for /api/searchtimers/discovery and /api/vdr/searchtimers/discovery.
- Added optional backend query parameter handling with default backend fallback.
- Added missing discovery controller 503 behavior.
- Covered the route through test-api-router with an in-memory discovery provider.
- Kept the phase free of daemon provider wiring, backend transport and backend mutation.

Phase 51.7 result:
- Added SearchTimerDiscoveryStaticProvider as a safe empty daemon provider.
- Wired provider, service, serializer and controller into DaemonRuntime.
- Injected SearchTimerDiscoveryController into ApiRouter during daemon initialization.
- Added TestHttpServer smoke coverage for /api/searchtimers/discovery.
- Kept the phase free of RESTfulAPI transport, epgsearch fetching and backend mutation.

Phase 51.8 result:
- Strengthened TestHttpServer smoke coverage for /api/searchtimers/discovery and /api/vdr/searchtimers/discovery.
- Verified explicit backend query propagation and default backend fallback.
- Verified the exact empty-provider JSON response body.
- Verified POST /api/searchtimers/discovery remains unavailable through the generic not-found response.
- Kept the phase free of RESTfulAPI transport, epgsearch fetching and backend mutation.

Phase 51.9 result:
- Added RestfulApiSearchTimerDiscoveryProvider as the RESTfulAPI-facing discovery provider contract.
- Defined /searchtimers/discovery.json as the upstream endpoint contract.
- Preserved safe empty pre-transport discovery behavior.
- Verified configured backend fallback and explicit backend override.
- Kept the phase free of IHttpClient, HTTP execution, JSON parsing, epgsearch fetching and backend mutation.

Phase 51.10 result:
- Closed the Live parity discovery foundation.
- Froze the pre-transport read-only discovery boundary before Phase 52.
- Confirmed the stable discovery API surface and JSON response shape.
- Preserved the no-HTTP-execute, no-epgsearch-fetching and no-mutation boundary.
- Handed off to SearchTimer automation planning.

Next implementation step:
- Phase 52.0 - SearchTimer automation foundation planning.

Completed outcomes:
- Broader EPG detail coverage prepared through explicit discovery catalog fields.
- Timer and SearchTimer helper discovery surfaces prepared.
- SearchTimer preview parity analysis remains separated from mutation.
- Timer conflict visibility remains a later read-only surface.
- Recording metadata visibility remains aligned with backend-aware frontend-ready APIs.
- Backend-aware frontend-ready discovery API surface established.

Important boundaries:
- Keep VDR as the source of truth for VDR-owned state.
- Keep epgsearch semantics behind explicit SearchTimer and EPGSearch boundaries.
- Prefer RESTfulAPI integration and extension over a VDR fork.
- Keep automatic SearchTimer evaluation out until Phase 52.
- Keep production mutation closed until explicitly reopened by a dedicated gated phase.

---
## Planned Major Milestones

### Phase 52 - SearchTimer Automation Foundation

Status: In Progress.

Goal:
- Extend SearchTimer from stored search rules to automated evaluation and action preparation.

Phase 52.0 result:
- Started SearchTimer automation as a planning-only foundation.
- Defined evaluation planning, matching, duplicate analysis, proposal, validation and execution handoff as separate steps.
- Preserved the no-real-timer-mutation and no-epgsearch-mutation boundary.
- Set Phase 52.1 as the first read-only model phase.

Phase 52.1 result:
- Added SearchTimerAutomationEvaluationPlan as the read-only automation planning model.
- Defined backend id, candidate limit and read-only inclusion flags.
- Hard-coded dry-run-only, no-mutation and no-scheduled-execution guarantees.
- Covered validation and safety invariants with a targeted unit test.
- Kept the phase free of EPG matching, duplicate detection, candidate creation, REST endpoints, daemon scheduling and mutation.

Phase 52.2 result:
- Added SearchTimerAutomationMatchCandidate as the read-only candidate model.
- Modeled backend id, SearchTimer id, EPG event id, event metadata, score and match reasons.
- Preserved dry-run-only, no-mutation, no-timer-proposal and duplicate-check-required invariants.
- Covered validation, clamping and safety behavior with a targeted unit test.
- Kept the phase free of matching execution, duplicate decisions, candidate timer creation, REST endpoints, daemon scheduling and mutation.

Phase 52.3 result:
- Added SearchTimerAutomationDuplicateDetection as the read-only duplicate risk model.
- Modeled duplicate risk level, existing timer reference, existing recording reference, title similarity, overlap and duplicate reasons.
- Preserved dry-run-only, no-mutation, no-automatic-decision and no-timer-proposal invariants.
- Covered validation, risk behavior, clamping and safety behavior with a targeted unit test.
- Kept the phase free of duplicate services, automatic decisions, candidate timer creation, REST endpoints, daemon scheduling and mutation.

Phase 52.4 result:
- Added SearchTimerAutomationCandidateTimerProposal as the read-only timer proposal model.
- Modeled proposed start/end time, margins, directory, priority, lifetime, duplicate risk and block reasons.
- Preserved dry-run-only, no-mutation, no-timer-creation, no-backend-write and no-automatic-execution invariants.
- Covered proposal construction, duplicate blocking, validation, clamping and safety behavior with a targeted unit test.
- Kept the phase free of proposal services, serializers, REST endpoints, daemon scheduling and backend mutation.

Phase 52.5 result:
- Added SearchTimerAutomationDryRunResult as the read-only dry-run aggregate.
- Added SearchTimerAutomationDryRunResultJsonSerializer for frontend-visible dry-run JSON.
- Serialized evaluation plan state, match candidates, duplicate detections, candidate timer proposals, warnings, errors and audit trail.
- Preserved dry-run-only, no-mutation, no-timer-creation, no-backend-write and no-automatic-execution invariants.
- Kept the phase free of services, REST endpoints, daemon scheduling and backend mutation.

Phase 52.6 result:
- Added SearchTimerAutomationReadOnlyService as the read-only service boundary.
- Aggregated supplied plans, match candidates, duplicate detections and candidate timer proposals into dry-run results.
- Added warnings and audit entries for boundary-level validation and count mismatches.
- Preserved dry-run-only, no-mutation, no-timer-creation, no-backend-write and no-automatic-execution invariants.
- Kept the phase free of matching execution, REST endpoints, daemon scheduling and backend mutation.

Phase 52.7 result:
- Added SearchTimerAutomationPreviewController.
- Added read-only REST preview endpoints for SearchTimer automation dry-runs.
- Wired the preview controller into ApiRouter and DaemonRuntime.
- Preserved dry-run-only, no-mutation, no-timer-creation, no-backend-write and no-automatic-execution invariants.
- Kept the phase free of matching execution, scheduling and backend mutation.

Phase 52.8 result:
- Added SearchTimerAutomationDaemonSchedulingPlan.
- Captured disabled and preview-only scheduling plans for future daemon automation.
- Preserved scheduler-disabled, dry-run-only, no-mutation, no-timer-creation, no-backend-write and no-automatic-execution invariants.
- Covered interval, candidate limit, snapshot freshness and safety audit behavior with a targeted unit test.
- Kept the phase free of daemon scheduler runtime, background loops, RESTfulAPI writes and epgsearch mutation.

Phase 52.9 result:
- Added SearchTimerAutomationSafetyReview.
- Consolidated dry-run, scheduling, execution and mutation safety boundaries.
- Confirmed preview-only safety while keeping scheduling runtime, automatic execution and backend mutation explicitly blocked.
- Covered safe preview and invalid-review paths with a targeted unit test.
- Kept the phase free of daemon scheduler runtime, background loops, RESTfulAPI writes and epgsearch mutation.

Phase 53.0 result:
- Fixed RESTfulAPI SearchTimer field mapping for title/subtitle/description search flags.
- Mapped compareTitle to use_title.
- Mapped compareSubtitle to use_subtitle.
- Mapped compareSummary to use_description.
- Added executor-level request body tests for title-only and subtitle/summary search field combinations.
- Kept the phase free of scheduler, automation execution and backend write policy changes.

Phase 53.1 result:
- Strengthened SearchTimer create request parser coverage for title-only JSON bodies.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false are preserved.
- Verified subtitle/summary search flag combinations are preserved.
- Added a dedicated parser test target to the local test group.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

Phase 53.2 result:
- Strengthened SearchTimer REST controller create-path coverage for title-only JSON bodies.
- Captured the create request handed to the test command executor.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false survive the controller handoff.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

Phase 53.3 result:
- Strengthened SearchTimer update request parser coverage for title-only JSON bodies.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false are preserved.
- Verified subtitle/summary search flag combinations are preserved.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

Phase 53.4 result:
- Strengthened SearchTimer REST controller update-path coverage for title-only JSON bodies.
- Captured the update request handed to the test command executor.
- Verified compareTitle=true, compareSubtitle=false and compareSummary=false survive the controller handoff.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

Phase 53.5 result:
- Added title-only compare-field preservation to SearchTimerWorkflowRequest.
- Parsed compareTitle/compareSubtitle/compareSummary/compareCategories in workflow validation requests.
- Preserved compare fields in SearchTimerWorkflowExecutionPlan::fromRequest.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

Phase 53.6 result:
- Mapped preserved workflow plan compare fields into SearchTimerCreateRequest.
- Mapped preserved workflow plan compare fields into SearchTimerUpdateRequest.
- Added title-only command request mapper coverage for workflow create and update.
- Kept the phase free of runtime behavior, scheduler and backend write policy changes.

Phase 53.7 result:
- Verified controlled workflow create dispatch preserves title-only compare fields into the injected command executor.
- Verified controlled workflow update dispatch preserves title-only compare fields into the injected command executor.
- Kept production backend mutation gated by execution mode, policy, guard and kill switch controls.

Phase 53.8 result:
- Closed the SearchTimer title-only workflow chain with a completion audit.
- Documented direct REST and workflow-path title-only preservation.
- Confirmed production mutation remains gated and no scheduler/runtime automation was enabled.
- Handed off to the cross-backend search and federation foundation.

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

### Phase 54 - SearchTimer Preview Runtime and EPG Input Performance

Status: In Progress.

Goal:
- Make SearchTimer preview correct and fast enough for operator-facing use against real VDR EPG data.

Completed results:
- Phase 54.0 wires runtime mutation policy execution while keeping the runtime gate closed by default.
- Phase 54.1 fixes SearchTimer preview comparison options and verifies title-only matching against live VDR EPG input.
- Phase 54.2 accepts ADR-0034 for warm EPG cache input, change-state invalidation and future SSE-triggered refresh.

Next implementation step:
- Phase 54.3 - SearchTimer warm EPG cache implementation.

Expected outcomes:
- Backend-scoped warm EPG cache for SearchTimer preview.
- Explicit cache readiness and stale-state diagnostics.
- No full RESTfulAPI EPG dump per interactive preview request.
- Change-state driven cache invalidation through polling first.
- Future RESTfulAPI SSE change stream support as an invalidation signal.
- Cross-backend SearchTimer preview can build on the warm-cache path later.

---

### Phase 55 - Backend Management and Client Administration Foundation

Status: Planned.

Goal:
- Turn backend configuration, capability visibility and backend health into manageable client-facing concepts.

Expected outcomes:
- Backend management API.
- Backend capability visibility.
- Backend health and diagnostics.
- Backend connection validation.
- Preparation for frontend backend administration.

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


### Phase 54.0 result: SearchTimer runtime mutation policy wiring

- Runtime Create/Update/Delete services are available to the SearchTimer controller.
- Direct API mutation is guarded by a closed runtime policy executor.

### Phase 54.1 result: SearchTimer preview comparison-option fix

- SearchTimerPreviewService now respects compareTitle, compareSubtitle and compareSummary.
- SearchTimer preview now respects limit and offset.
- Real yaVDR validation confirmed title-only SearchTimer preview produces the expected Treffer set for "Amerika".

### Phase 54.2 result: SearchTimer warm EPG cache architecture

- ADR-0034 defines that interactive SearchTimer preview must not fetch the full RESTfulAPI EPG dump per request.
- The next implementation step is a backend-scoped warm EPG cache with explicit readiness diagnostics.
- Polling-based change-state invalidation comes first; RESTfulAPI SSE remains a later invalidation trigger.
