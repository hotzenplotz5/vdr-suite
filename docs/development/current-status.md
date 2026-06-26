# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Roadmap](../planning/roadmap.md)
- [Lazy Recording Loading](../planning/lazy-recording-loading.md)
- [Startup Snapshot Runtime Rule](startup-snapshot-runtime.md)
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

Overall foundation progress, not product completion:

    ███████░░░ 70%

Milestone progress:

    Core Runtime Foundation              ██████████ 100%  completed
    Multi-Backend Foundation             ██████████ 100%  completed
    Query Foundation                     ██████████ 100%  completed
    Action Foundation                    ██████████ 100%  completed
    Metadata Foundation                  ██████████ 100%  completed
    Documentation Foundation             ██████████ 100%  completed
    SearchTimer Backend Foundation       ██████████ 100%  completed
    SearchTimer User Workflow            ██████████ 100%  completed
    SearchTimer Runtime Mutation Policy  ██████████ 100%  completed
    SearchTimer Preview EPG Performance  █░░░░░░░░░  10%  in progress
    Lazy Recording Loading               █░░░░░░░░░  10%  in progress
    Live Plugin Parity Foundation        ██████████ 100%  completed
    Automation Foundation                ██████████ 100%  completed
    Federation Foundation                ░░░░░░░░░░   0%  planned
    Frontend Foundation                  ░░░░░░░░░░   0%  planned

Current milestone:

    Phase 54.3 - SearchTimer warm EPG cache implementation

Progress source: [Project Progress](../planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 54.3e - SearchTimer preview EPG input status contract
```

Current documentation consolidation state:

```text
Phase 54.3e - SearchTimer preview EPG input status contract
```

Next major implementation milestone:

```text
Phase 54.3 - SearchTimer warm EPG cache implementation
```

Required planned follow-up:

```text
Lazy Recording Loading and Backend-Scoped Recording Refresh
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
SearchTimer Runtime Mutation Policy
SearchTimer Warm EPG Cache Architecture
SearchTimer Preview EPG Input Status Contract
```

Current foundation in progress:

```text
SearchTimer Preview EPG Performance
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
- Person metadata, recording-person search and recording-character search foundations are implemented.
- SearchTimer route, daemon backend provider wiring and backend contract documentation are implemented.
- Native fuzzy SearchTimer backend capability validation is complete through operator refresh, capability report and persisted restore.
- SearchTimer user workflow, validation, planning, guarded execution and readback verification foundations are implemented.
- Phase 53.x preserves title-only SearchTimer compare fields across REST, workflow planning and command dispatch.
- Phase 54.1 fixes SearchTimer preview comparison-option handling and verifies title-only preview behavior against live VDR EPG input.
- Phase 54.2 accepts ADR-0034 for warm EPG cache input, change-state invalidation and future SSE-triggered refresh.
- Phase 54.3e verifies and documents the SearchTimer preview `epgInput` contract so non-ready EPG input is not hidden as a normal zero-match result.
- Phase 54.3 now has a runtime cache refresh foundation: backend-scoped refresh service, refresh service registry, refresh controller, explicit REST refresh route, daemon wiring and router regression coverage are implemented.
- ADR-0035 records that recordings are a heavy on-demand domain and must not be loaded synchronously for all backends during daemon startup.
- Startup snapshot runtime is implemented for the initial poll: status, timers, SearchTimer metadata and channels may load, while recordings and full EPG events remain excluded from startup.

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
- Initial PollingService startup snapshots intentionally skip recordings and full events so daemon startup remains lightweight.
- VdrEventQuery provides the first backend-neutral selective EPG query contract.
- Events and EPG are treated as heavy domains and are not automatically full-refreshed by default.
- Recordings are also a heavy domain for startup and multi-backend runtime planning; startup snapshots must not synchronously load recordings for every backend.
- EPG search operates over selective event windows and does not require a persistent full EPG mirror.
- SearchTimer preview exposes `epgInput.status`, `epgInput.available` and `epgInput.warnings`: ready empty input is a valid zero-result preview, while warming, stale, unknown and unavailable input is non-authoritative.
- SearchTimer preview EPG cache refresh uses backend-scoped selective event queries and is exposed through an explicit read-only refresh endpoint.
- Recording pages must eventually render before recordings are loaded and show backend-scoped loading state until the selected backend is ready.
- Recording actions use backend-native recording identity.
- Content classification uses source-aware evidence for genre, rating, metadata and policy work.
- Person architecture uses source-aware evidence, roles, confidence, normalized names, character names and provider references.

---

## Selective Backend Query Rule

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

Heavy domains must not use full-domain runtime refreshes as the default strategy.

Heavy domains currently include:

- EPG
- recordings
- metadata
- posters
- fanart
- preview data
- scraper-derived data

Preferred runtime strategies are:

- startup snapshots that exclude heavy domains
- channel-scoped queries
- time-window queries
- object-specific queries
- backend-scoped on-demand recording refreshes
- change-hint driven refreshes
- warm backend-scoped caches for interactive preview paths

Performance goal:

Backend workload should remain comparable to established VDR frontends such as live whenever equivalent information is requested.

Recording-specific startup rule:

- daemon startup must not synchronously load recordings for all configured backends
- recording pages must show backend-scoped loading state instead of assuming data is already available
- a ready empty recording cache is a valid empty list, but unknown/loading/unavailable/error is not

---

## Current Implemented API Areas

Implemented API areas include:

- snapshot-backed VDR read APIs
- backend registry and backend-aware read APIs
- selective EPG read APIs
- EPG search API
- SearchTimer backend provider and route foundations
- SearchTimer user workflow foundation
- SearchTimer preview EPG input status contract
- SearchTimer preview EPG cache explicit refresh API
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
Phase 54.3 - SearchTimer warm EPG cache implementation
```

The next implementation phase should continue the backend-scoped warm EPG cache for SearchTimer preview so interactive preview requests do not fetch the full RESTfulAPI EPG dump.

Implemented so far:

```text
SearchTimerPreviewEpgCacheRefreshService
SearchTimerPreviewEpgCacheRefreshServiceRegistry
SearchTimerPreviewEpgCacheRefreshController
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
```

Required planned follow-up:

```text
Lazy Recording Loading and Backend-Scoped Recording Refresh
```

This follow-up must ensure daemon startup remains lightweight and recordings are loaded only on demand for the selected backend with explicit loading/status feedback.

Important boundaries:

- keep VDR as the source of truth for VDR-owned state
- prefer selective EPG queries and warm cache refresh over full EPG transfers in interactive request paths
- do not synchronously load recordings for all backends during daemon startup
- keep backend-scoped recording refresh independent per backend
- keep metadata provider details behind provider boundaries
- keep policy enforcement out until dedicated profile and permission architecture exists
- build SearchTimer on top of existing EPG search and recording metadata foundations
- preserve backend identity for future multi-backend SearchTimer behavior
- treat SSE as a future invalidation signal, not as a full EPG payload stream

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
