# VDR-Suite Documentation Index

This document is the central entry point for the VDR-Suite documentation.

It links the project vision, current status, architecture documents, ADRs, development notes, diagrams and planning documents.

---

## Platform Direction

VDR-Suite is evolving into a VDR-centered platform.

The primary architectural question is not:

```text
Which frontend should VDR-Suite support?
```

The primary architectural question is:

```text
Which VDR-Suite capabilities should be reusable by other programs?
```

Potential consumers include:

- Web frontends
- Desktop frontends
- Mobile frontends
- VDR plugins
- OSD integrations
- Automation tools
- Monitoring tools
- External services
- Third-party management tools

See:

- ADR-007 Platform API Strategy
- Core Platform Model

---

## Start Here

Read these documents first:

- [VDR-Suite Vision](introduction/vdr-suite-vision.md)
- [Current Project Status](development/current-status.md)
- [Project Principles](project-principles.md)
- [Project Glossary](project-glossary.md)

---

## Project Introduction

- [VDR-Suite Vision](introduction/vdr-suite-vision.md)
- [Phase 0 Overview](phase-0/00-overview.md)
- [Phase 0 Vision](phase-0/01-vision.md)
- [Phase 0 Architecture](phase-0/02-architecture.md)
- [Phase 0 Data Model](phase-0/03-data-model.md)
- [Phase 0 Service Layer](phase-0/04-service-layer.md)
- [Phase 0 Plugin Strategy](phase-0/05-plugin-strategy.md)
- [Phase 0 UI Concept](phase-0/06-ui-concept.md)
- [Phase 0 Development Rules](phase-0/07-development-rules.md)
- [Phase 0 External Module Status](phase-0/08-external-module-status.md)

---

## Development Status

- [Current Project Status](development/current-status.md)
- [Runtime Diagnostics Measurement Collection](development/phase-10-runtime-diagnostics-measurement-collection.md)
- [Completed Phases](development/completed-phases.md)
- [Development Milestones](development/milestones.md)
- [Planning Milestones](planning/milestones.md)
- [Roadmap](planning/roadmap.md)

---

## Architecture

Core architecture documents:

- [Core Platform Model](architecture/vdr-suite-core-platform-model.md)
- [Suite Components](architecture/suite-components.md)
- [VDR Backends](architecture/vdr-backends.md)
- [VDR Domain Model](architecture/vdr-domain-model.md)
- [REST API Runtime](architecture/rest-api-runtime.md)
- [Daemon REST Runtime](architecture/daemon-rest-runtime.md)
- [HTTP Server Boundary](architecture/http-server-boundary.md)
- [Test HTTP Server](architecture/test-http-server.md)
- [RESTfulAPI Integration](architecture/restfulapi-integration.md)
- [Snapshot Architecture](architecture/snapshot-architecture.md)
- [Snapshot Access Architecture](architecture/snapshot-access-architecture.md)
- [Internal Event Dispatch Architecture](architecture/internal-event-dispatch-architecture.md)
- [Partial Snapshot Refresh Architecture](architecture/partial-snapshot-refresh-architecture.md) – Phase 9 runtime plan integration completed
- [Media Platform Comparison](architecture/media-platform-comparison.md)
- [External Project Analysis](architecture/external-project-analysis.md)

---

## Research and Ecosystem Analysis

- [External Project Analysis](architecture/external-project-analysis.md)

Phase-specific architecture notes:

- [Phase 8.94 Snapshot Cache Integration Plan](architecture/phase-8.94-snapshot-cache-integration-plan.md)
- [Phase 8.94 Runtime Wiring Notes](architecture/phase-8.94-runtime-wiring-notes.md)

---

## Development Architecture Notes

