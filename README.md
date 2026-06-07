# VDR-Suite

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, snapshot architecture, REST APIs and future frontend integrations.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Start Here

New to VDR-Suite?

Read these documents first:

1. [Documentation Index](docs/index.md)
2. [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
3. [Current Project Status](docs/development/current-status.md)
4. [Roadmap](docs/planning/roadmap.md)
5. [Planning Milestones](docs/planning/milestones.md)
6. [Core Platform Model](docs/architecture/vdr-suite-core-platform-model.md)

The documentation index is the central entry point for all project documentation.

---

## Current Development Position

Current implementation status:

```text
Phase 11.6 Snapshot Read APIs completed for the current domain set
```

Next major architecture target:

```text
Phase 12.0 Snapshot Change Feed Architecture
```

Authoritative project status:

- [Current Project Status](docs/development/current-status.md)
- [Phase 11 Snapshot Read APIs](docs/development/phase-11-snapshot-read-apis.md)

---

## Planning

Project planning documents:

- [Roadmap](docs/planning/roadmap.md)
- [Planning Milestones](docs/planning/milestones.md)
- [Development Milestones](docs/development/milestones.md)

The roadmap defines direction.

Milestones define execution targets.

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

Important architecture entry points:

- [Core Platform Model](docs/architecture/vdr-suite-core-platform-model.md)
- [VDR Backends](docs/architecture/vdr-backends.md)
- [Snapshot Architecture](docs/architecture/snapshot-architecture.md)
- [Snapshot Access Architecture](docs/architecture/snapshot-access-architecture.md)
- [Internal Event Dispatch Architecture](docs/architecture/internal-event-dispatch-architecture.md)
- [Partial Snapshot Refresh Architecture](docs/architecture/partial-snapshot-refresh-architecture.md)

Long-term decisions:

- [Architecture Decision Records](docs/index.md#architecture-decision-records)

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
- snapshot read APIs and JSON serialization

---

## Repository Structure

- `core/sqlite` – SQLite wrapper and database tests
- `core/recordings` – recordings, metadata, jobs, actions and dashboard services
- `core/http` – HTTP abstractions, clients, listener and test server
- `core/vdr` – VDR domain objects, adapters, mappers, overview, polling and snapshot services
- `core/runtime` – runtime logging and diagnostics foundations
- `core/daemon` – daemon runtime and lifecycle
- `api/rest` – REST controllers and API router
- `apps/dashboard` – dashboard CLI
- `apps/daemon` – daemon entry point
- `mk` – modular Makefile include files
- `docs` – project documentation, planning, architecture notes and ADRs

---

## Build

Common targets:

```bash
make daemon
make test
```

Useful targeted tests:

```bash
make test-vdr-controller
make test-vdr-snapshot-builder
make test-polling-service
make test-restful-api-vdr-adapter
make test-runtime-diagnostics
```

Use `make test` before milestone tags, larger refactoring and runtime architecture changes.

---

## Documentation Hub

Primary documentation entry points:

- [Documentation Index](docs/index.md)
- [Current Project Status](docs/development/current-status.md)
- [Roadmap](docs/planning/roadmap.md)
- [Planning Milestones](docs/planning/milestones.md)
- [Development Milestones](docs/development/milestones.md)
- [Completed Phases](docs/development/completed-phases.md)
- [Architecture Documents](docs/index.md#architecture)
- [Architecture Decision Records](docs/index.md#architecture-decision-records)

---

## Development Rules

Project-specific rules are documented in:

- [Current Project Status](docs/development/current-status.md)
- [Phase 0 Development Rules](docs/phase-0/07-development-rules.md)

---

## License

See `LICENSE`.
