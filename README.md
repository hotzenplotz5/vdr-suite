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
## Project State Snapshot

This is a verified implementation-state snapshot, not a product-completion percentage.

### Verified foundations

- Core runtime and daemon foundation
- VDR backend adapter and RESTfulAPI integration foundation
- Backend registry and multi-backend runtime foundation
- Snapshot cache, snapshot access and change-feed foundation
- REST routing and JSON response boundaries
- Recording query foundation
- Recording action validation and guarded execution foundation
- Selective EPG query and EPG search foundation
- Content classification and person metadata foundations
- Recording person and character search foundations
- SearchTimer backend, validation, planning and workflow foundations
- SearchTimer safety gates, readback verification and production mutation policy foundations
- Live parity discovery foundation
- Real VDR acceptance manifest and runner foundation
- Daemon lifecycle hardening for duplicate bind failures and SIGTERM shutdown

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.

### Guarded or deliberately incomplete areas

- SearchTimer production mutation remains gated and closed by default.
- Recording operations need a dedicated safety audit before destructive real-backend probes are expanded.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- Suite-owned metadata database and external scraper/provider strategy are planned but not yet implemented as the final metadata product.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy are still planned.
- Web, Windows, Android, iOS and TV frontends are planned product layers, not completed foundations.

### Current active focus

```text
Phase 55.6 - Recording operations audit and safety policy
```

### Later strategic milestones

- Library boundary and packaging strategy
- Multi-site backend federation and permissions
- Frontend foundation
- Suite metadata database and external provider integration
- Safe production-grade recording operations

Progress source: docs/planning/project-progress.md
<!-- PROJECT_PROGRESS_END -->

---

## Current Project State

Latest completed implementation phase:

    Phase 55.5m - Documentation consolidation and roadmap alignment

Current documentation consolidation:

    Phase 55.5m - Documentation consolidation and roadmap alignment

Next major implementation milestone:

    Phase 55.6 - Recording operations audit and safety policy

Required planned follow-up:

    Recording operations audit and safety policy

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
- Real VDR Acceptance Foundation
- Daemon Runtime Lifecycle Hardening
- Documentation Handoff Verification

Current foundation in progress:

- Recording Operations Audit and Safety Policy

---

## Major Implemented Areas

Runtime:

- Snapshot cache
- Snapshot access services
- Snapshot change feed
- Live transport foundation
- Runtime diagnostics
- Real VDR acceptance manifest and runner
- HTTP listener bind-failure hardening
- SIGTERM daemon shutdown hardening

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
- SearchTimer workflow validation and planning covered by real VDR acceptance

Recordings:

- Recording query API
- Recording actions
- Validation and execution services
- Backend-aware recording handling
- Lazy recording loading planned through ADR-0035

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

ADR-0035 defines the lazy recording loading rule:

    Do not synchronously load recordings for all backends during daemon startup.
    Load recordings backend-by-backend on demand with explicit loading state.

Performance target:

Backend workload should be comparable to established VDR frontends such as live when equivalent information is requested.

---

## Roadmap Snapshot

- Phase 50 - SearchTimer User Workflow Foundation
- Phase 51 - Live Plugin Parity Foundation
- Phase 52 - SearchTimer Automation Foundation
- Phase 53 - Recommendation Foundation
- Phase 54 - SearchTimer Preview Runtime and EPG Input Performance
- Lazy Recording Loading - no startup recording load, backend-scoped on-demand refresh
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
- [Lazy Recording Loading](docs/planning/lazy-recording-loading.md)
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
- [ADR-0035 Lazy Recording Loading and Backend-Scoped Refresh](docs/adr/ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)

Metadata:

- [Person API](docs/development/person-api.md)
- [Genre Architecture](docs/development/genre-architecture.md)
- [Content Rating API](docs/development/content-rating-api.md)

Search:

- [EPG Search API](docs/development/epg-search-api.md)
- [SearchTimer Backend Contract](docs/development/searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](docs/development/searchtimer-real-payload-validation.md)
- [SearchTimer User Workflow Foundation](docs/development/searchtimer-user-workflow-foundation.md)
