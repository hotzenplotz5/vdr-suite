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
 ├─ Planning Index
 ├─ Development Index
 ├─ Architecture Index
 ├─ ADR Index
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
3. [Planning Index](planning/index.md)
4. [Roadmap](planning/roadmap.md)
5. [Architecture Index](architecture/index.md)
6. [Core Platform Model](architecture/vdr-suite-core-platform-model.md)
7. [Snapshot Architecture](architecture/snapshot-architecture.md)
8. [Development Index](development/index.md)
9. [Phase 11 Snapshot Read APIs](development/phase-11-snapshot-read-apis.md)
10. [ADR Index](adr/index.md)
11. [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)
12. [ADR-0014 Recording Identity Strategy](adr/ADR-0014-recording-identity-strategy.md)

---

## Current Position

Current completed implementation phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Current transition:

```text
Documentation cleanup
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

- [Architecture Index](architecture/index.md)
- [ADR Index](adr/index.md)
- [ADR-007 Platform API Strategy](adr/007-platform-api-strategy.md)
- [ADR-0014 Recording Identity Strategy](adr/ADR-0014-recording-identity-strategy.md)
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

- [Planning Index](planning/index.md)
- [Roadmap](planning/roadmap.md)
- [Planning Milestones](planning/milestones.md)

---

## Development Status

- [Development Index](development/index.md)
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

- [Architecture Index](architecture/index.md)

The architecture index groups the architecture documentation by platform, VDR backend integration, REST/HTTP runtime, snapshot architecture, event/change architecture and historical architecture notes.

---

## Architecture Decision Records

- [ADR Index](adr/index.md)

The ADR index lists all foundation, future-facing and superseded ADRs.

---

## Build, Dependencies and Database

- [Build Requirements](build-requirements.md)
- [Dependencies](dependencies.md)
- [Database Design](database-design.md)

---

## Documentation Maintenance

When adding new documentation:

- add the document to this index
- update the planning index when adding planning documents
- update the development index when adding development documents
- update the architecture index when adding architecture documents
- update the ADR index when adding or superseding ADRs
- keep README and roadmap linked
- update current status after completed phases
- avoid duplicate navigation structures