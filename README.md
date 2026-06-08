# VDR-Suite

## Quick Links

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Current Project Status](docs/development/current-status.md)
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
Live Transport            ░░░░░░░░░░░░   0%
```

Current Phase: Phase 12 Complete

Next Phase: Phase 13 - Live Update Transport

Roadmap Progress: 76.5%

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
4. [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
5. [Current Project Status](docs/development/current-status.md)
6. [Roadmap](docs/planning/roadmap.md)
7. [Architecture Index](docs/architecture/index.md)
8. [ADR Index](docs/adr/index.md)

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
```

Current architecture focus:

```text
Live Update Transport
Capability System
Federation Foundations
```

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
- change-state polling and partial snapshot refresh planning
- runtime logging, timing and diagnostics foundations
- snapshot read APIs
- snapshot change feed foundation

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

---

## Documentation Hub

Primary entry points:

- [Project Status Dashboard](docs/project-status-dashboard.md)
- [Project Overview](docs/project-overview.md)
- [Documentation Index](docs/index.md)
- [Current Project Status](docs/development/current-status.md)
- [Completed Phases](docs/development/completed-phases.md)
- [Roadmap](docs/planning/roadmap.md)
- [Development Documentation](docs/development/index.md)
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
