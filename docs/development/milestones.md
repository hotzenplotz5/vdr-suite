# VDR-Suite – Development Milestones

This file keeps implementation milestone and tag history out of `docs/development/current-status.md`.

Forward-looking milestone planning is tracked in:

- [Planning Milestones](../planning/milestones.md)
- [Roadmap](../planning/roadmap.md)

Current verified project state is tracked in:

- [Current Project Status](current-status.md)

---

## Existing Milestone Tags

### v0.1-phase0

Project foundation and documentation.

### v0.2-phase1-core

Core architecture and persistence.

### v0.3-phase1-workflow

Workflow layer.

### v0.4-phase2-worker

Worker lifecycle.

### v0.5-phase2-queue

Queue-style processing.

### v0.6-phase3-dashboard

Job dashboard service.

### v0.7-phase4-recording-dashboard

Recording dashboard service.

### v0.8-phase5-dashboard-facade

Dashboard facade.

### v0.9-phase6-json

Dashboard JSON serialization.

### v1.0-phase6-cli

Dashboard CLI application.

### v1.1-phase6-rest-skeleton

Initial REST API foundation and project structure.

### v1.2-phase7-router

API router.

### v1.3-phase7-jobs-api

Jobs API route.

### v1.4-phase7-recordings-api

Recordings API route.

### v1.5-phase7-metadata-api

Metadata API route.

### v1.6-phase8-daemon-foundation

Daemon foundation.

### v1.7-phase8-vdr-adapter-foundation

External VDR integration foundation.

### v1.8-phase8-vdr-config

VdrConfig architecture.

### v1.9-phase8-vdr-adapter-interface

IVdrAdapter abstraction layer.

### v1.10-phase8-vdr-adapter-factory

VdrAdapterFactory implementation.

### v1.11-phase8-vdr-mock-backend

Mock backend implementation.

### v1.12-phase8-vdr-backend-architecture

Backend-independent VDR backend architecture.

### v1.13-phase8-restfulapi-architecture

RESTfulAPI integration architecture.

### v1.14-phase8-http-abstraction

HTTP abstraction layer.

### v1.15-phase8-restfulapi-adapter

RESTfulAPI VDR adapter foundation.

### v1.18-phase8-vdr-event-domain-object

VDR event domain object.

### v1.19-phase8-vdr-event-adapter-architecture

VDR event adapter architecture.

### v1.42-phase8-library-first-vdr-architecture

Media platform comparison and Library First VDR architecture.

---

## Verified Implementation Milestones After v1.42

The repository continued with additional implementation phases after the last documented milestone tag.

The following milestones are tracked by commit history and current status documentation, even if no dedicated tag exists for each sub-phase.

### Phase 8 – VDR Backend and Architecture Expansion

Status: Completed foundation

Verified work included:

- VDR domain model expansion
- RESTfulAPI mapper work
- HTTP abstraction and test server work
- backend identity and federation ADRs
- source, capability and permission architecture notes
- snapshot access architecture preparation

### Phase 9 – Snapshot Runtime Validation

Status: Completed

Verified work included:

- `VdrSnapshot`
- `VdrSnapshotBuilder`
- `PollingService`
- `SnapshotCache`
- `SnapshotCacheService`
- `SnapshotAccessService`
- snapshot-backed overview support
- change detection foundation
- snapshot refresh decision model
- snapshot refresh planner
- partial snapshot refresh validation

### Phase 10 – Runtime Diagnostics and Runtime Wiring

Status: Completed

Verified work included:

- `IRuntimeMeasurementSink`
- `RuntimeDiagnosticsService`
- HTTP client runtime measurement collection
- snapshot builder runtime measurement collection
- polling service runtime measurement collection
- runtime diagnostics JSON serialization
- `GET /api/runtime`
- `GET /api/runtime/summary`
- bounded diagnostics retention
- runtime configuration cleanup
- status documentation split

### Phase 11 – Snapshot Read APIs

Status: Completed for current domain set

Verified work included:

- `VdrSnapshotReadService`
- `VdrSnapshotReadJsonSerializer`
- snapshot-backed VDR controller methods
- snapshot read API router paths
- HTTP server test coverage
- status JSON serialization
- channel JSON serialization
- timer JSON serialization
- event JSON serialization
- recording JSON serialization
- controller and router tests for all current snapshot read domains

Latest verified implementation state:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Verified with:

```text
make test-api-router
make test
```

---

## Current Development Transition

Current transition:

```text
Phase 11 complete
↓
Documentation cleanup
↓
Phase 12.0 Snapshot Change Feed Architecture
```

Phase 12 should start with architecture review and documentation before new live-update transport is implemented.

---

## Tagging Rule

After implementation phases, create a milestone tag when the build has been verified and the phase represents a stable project milestone.

Documentation-only cleanup commits may remain untagged unless they mark a larger architectural milestone.

Tag names must not be invented retroactively in this document. Only record tags that actually exist in the repository.
