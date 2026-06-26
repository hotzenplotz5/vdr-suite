# VDR-Suite

[![VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml)

## Project Overview

- German: [Projektübersicht](docs/project-overview.de.md)
- English: [Project Overview](docs/project-overview.en.md)

---

## What is VDR-Suite?

VDR-Suite is a modern multi-backend platform for VDR.

It provides a snapshot-based runtime, backend-neutral services, REST APIs, metadata foundations, search foundations and future automation features for VDR environments.

VDR remains the source of truth.

VDR-Suite complements VDR. It does not replace it.

---

<!-- PROJECT_PROGRESS_START -->
## Project Progress

Overall project progress:

    ██████████ 100%

Milestone progress:

    Core Runtime Foundation             ██████████ 100%  completed
    Multi-Backend Foundation            ██████████ 100%  completed
    Query Foundation                    ██████████ 100%  completed
    Action Foundation                   ██████████ 100%  completed
    Metadata Foundation                 ██████████ 100%  completed
    Documentation Foundation            ██████████ 100%  completed
    SearchTimer Backend Foundation      ██████████ 100%  completed
    SearchTimer User Workflow           ██████████ 100%  completed
    SearchTimer Runtime Mutation Policy ██████████ 100%  completed
    SearchTimer Preview EPG Performance █░░░░░░░░░  10%  in progress
    Live Plugin Parity Foundation       ██████████ 100%  completed
    Automation Foundation               ██████████ 100%  completed
    Federation Foundation               ░░░░░░░░░░   0%  planned
    Frontend Foundation                 ░░░░░░░░░░   0%  planned

Current milestone:

    Phase 54.3 - SearchTimer warm EPG cache implementation

Progress source: [Project Progress](docs/planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Project State

Latest completed implementation phase:

    Phase 54.2 - SearchTimer warm EPG cache architecture

Current documentation consolidation:

    Phase 54.2 - SearchTimer warm EPG cache architecture

Next major implementation milestone:

    Phase 54.3 - SearchTimer warm EPG cache implementation

Completed foundations:

- Core Platform Foundation
- VDR Backend Foundation
- Multi-Backend Foundation
- Snapshot Runtime Foundation
- Change Feed Foundation
- Live Transport Foundation
- Capability Foundation
- Recording Query Foundation
- Recording Action Foundation
- EPG Search Foundation
- Content Classification Foundation
- Person Metadata Foundation
- Recording Person Search Foundation
- Recording Character Search Foundation
- SearchTimer Backend Foundation
- Native Fuzzy SearchTimer Capability Validation
- SearchTimer User Workflow Foundation
- SearchTimer Runtime Mutation Policy

Current foundation in progress:

- SearchTimer Preview EPG Performance

---

## Major Implemented Areas

Runtime:

- Snapshot cache
- Snapshot access services
- Snapshot change feed
- Live transport foundation
- Runtime diagnostics

Multi-backend:

- Backend registry
- Backend-aware snapshots
- Backend-aware routing
- Backend-aware polling
- Multi-backend snapshot APIs

EPG:

- Selective EPG query architecture
- EPG search API
- Backend-neutral EPG services
- SearchTimer preview EPG cache strategy documented through ADR-0034

SearchTimer:

- SearchTimer backend provider and route foundation
- SearchTimer real payload validation
- SearchTimer real VDR compatibility validation
- Native fuzzy SearchTimer capability validation
- SearchTimer user workflow foundation completed with verified execution and production mutation closed
- SearchTimer preview comparison options verified against live VDR EPG input

Recordings:

- Recording query API
- Recording actions
- Validation and execution services
- Backend-aware recording handling

Metadata:

- Content classification architecture
- Genre foundations
- Person metadata foundations
- Recording person search
- Recording character search

---

## Architecture Principles

ADR-0021 defines a core runtime rule:

    Prefer selective backend queries
    before full-domain transfers.

ADR-0028 defines the content classification rule:

    Do not model genre as a single plain string.
    Preserve source-aware classification evidence.

ADR-0034 defines the SearchTimer preview EPG input rule:

    Do not fetch the full EPG dump during each interactive SearchTimer preview.
    Use a warm backend-scoped EPG cache with explicit readiness metadata.

Performance target:

Backend workload should be comparable to established VDR frontends such as live when equivalent information is requested.

---

## Roadmap Snapshot

- Phase 50 - SearchTimer User Workflow Foundation
- Phase 51 - Live Plugin Parity Foundation
- Phase 52 - SearchTimer Automation Foundation
- Phase 53 - Recommendation Foundation
- Phase 54 - SearchTimer Preview Runtime and EPG Input Performance
- Phase 55 - Backend Management and Client Administration Foundation
- Later - Profiles, Permissions and Policy
- Later - Unified Search Foundation
- Later - Content Knowledge Graph

---

## Documentation

Project status:

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Current Project Status](docs/development/current-status.md)
- [Roadmap](docs/planning/roadmap.md)
- [Completed Phases](docs/development/completed-phases.md)

Developer documentation:

- [Developer Documentation](docs/development/index.md)
- [Developer Onboarding](docs/development/developer-onboarding.md)
- [Architecture Map](docs/development/architecture-map.md)

Architecture:

- [Architecture Index](docs/architecture/index.md)
- [ADR Index](docs/adr/index.md)
- [ADR-0021 Selective Backend Query Strategy](docs/adr/ADR-0021-selective-backend-query-strategy.md)
- [ADR-0028 Content Classification Architecture](docs/adr/ADR-0028-content-classification-architecture.md)
- [ADR-0034 SearchTimer Warm EPG Cache and Change Invalidation](docs/adr/ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)

Metadata:

- [Person API](docs/development/person-api.md)
- [Genre Architecture](docs/development/genre-architecture.md)
- [Content Rating API](docs/development/content-rating-api.md)

Search:

- [EPG Search API](docs/development/epg-search-api.md)
- [SearchTimer Backend Contract](docs/development/searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](docs/development/searchtimer-real-payload-validation.md)
- [SearchTimer User Workflow Foundation](docs/development/searchtimer-user-workflow-foundation.md)
- [SearchTimer Preview EPG Cache Strategy](docs/development/searchtimer-preview-epg-cache-strategy.md)
- [EPGSearch Native Fuzzy Real Backend Validation](docs/development/epgsearch-native-fuzzy-real-backend-validation.md)

---

## Start Here

1. [Project Status Dashboard](docs/project-status-dashboard.md)
2. [Current Project Status](docs/development/current-status.md)
3. [Roadmap](docs/planning/roadmap.md)
4. [Completed Phases](docs/development/completed-phases.md)
5. [Project Overview](docs/project-overview.md)
6. [Documentation Index](docs/index.md)
7. [Developer Documentation](docs/development/index.md)
8. [Architecture Index](docs/architecture/index.md)
9. [ADR Index](docs/adr/index.md)

---

## Documentation Navigation Rule

The README is the repository entry point.

Every documentation page should provide navigation back to:

- [README](README.md)
- [Documentation Index](docs/index.md)
- [Project Overview](docs/project-overview.md)
- the local section index when applicable
