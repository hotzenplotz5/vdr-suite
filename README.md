# VDR-Suite

[![VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml)

## Project Overview

- German: [Projektübersicht](docs/project-overview.de.md)
- English: [Project Overview](docs/project-overview.en.md)

## Quick Links

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Current Project Status](docs/development/current-status.md)
- [Developer Documentation](docs/development/index.md)
- [Real Recording Action End-to-End Validation](docs/development/real-recording-action-e2e-validation.md)
- [Phase 44 Recording Action Runtime Completion](docs/development/phase-44-recording-action-runtime-completion.md)
- [Phase 45 EPG Search Architecture](docs/development/phase-45-epg-search-architecture.md)
- [EPG Search API](docs/development/epg-search-api.md)
- [Genre Architecture](docs/development/genre-architecture.md)
- [ADR-0028 Content Classification Architecture](docs/adr/ADR-0028-content-classification-architecture.md)
- [Developer Onboarding](docs/development/developer-onboarding.md)
- [Architecture Map](docs/development/architecture-map.md)
- [Roadmap](docs/planning/roadmap.md)
- [Architecture](docs/architecture/index.md)
- [ADR](docs/adr/index.md)

---

## Current Release State

```text
Backend Foundation        ████████████ 100%
Snapshot Runtime          ████████████ 100%
Read API                  ████████████ 100%
Change Feed               ████████████ 100%
Backend Registry          ████████████ 100%
Backend-Aware Snapshots   ████████████ 100%
Multi-Backend Routing     ████████████ 100%
Multi-Backend Polling     ████████████ 100%
Multi-Backend Read API    ████████████ 100%
CI Foundation             ████████████ 100%
Live Transport            ████████████ 100%
Selective Event Queries   ████████████ 100%
Heavy Domain Policy       ████████████ 100%
EPG REST API Boundary     ████████████ 100%
EPG Search API            ████████████ 100%
Content Classific. ADR.   ████████████ 100%
Backend Optional Runtime  ████████████ 100%
```

Latest Completed Implementation Phase: Phase 46.10 - Content Rating JSON Contract

Current Implementation Focus: Phase 46.11 - Content Rating REST Boundary

Roadmap Progress: see [Roadmap](docs/planning/roadmap.md)

---

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, snapshot architecture, REST APIs, backend registry foundations and future frontend integrations.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Architecture Highlight

ADR-0021 defines a core runtime rule:

```text
Prefer selective backend queries
before full-domain transfers.
```

ADR-0028 defines the content classification rule:

```text
Do not model genre as a single plain string.
Preserve source-aware classification evidence.
```

The performance target is backend workload comparable to established VDR frontends such as `live` when equivalent user-visible information is requested.

---

## Start Here

New to VDR-Suite?

Read these documents first:

1. [Project Status Dashboard](docs/project-status-dashboard.md)
2. [Project Overview](docs/project-overview.md)
3. [Documentation Index](docs/index.md)
4. [Developer Documentation](docs/development/index.md)
5. [Real Recording Action End-to-End Validation](docs/development/real-recording-action-e2e-validation.md)
5. [Developer Onboarding](docs/development/developer-onboarding.md)
6. [Architecture Map](docs/development/architecture-map.md)
7. [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
8. [Current Project Status](docs/development/current-status.md)
9. [Roadmap](docs/planning/roadmap.md)
10. [Architecture Index](docs/architecture/index.md)
11. [ADR Index](docs/adr/index.md)
12. [ADR-0021 Selective Backend Query Strategy](docs/adr/ADR-0021-selective-backend-query-strategy.md)
13. [ADR-0028 Content Classification Architecture](docs/adr/ADR-0028-content-classification-architecture.md)
14. [Genre Architecture](docs/development/genre-architecture.md)

---

## Documentation Navigation Rule

The README is the repository entry point.

Every documentation page should provide navigation back to:

- [README](README.md)
- [Documentation Index](docs/index.md)
- [Project Overview](docs/project-overview.md)
- the local section index when applicable

This prevents documentation dead ends and keeps the documentation usable from GitHub, editors and local checkouts.
