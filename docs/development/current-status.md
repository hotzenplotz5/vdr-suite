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

    ████████░░ 77%

Milestone progress:

    Core Runtime Foundation   ██████████ 100%  completed
    Multi-Backend Foundation  ██████████ 100%  completed
    Query Foundation          ██████████ 100%  completed
    Action Foundation         ██████████ 100%  completed
    Metadata Foundation       ██████████ 100%  completed
    Documentation Foundation  ██████████ 100%  completed
    SearchTimer Foundation    ██████████  95%  in progress
    Automation Foundation     ░░░░░░░░░░   0%  planned
    Federation Foundation     ░░░░░░░░░░   0%  planned
    Frontend Foundation       ░░░░░░░░░░   0%  planned

Current milestone:

    Phase 49.5 - EPGSearch parameter regression expansion

Progress source: [Project Progress](../planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 49.16 - EPGSearch native fuzzy runtime capability wiring
```

Current documentation consolidation state:

```text
Phase 46.41 - Current Project Status Refresh
```

Next major implementation milestone:

```text
Phase 49.17 - EPGSearch native fuzzy capability persistence
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
```

Current foundation in progress:

```text
EPG Person Search Foundation
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
- EPG person search has started with the result model foundation.
- SearchTimer route and daemon backend provider wiring are implemented.
- SearchTimer backend contract documentation defines the current minimal domain and known epgsearch and Live-style expansion gaps.
- SearchTimer real payload validation workflow is documented and supported by a local capture helper.
- SearchTimer domain now models stable recording and schedule options from real RESTfulAPI payloads.

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
Phase 49.17 - EPGSearch native fuzzy capability persistence
```

The next major implementation milestone is SearchTimer. It should turn existing EPG search, recording search and metadata search foundations into persistent search rules and later automation.

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
