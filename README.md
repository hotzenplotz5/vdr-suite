# VDR-Suite

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, daemon-owned state, snapshot architecture, REST APIs and future frontend integrations.

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Start Here

New to VDR-Suite?

Read these documents first:

1. Project Overview (docs/project-overview.md)
2. Documentation Index (docs/index.md)
3. VDR-Suite Vision (docs/introduction/vdr-suite-vision.md)
4. Current Project Status (docs/development/current-status.md)
5. Roadmap (docs/planning/roadmap.md)
6. Architecture Index (docs/architecture/index.md)
7. ADR Index (docs/adr/index.md)

---

## Documentation Navigation Rule

The README is the repository entry point.

Every documentation page should provide navigation back to:

- README.md
- docs/index.md
- docs/project-overview.md
- the local section index when applicable

This prevents documentation dead ends and keeps the documentation usable from GitHub, editors and local checkouts.

---

## Current Development Position

Current implementation status:

```text
Phase 12 completed
Snapshot Change Feed Foundation completed
```

Next major architecture target:

```text
Phase 13 - Live Update Transport
```

Authoritative project status:

- docs/project-overview.md
- docs/development/current-status.md
- docs/development/completed-phases.md
- docs/planning/roadmap.md

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

- core/sqlite
- core/recordings
- core/http
- core/vdr
- core/runtime
- core/daemon
- api/rest
- apps/dashboard
- apps/daemon
- mk
- docs

---

## Build

```bash
make daemon
make test
```

---

## Documentation Hub

Primary entry points:

- docs/project-overview.md
- docs/index.md
- docs/development/current-status.md
- docs/development/completed-phases.md
- docs/planning/roadmap.md
- docs/architecture/index.md
- docs/adr/index.md

---

## License

See LICENSE.
