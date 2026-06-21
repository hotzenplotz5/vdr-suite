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

    Phase 47.0 - SearchTimer Foundation

Progress source: [Project Progress](docs/planning/project-progress.md)
<!-- PROJECT_PROGRESS_END -->

---

## Current Project State

Latest completed implementation phase:

    Phase 47.39 - SearchTimer foundation progress

Current documentation consolidation:

    Phase 46.42 - README Refresh

Next major implementation milestone:

    Phase 47.40 - SearchTimer documentation synchronization

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

Current foundation in progress:

- EPG Person Search Foundation

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

Performance target:

Backend workload should be comparable to established VDR frontends such as live when equivalent information is requested.

---

## Roadmap Snapshot

- Phase 47 - SearchTimer Foundation
- Phase 48 - Unified Search Foundation
- Phase 49 - Profiles, Permissions and Policy
- Phase 50 - Backend Management Foundation
- Phase 51 - Live Plugin Parity Foundation
- Phase 52 - SearchTimer Automation
- Phase 53 - Recommendation Foundation
- Phase 54 - Cross Backend Search and Federation
- Phase 55 - Content Knowledge Graph

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

Metadata:

- [Person API](docs/development/person-api.md)
- [Genre Architecture](docs/development/genre-architecture.md)
- [Content Rating API](docs/development/content-rating-api.md)

Search:

- [EPG Search API](docs/development/epg-search-api.md)

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
