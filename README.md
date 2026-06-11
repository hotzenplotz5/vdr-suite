# VDR-Suite

[![VDR-Suite CI](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hotzenplotz5/vdr-suite/actions/workflows/ci.yml)

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
Live Transport            ░░░░░░░░░░░░   0%
```

Latest Completed Implementation Phase: Phase 20.0 - Live Update Event Foundation

Current Implementation Focus: Phase 20.1 - Live Transport Interface

Roadmap Progress: see [Roadmap](docs/planning/roadmap.md)

---

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, snapshot architecture, REST APIs, backend registry foundations and future frontend integrations.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

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
```

Current architecture focus:

```text
Phase 19.3 - Snapshot Change Feed REST Validation
Introduce the live transport boundary above the validated snapshot change feed
Keep real VDR tests opt-in and separate from fast unit tests
Preserve mock-based CI compatibility
```

Authoritative project status:

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Project Overview](docs/project-overview.md)
- [Current Project Status](docs/development/current-status.md)
- [Completed Phases](docs/development/completed-phases.md)
- [Roadmap](docs/planning/roadmap.md)

---

## Architecture Direction

VDR-Suite is designed to remain:

- VDR-centric
- backend-neutral
- service-oriented
- daemon-driven
- snapshot-oriented
- prepared for future multi-VDR environments
- suitable for future API consumers and frontends

---

## Implemented Areas

The project currently contains foundations for:

- SQLite persistence
- recording, metadata, job and action services
- dashboard services and JSON serialization
- REST controllers and API routing
- HTTP client/server abstractions
- VDR domain objects and adapter boundaries
- RESTfulAPI integration
- daemon-owned VDR snapshots
- snapshot cache and snapshot access services
- backend registry and backend registry service layer
- backend registry JSON serialization and REST API exposure
- backend-aware snapshot access routing boundary
- backend-aware VDR controller methods
- backend snapshot API routes
- dynamic backend snapshot route parsing
- multi-snapshot cache foundation
- multi-backend snapshot cache access
- backend-aware snapshot builder
- backend-aware polling service and polling coordinator
- daemon runtime backend context collection
- registry-driven backend runtime context creation
- backend-aware snapshot change feed service
- multi-backend snapshot read service
- multi-backend snapshot summary serialization
- multi-backend snapshots REST endpoint
- multi-backend snapshots REST route test coverage
- change-state polling and partial snapshot refresh planning
- runtime logging, timing and diagnostics foundations
- snapshot read APIs
- snapshot change feed foundation
- GitHub Actions CI full regression test

---

## Repository Structure

- `core/sqlite`
- `core/recordings`
- `core/http`
- `core/vdr`
- `core/runtime`
- `core/daemon`
- `api/rest`
- `apps/dashboard`
- `apps/daemon`
- `mk`
- `docs`

---

## Build

```bash
make daemon
make test
```

The full regression test is also executed by GitHub Actions on push and pull request events.

Recommended local pre-commit verification for architecture work:

```bash
make test-fast
make test-docs
make test-architecture
make test-phase
make daemon
```

---

## Documentation Hub

Primary entry points:

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Project Overview](docs/project-overview.md)
- [Documentation Index](docs/index.md)
- [Development Documentation](docs/development/index.md)
- [Developer Onboarding](docs/development/developer-onboarding.md)
- [Architecture Map](docs/development/architecture-map.md)
- [Testing Guide](docs/development/testing-guide.md)
- [Coding Standards](docs/development/coding-standards.md)
- [Documentation Standards](docs/development/documentation-standards.md)
- [Backend Development Guide](docs/development/backend-development-guide.md)
- [Contributor Guide](docs/development/contributor-guide.md)
- [Release Process](docs/development/release-process.md)
- [Current Project Status](docs/development/current-status.md)
- [Current Architecture State](docs/development/current-architecture-state.md)
- [Completed Phases](docs/development/completed-phases.md)
- [Roadmap](docs/planning/roadmap.md)
- [Planning Documentation](docs/planning/index.md)
- [Architecture Documentation](docs/architecture/index.md)
- [Architecture Decision Records](docs/adr/index.md)
- [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
- [Build Requirements](docs/build-requirements.md)
- [Dependencies](docs/dependencies.md)
- [Database Design](docs/database-design.md)

---

## License

See [LICENSE](LICENSE).
