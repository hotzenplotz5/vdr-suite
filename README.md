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
Backend Identity          ████████████ 100%
CI Foundation             ████████████ 100%
Live Transport            ░░░░░░░░░░░░   0%
```

Current Phase: Phase 14ci - GitHub Actions CI Foundation

Latest Completed Implementation Phase: Phase 14.3 - Backend-Aware Snapshot Read Routing Boundary

Next Architecture Phase: BackendNode / BackendRegistry preparation

Roadmap Progress: see [Roadmap](docs/planning/roadmap.md)

---

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, snapshot architecture, REST APIs and future frontend integrations.

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
✓ Backend-Aware Snapshot Access Boundary
✓ Snapshot Read APIs
✓ Snapshot Change Feed
✓ Runtime Diagnostics
✓ VDR Health API
✓ VDR Snapshot Summary API
✓ VDR Capability API
✓ Capability Resolver Foundation
✓ GitHub Actions CI
```

Current architecture focus:

```text
BackendNode / BackendRegistry Preparation
Multi-VDR Read Routing Preparation
CI-backed Regression Validation
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
- backend-aware snapshot access routing boundary
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