- [Phase 8 Architecture Guardrails](development/phase-8-architecture-guardrails.md)
- [Phase 8.37 VDR Source Model Status](development/phase-8.37-vdr-source-model-status.md)
- [Phase 8.38 Minimal Source Domain Object Gate](development/phase-8.38-minimal-source-domain-object-gate.md)
- [Phase 8.38 Source Registry Architecture](development/phase-8.38-source-registry-architecture.md)
- [Phase 8.38 Source Type Decision](development/phase-8.38-source-type-decision.md)
- [Phase 8.39 Content Identity Architecture](development/phase-8.39-content-identity-architecture.md)
- [Phase 8.40 Action Target Architecture](development/phase-8.40-action-target-architecture.md)
- [Phase 8.41 Platform Action Architecture](development/phase-8.41-platform-action-architecture.md)
- [Phase 8.42 Capability Resolver Architecture](development/phase-8.42-capability-resolver-architecture.md)
- [Phase 10 Runtime Diagnostics Measurement Collection](development/phase-10-runtime-diagnostics-measurement-collection.md)

---

## Architecture Decision Records

Foundation ADRs:

- [ADR-0001 Monorepo](adr/ADR-0001-monorepo.md)
- [ADR-0002 SQLite](adr/ADR-0002-sqlite.md)
- [ADR-0003 REST API](adr/ADR-0003-rest-api.md)
- [ADR-0004 C++17](adr/ADR-0004-cpp17.md)
- [ADR-0005 External VDR Integration Strategy](adr/ADR-0005-external-vdr-integration-strategy.md)
- [ADR-0006 VDR Backend Architecture](adr/ADR-0006-vdr-backend-architecture.md)
- [ADR-0007 RESTfulAPI Adapter Boundary](adr/ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0008 Real HTTP Server Strategy](adr/ADR-0008-real-http-server-strategy.md)
- [ADR-0009 HTTP Server Factory Strategy](adr/ADR-0009-http-server-factory-strategy.md)
- [ADR-0010 Library-First VDR Architecture](adr/ADR-0010-library-first-vdr-architecture.md)
- [ADR-0011 VDR Source Model Architecture](adr/ADR-0011-vdr-source-model-architecture.md)
- [ADR-0011 VDR Source Model](adr/ADR-0011-vdr-source-model.md)
- [ADR-0012 Source Capability Model](adr/ADR-0012-source-capability-model.md)
- [ADR-0013 Permission Model](adr/ADR-0013-permission-model.md)

Future-facing backend ADRs:

- [ADR-001 Backend Identity Strategy](adr/adr-001-backend-identity-strategy.md)
- [ADR-002 Backend Federation Strategy](adr/adr-002-backend-federation-strategy.md)
- [ADR-003 Backend Capability Strategy](adr/adr-003-backend-capability-strategy.md)
- [ADR-004 Backend Lifecycle Strategy](adr/adr-004-backend-lifecycle-strategy.md)
- [ADR-005 Stream Provider Strategy](adr/adr-005-stream-provider-strategy.md)
- [ADR-006 Internal Event Dispatch Strategy](adr/adr-006-internal-event-dispatch-strategy.md)
- [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)

---

## Diagrams

Mermaid diagrams:

- [System Overview](diagrams/system-overview.mmd)
- [Service Layer](diagrams/service-layer.mmd)
- [Database](diagrams/database.mmd)
- [Plugin Migration](diagrams/plugin-migration.mmd)

---

## Build, Dependencies and Database

- [Build Requirements](build-requirements.md)
- [Dependencies](dependencies.md)
- [Database Design](database-design.md)

---

## Recommended Reading Order

For new contributors:

1. [VDR-Suite Vision](introduction/vdr-suite-vision.md)
2. [Current Project Status](development/current-status.md)
3. [Runtime Diagnostics Measurement Collection](development/phase-10-runtime-diagnostics-measurement-collection.md)
4. [Core Platform Model](architecture/vdr-suite-core-platform-model.md)
5. [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)
6. [Snapshot Architecture](architecture/snapshot-architecture.md)
7. [VDR Backends](architecture/vdr-backends.md)
8. [Architecture Decision Records](#architecture-decision-records)

---

## Documentation Maintenance

When adding new documentation:

- add the document to this index
- link related architecture documents
- update Current Project Status after completed phases
- create or update ADRs for long-term architectural decisions
