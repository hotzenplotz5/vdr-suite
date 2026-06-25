# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite.

Implementation history belongs in [Completed Phases](completed-phases.md).

Future planning belongs in [Roadmap](../planning/roadmap.md).

---

## Project

VDR-Suite is a service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST APIs, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

```text
main
```

---

<!-- PROJECT_PROGRESS_START -->
## Project Progress

Overall project progress:

    ██████████ 100%

Milestone progress:

    Core Runtime Foundation         ██████████ 100%  completed
    Multi-Backend Foundation        ██████████ 100%  completed
    Query Foundation                ██████████ 100%  completed
    Action Foundation               ██████████ 100%  completed
    Metadata Foundation             ██████████ 100%  completed
    Documentation Foundation        ██████████ 100%  completed
    SearchTimer Backend Foundation  ██████████ 100%  completed
    SearchTimer User Workflow       ██████████ 100%  completed
    Live Plugin Parity Foundation   ██████████ 100%  completed
    Automation Foundation           ██████████ 100%  completed
    Federation Foundation           ░░░░░░░░░░   0%  planned
    Frontend Foundation             ░░░░░░░░░░   0%  planned

Current milestone:

    Phase 54.0 - Cross-backend search and federation foundation planning

Progress source: [Project Progress](../planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 53.8 - SearchTimer title-only workflow completion audit
```

Current documentation consolidation state:

```text
Phase 53.8 - SearchTimer title-only workflow completion audit
```

Next major implementation milestone:

```text
Phase 54.0 - Cross-backend search and federation foundation planning
```

Completed foundations:

```text
Core Platform Foundation
VDR Backend Foundation
Multi-Backend Foundation
Snapshot Runtime Foundation
Change Feed Foundation
Live Transport Foundation
Capability Foundation
Recording Query Foundation
Recording Action Foundation
EPG Search Foundation
Content Classification Foundation
Person Metadata Foundation
Recording Person Search Foundation
Recording Character Search Foundation
SearchTimer User Workflow Foundation
```

Current foundation in progress:

```text
Live Plugin Parity Foundation
```

Direct GitHub documentation synchronization should still be followed locally by:

```text
make test-docs
make test-phase
```

---

## Verification Summary

- Snapshot-based read architecture is completed for the current domain set.
- Backend registry, backend-aware snapshots and multi-backend snapshot summaries are implemented.
- Change feed and live transport foundations are implemented.
- Selective EPG REST APIs and the EPG search API are implemented.
- Recording query and recording action foundations are implemented.
- ADR-0028 documents the source-aware content classification architecture.
- Genre classification, resolution, canonical registry and localization foundations are implemented.
- Person metadata foundations are implemented for roles, sources, resolution, query, search, JSON and REST boundaries.
- Recording-person search is implemented over recording-attached person metadata.
- Recording character search is implemented through characterName filtering.
- Real yaVDR validation confirms RESTfulAPI additional_media actor availability.
- EPG person search has the result model foundation and still needs service, JSON, REST and real metadata validation.
- SearchTimer route and daemon backend provider wiring are implemented.
- SearchTimer backend contract documentation defines the current minimal domain and known epgsearch and Live-style expansion gaps.
- SearchTimer real payload validation workflow is documented and supported by a local capture helper.
- SearchTimer domain now models stable recording and schedule options from real RESTfulAPI payloads.
- Native fuzzy SearchTimer backend capability validation is complete through operator refresh, capability report and persisted restore.
- Phase 50.0 defines the SearchTimer user workflow foundation for manual list, preview, create, readback, update and delete behavior.
- Phase 50.1 adds the backend-neutral SearchTimerWorkflowRequest model for user workflow intent.
- Phase 50.2 adds SearchTimer workflow validation before transport mapping or backend writes.
- Phase 50.3 adds a stable JSON contract for SearchTimer workflow validation results.
- Phase 50.4 exposes SearchTimer workflow validation through POST /api/searchtimers/validate without executing writes.
- Phase 50.5 documents the SearchTimer workflow validation request and response contract for operators and clients.
- Phase 50.6 extracts SearchTimer workflow validation body parsing into a dedicated request parser boundary.
- Phase 50.7 adds a backend-neutral SearchTimer workflow execution plan model without executing backend writes.
- Phase 50.8 adds a planning service that turns workflow requests into execution plans without executing backend writes.
- Phase 50.9 adds a JSON contract for SearchTimer workflow execution plans so clients can inspect planned steps before execution.
- Phase 50.10 exposes SearchTimer workflow planning through REST without executing backend writes.
- Phase 50.11 adds a guarded SearchTimer workflow execution skeleton without backend mutation.
- Phase 50.12 adds a JSON contract for guarded SearchTimer workflow execution results.
- Phase 50.13 exposes the guarded execution skeleton through REST without backend mutation.
- Phase 50.14 documents the guarded execution REST contract, confirmation semantics and dry-run safety boundary.
- Phase 50.15 adds the workflow-to-command request mapping boundary without executing backend mutations.
- Phase 50.16 adds a guarded command dispatch skeleton that maps command requests without calling the executor.
- Phase 50.17 wires the guarded command dispatch skeleton into the REST execution path while preserving dry-run behavior.
- Phase 50.18 adds explicit dispatch result semantics for command request mapping, real execution state and dispatch stage.
- Phase 50.19 adds an explicit execution mode contract for dryRun, prepare and future execute behavior.
- Phase 50.20 adds an explicit executor opt-in boundary before future real backend execution can be wired.
- Phase 50.21 exposes executor opt-in parsing through the REST execution contract while preserving dry-run safety.
- Phase 50.22 adds a central real-execution policy boundary before any future backend mutation can be wired.
- Phase 50.23 adds a real executor injection skeleton while keeping the policy denied and backend mutation impossible.
- Phase 50.24 adds a guarded executor invocation contract while keeping invocationAttempted=false in all paths.
- Phase 50.25 adds executor-result mapping so a future successful executor result can be represented as executed=true without enabling mutation.
- Phase 50.26 adds the final executor invocation kill-switch contract while keeping the standard path non-mutating.
- Phase 50.27 adds a controlled test executor invocation path that proves the success chain without exposing REST or real backend mutation.
- Phase 50.28 adds an executor invocation audit trail for policy, guard, kill-switch, invocation and result-mapping decisions.
- Phase 50.29 adds a non-mutating real backend execution readiness review and keeps production readiness false.
- Phase 50.30 adds a machine-readable production executor hardening plan and keeps readyForProductionExecution=false.
- Phase 50.31 adds a disabled-by-default production real execution enablement switch contract.
- Phase 50.32 adds a backend write allowlist gate for production SearchTimer writes.
- Phase 50.33 adds a per-backend write permission gate after the allowlist.
- Phase 50.34 adds a closed-by-default production policy gate as the final safety gate before real yaVDR testing.
- Phase 50.29 adds a non-mutating real backend execution readiness review and keeps production readiness false.
- Phase 50.24 adds a guarded executor invocation contract while keeping invocationAttempted=false in all paths.
- Phase 50.50 closes the SearchTimer user workflow foundation while keeping production mutation closed.
- Phase 51.0 starts the Live plugin parity foundation with a source-audited gap matrix for VDR, epgsearch, RESTfulAPI, Live and VDR-Suite.
- Phase 51.1 adds the read-only Live parity discovery domain foundation for Extended EPG info, channel groups, blacklists and recording directories.
- Phase 51.1 also fixes daemon source wiring for SearchTimerWorkflowValidationRequestParser.
- Phase 51.2 adds the read-only Live parity discovery JSON contract for Extended EPG info, channel groups, blacklists and recording directories.
- Phase 51.3 adds the read-only Live parity discovery REST controller contract without router wiring or backend transport.
- Phase 51.4 adds the read-only Live parity discovery service/provider contract without backend transport.
- Phase 51.5 connects the Live parity discovery controller to the service contract while keeping router wiring and backend transport out.
- Phase 51.6 adds the read-only Live parity discovery ApiRouter contract with optional backend query parameter and missing-controller 503 behavior.
- Phase 51.7 wires Live parity discovery into the daemon runtime with a safe static empty provider and HTTP server smoke coverage.
- Phase 51.8 strengthens the HTTP smoke contract for SearchTimer discovery aliases, default backend handling and read-only POST rejection.
- Phase 51.9 defines the RESTfulAPI-facing provider contract and upstream endpoint boundary without enabling HTTP transport.
- Phase 51.10 closes the Live parity discovery foundation and freezes the pre-transport, read-only boundary before SearchTimer automation begins.
- Phase 52.0 starts SearchTimer automation with a planning-only architecture boundary and explicit no-mutation safety rules.
- Phase 52.1 adds the read-only SearchTimer automation evaluation plan model with dry-run-only and no-mutation guarantees.
- Phase 52.2 adds the read-only SearchTimer automation match candidate model without matching execution, timer proposal creation or backend mutation.
- Phase 52.3 adds the read-only SearchTimer automation duplicate detection model without automatic duplicate decisions, timer proposals or backend mutation.
- Phase 52.4 adds the read-only SearchTimer automation candidate timer proposal model without creating timers or enabling backend writes.
- Phase 52.5 adds the SearchTimer automation dry-run result JSON serializer without REST endpoints, scheduling or backend mutation.
- Phase 52.6 adds the SearchTimer automation read-only service boundary without matching execution, REST endpoints, scheduling or backend mutation.
- Phase 52.7 adds the SearchTimer automation REST preview contract and daemon wiring without scheduling or backend mutation.
- Phase 52.8 adds the SearchTimer automation daemon scheduling plan model without enabling a scheduler, background loop or backend mutation.
- Phase 52.9 adds the SearchTimer automation safety review and confirms that automation is currently preview-only, not scheduler-safe, not execution-safe and not mutation-safe.
- Phase 53.0 fixes RESTfulAPI SearchTimer field mapping so title-only requests map compareTitle/compareSubtitle/compareSummary to use_title/use_subtitle/use_description correctly.
- Phase 53.1 strengthens the SearchTimer create request parser contract for title-only and subtitle/summary search flag preservation.
- Phase 53.2 verifies the SearchTimer REST controller create path preserves title-only flags into the create service/executor handoff.
- Phase 53.3 strengthens the SearchTimer update request parser contract for title-only and subtitle/summary search flag preservation.
- Phase 53.4 verifies the SearchTimer REST controller update path preserves title-only flags into the update service/executor handoff.
- Phase 53.5 preserves title-only flags in workflow request parsing and execution plan construction.
- Phase 53.6 maps preserved title-only workflow plan fields into concrete create/update command requests.
- Phase 53.7 verifies controlled workflow dispatch preserves title-only command requests into the injected command executor.
- Phase 53.8 closes the SearchTimer title-only workflow chain with a completion audit and keeps production mutation gated.

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Backend identity is present in snapshot change feed entries, snapshot read metadata and cached snapshots.
- Backend registry service, serializer and controller expose backend identity through service and REST boundaries.
- Snapshot cache can store snapshots per backend while preserving the legacy single-snapshot interface.
- Snapshot access and snapshot read services support backend-aware reads.
- VDR controller exposes default VDR reads, backend-specific reads and multi-backend snapshot summary reads.
- PollingService and BackendPollingCoordinator support backend-aware polling coordination.
- VdrEventQuery provides the first backend-neutral selective EPG query contract.
- Events and EPG are treated as heavy domains and are not automatically full-refreshed by default.
- EPG search operates over selective event windows and does not require a persistent full EPG mirror.
- Recording actions use backend-native recording identity.
- Content classification uses source-aware evidence for genre, rating, metadata and policy work.
- Person architecture uses source-aware evidence, roles, confidence, normalized names, character names and provider references.
- Recording-person search preserves both matched person metadata and recording context.
- Character search uses TVScraper actor role data mapped to Person.characterName.
- EPG person search currently has the result model foundation and still needs service, JSON, REST and real metadata validation.

---

## Selective Backend Query Rule

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

Heavy domains must not use full-domain runtime refreshes as the default strategy.

Heavy domains currently include:

- EPG
- metadata
- posters
- fanart
- preview data
- scraper-derived data

Preferred runtime strategies are:

- channel-scoped queries
- time-window queries
- object-specific queries
- change-hint driven refreshes

Performance goal:

Backend workload should remain comparable to established VDR frontends such as live whenever equivalent information is requested.

---

## Current Implemented API Areas

Implemented API areas include:

- snapshot-backed VDR read APIs
- backend registry and backend-aware read APIs
- selective EPG read APIs
- EPG search API
- SearchTimer backend provider and route foundations
- SearchTimer user workflow foundation
- recording query API
- recording action validation and execution APIs
- person query APIs
- recording-person search APIs
- recording character search through characterName
- dashboard, jobs, metadata and runtime diagnostics APIs

Detailed route contracts belong in the domain-specific API documentation files.

---

## Current Test Runtime Observation

The full local regression target is intentionally no longer the default verification path for normal development work.

Recommended local verification for architecture and documentation work:

```text
make test-docs
make test-phase
make daemon
```

Targeted local tests remain useful for narrow changes and for real VDR validation.

Real VDR tests are reserved for backend integration, RESTfulAPI validation, SSE streams, polling, snapshot runtime, multi-VDR scenarios and real metadata availability checks.

---

## Next Technical Focus

```text
Phase 54.0 - Cross-backend search and federation foundation planning
```

The next implementation phase should begin the cross-backend search and federation foundation with planning-only architecture documentation.

Important boundaries:

- keep VDR as the source of truth for VDR-owned state
- prefer selective EPG queries over persistent full EPG mirroring
- keep metadata provider details behind provider boundaries
- keep policy enforcement out until dedicated profile and permission architecture exists
- build SearchTimer on top of existing EPG search and recording metadata foundations
- preserve backend identity for future multi-backend SearchTimer behavior

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- SearchTimer foundation work is synchronized through the latest verified implementation phase.
- Documentation synchronization now tracks the real implementation state before real integration testing starts.
