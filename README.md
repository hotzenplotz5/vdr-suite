# VDR-Suite

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, REST API foundations and future user interfaces.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Start Here

New to VDR-Suite?

Read these documents first:

- [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
- [Documentation Index](docs/index.md)
- [Current Project Status](docs/development/current-status.md)
- [Core Platform Model](docs/architecture/vdr-suite-core-platform-model.md)

The documentation index is the central entry point for project vision, architecture documents, ADRs, diagrams, development status and planning.

---

## Current Status

The current implementation status changes frequently during active development.

Authoritative status lives here:

- [Current Project Status](docs/development/current-status.md)

The README intentionally does not duplicate detailed phase history, milestone status or long architecture explanations. This avoids stale information and keeps the README useful as a stable project entry point.

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

Current architecture details are documented in:

- [Core Platform Model](docs/architecture/vdr-suite-core-platform-model.md)
- [VDR Backends](docs/architecture/vdr-backends.md)
- [Snapshot Architecture](docs/architecture/snapshot-architecture.md)
- [Snapshot Access Architecture](docs/architecture/snapshot-access-architecture.md)
- [Internal Event Dispatch Architecture](docs/architecture/internal-event-dispatch-architecture.md)
- [Partial Snapshot Refresh Architecture](docs/architecture/partial-snapshot-refresh-architecture.md)

Long-term architectural decisions are documented in:

- [Architecture Decision Records](docs/adr/)

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
- change-state polling and partial snapshot refresh planning
- runtime logging, timing and diagnostics foundations

For the exact current phase state, see:

- [Current Project Status](docs/development/current-status.md)

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
- `docs` – project documentation, architecture notes and ADRs

---

## Build

Common targets:

```bash
make daemon
make test
```

Useful targeted tests during local development:

```bash
make test-vdr-snapshot-builder
make test-polling-service
make test-restful-api-vdr-adapter
make test-restful-api-change-state-adapter
make test-runtime-diagnostics
```

Use `make test` for release preparation, tags, larger refactoring, build-system changes and runtime wiring changes.

---

## Documentation

Important entry points:

- [Documentation Index](docs/index.md)
- [VDR-Suite Vision](docs/introduction/vdr-suite-vision.md)
- [Current Project Status](docs/development/current-status.md)
- [Completed Phases](docs/development/completed-phases.md)
- [Development Milestones](docs/development/milestones.md)
- [Architecture Documents](docs/index.md#architecture)
- [Architecture Decision Records](docs/index.md#architecture-decision-records)

---

## Development Rules

Project-specific rules are documented and maintained in:

- [Current Project Status](docs/development/current-status.md)
- [Phase 0 Development Rules](docs/phase-0/07-development-rules.md)

Important working rules include:

- read existing code before changes
- keep architecture decisions documented
- avoid placeholder implementations
- keep builds working after small changes
- run targeted tests before commits
- run `make test` before larger pushes or runtime changes

---

## License

See `LICENSE`.
