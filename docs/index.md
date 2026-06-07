# VDR-Suite Documentation Index

This document is the central entry point for VDR-Suite documentation.

It links the project vision, current status, roadmap, milestones, architecture documents, ADRs, development notes, diagrams and planning documents.

---

## Documentation Structure

```text
README
 ├─ Vision
 ├─ Current Status
 ├─ Roadmap
 └─ Documentation Index

Documentation Index
 ├─ Introduction
 ├─ Planning
 ├─ Development Status
 ├─ Architecture
 ├─ Architecture Notes
 ├─ ADRs
 ├─ Research
 ├─ Diagrams
 └─ Build / Database
```

Use this index as the authoritative navigation entry for all project documentation.

---

## Start Here

Read these documents first:

- [README](../README.md)
- [VDR-Suite Vision](introduction/vdr-suite-vision.md)
- [Current Project Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Planning Milestones](planning/milestones.md)
- [Project Principles](project-principles.md)
- [Project Glossary](project-glossary.md)

Recommended reading order for new contributors:

1. [VDR-Suite Vision](introduction/vdr-suite-vision.md)
2. [Current Project Status](development/current-status.md)
3. [Roadmap](planning/roadmap.md)
4. [Planning Milestones](planning/milestones.md)
5. [Core Platform Model](architecture/vdr-suite-core-platform-model.md)
6. [Snapshot Architecture](architecture/snapshot-architecture.md)
7. [Phase 11 Snapshot Read APIs](development/phase-11-snapshot-read-apis.md)
8. [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)
9. [Architecture Decision Records](#architecture-decision-records)

---

## Current Position

Current completed implementation phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Current transition:

```text
Documentation cleanup
↓
Phase 12.0 Snapshot Change Feed Architecture
```

Authoritative current state:

- [Current Project Status](development/current-status.md)
- [Phase 11 Snapshot Read APIs](development/phase-11-snapshot-read-apis.md)
- [Roadmap](planning/roadmap.md)

---

## Platform Direction

VDR-Suite is evolving into a VDR-centered platform.

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

- [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)
- [Core Platform Model](architecture/vdr-suite-core-platform-model.md)

---

## Project Introduction

- [VDR-Suite Vision](introduction/vdr-suite-vision.md)
- [Project Principles](project-principles.md)
- [Project Glossary](project-glossary.md)
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

## Planning

- [Roadmap](planning/roadmap.md)
- [Planning Milestones](planning/milestones.md)

---

## Development Status

- [Current Project Status](development/current-status.md)
- [Completed Phases](development/completed-phases.md)
- [Development Milestones](development/milestones.md)
- [Build System State](development/build-system-state.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Current Technical Debt](development/current-technical-debt.md)
- [Phase 9 Runtime Validation Result](development/phase-9-runtime-validation-result.md)
- [Runtime Diagnostics Status](development/runtime-diagnostics-status.md)
- [Phase 10 Runtime Diagnostics Measurement Collection](development/phase-10-runtime-diagnostics-measurement-collection.md)
- [Phase 11 Snapshot Read APIs](development/phase-11-snapshot-read-apis.md)

---

## Architecture

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
- [Partial Snapshot Refresh Architecture](architecture/partial-snapshot-refresh-architecture.md)
- [Media Platform Comparison](architecture/media-platform-comparison.md)
- [External Project Analysis](architecture/external-project-analysis.md)

---

## Architecture Decision Records

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
- [ADR-0012 Source Capability Model](adr/ADR-0012-source-capability-model.md)
- [ADR-0013 Permission Model](adr/ADR-0013-permission-model.md)
- [ADR-001 Backend Identity Strategy](adr/adr-001-backend-identity-strategy.md)
- [ADR-002 Backend Federation Strategy](adr/adr-002-backend-federation-strategy.md)
- [ADR-003 Backend Capability Strategy](adr/adr-003-backend-capability-strategy.md)
- [ADR-004 Backend Lifecycle Strategy](adr/adr-004-backend-lifecycle-strategy.md)
- [ADR-005 Stream Provider Strategy](adr/adr-005-stream-provider-strategy.md)
- [ADR-006 Internal Event Dispatch Strategy](adr/adr-006-internal-event-dispatch-strategy.md)
- [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)

---

## Build, Dependencies and Database

- [Build Requirements](build-requirements.md)
- [Dependencies](dependencies.md)
- [Database Design](database-design.md)

---

## Documentation Maintenance

When adding new documentation:

- add the document to this index
- keep README and roadmap linked
- update current status after completed phases
- avoid duplicate navigation structures
