# VDR-Suite

[![VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml)

## Project Overview

- German: [Projektübersicht](docs/project-overview.de.md)
- English: [Project Overview](docs/project-overview.en.md)

## Quick Links

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Current Project Status](docs/development/current-status.md)
- [Developer Documentation](docs/development/index.md)
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
Event-Free Initial Poll   ████████████ 100%
EPG Decoupling Validation ████████████ 100%
EPG JSON Escaping        ████████████ 100%
Backend Optional Runtime  ████████████ 100%
```

Latest Completed Implementation Phase: Phase 28.12 - Recording Query API Documentation

Current Implementation Focus: Phase 29.0 - Multi-Backend Recording Identity Foundation

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

The performance target is backend workload comparable to established VDR frontends such as `live` when equivalent user-visible information is requested.

---

## Start Here

New to VDR-Suite?

Read these documents first:

1. [Project Status Dashboard](docs/project-status-dashboard.md)
2. [Project Overview](docs/project-overview.md)
3. [Documentation Index](docs/index.md)
4. [Developer Documentation](docs/development/index.md)
5. [Developer Onboarding](docs/development/developer-onboarding.md)
6. [Architecture Map](docs/development/architecture-map.md)
7. [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
8. [Current Project Status](docs/development/current-status.md)
9. [Roadmap](docs/planning/roadmap.md)
10. [Architecture Index](docs/architecture/index.md)
11. [ADR Index](docs/adr/index.md)
12. [ADR-0021 Selective Backend Query Strategy](docs/adr/ADR-0021-selective-backend-query-strategy.md)

---

## Documentation Navigation Rule

The README is the repository entry point.

Every documentation page should provide navigation back to:

- [README](README.md)
- [Documentation Index](docs/index.md)
- [Project Overview](docs/project-overview.md)
- the local section index when applicable

This prevents documentation dead ends and keeps the documentation usable from GitHub, editors and local checkouts.

---

## Current Architecture State

```text
✓ Snapshot Architecture
✓ Snapshot Cache
✓ Snapshot Access Layer
✓ Snapshot Read APIs
✓ Snapshot Change Feed
✓ Runtime Diagnostics
✓ VDR Health API
✓ VDR Snapshot Summary API
✓ VDR Capability API
✓ Capability Resolver Foundation
✓ Backend Registry Foundation
✓ Backend Registry API
✓ Backend-Aware Snapshot Access
✓ Backend-Aware VDR Controller Methods
✓ Multi-Snapshot Cache Foundation
✓ Backend-Aware Snapshot Builder
✓ Backend-Aware Polling Service
✓ Backend Polling Coordinator
✓ Backend Runtime Context
✓ Daemon Runtime Context Collection
✓ Registry-driven Backend Runtime Context Creation
✓ Backend-aware Snapshot Change Feed
✓ Multi-Backend Snapshot Read Foundation
✓ Multi-Backend Snapshot Summary Serialization
✓ Multi-Backend Snapshots REST Endpoint
✓ Multi-Backend Snapshots REST Route Test
✓ GitHub Actions CI
✓ Selective Event Query Contract
✓ Heavy Domain Refresh Policy
✓ EPG REST API Boundary
✓ Live EPG REST API Verification
✓ Event-Free Initial Snapshot
✓ EPG Snapshot Decoupling Validation
✓ EPG JSON Escaping
✓ Backend Optional Runtime
```

Current architecture focus:

```text
Phase 29.0 - Multi-Backend Recording Identity Foundation
Evaluate selective recording query boundaries
Keep VDR as source of truth for recordings
Keep VDR-Suite metadata complementary
```
