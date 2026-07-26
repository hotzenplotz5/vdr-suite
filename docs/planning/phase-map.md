# VDR-Suite Phase Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Current Project Status](../development/current-status.md)
- [Completed Phases](../development/completed-phases.md)
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)

---

## Purpose

This file is the canonical compact phase-number map for VDR-Suite. Detailed history belongs in [Completed Phases](../development/completed-phases.md); strict future order belongs in [Roadmap](roadmap.md).

---

## Completed Phase Ranges

| Range | Status | Track | Result |
| --- | --- | --- | --- |
| Phase 1.x-7.x | Completed | Core Platform | Database, repositories, services, REST boundaries and daemon foundation. |
| Phase 8.x | Completed | VDR Backend | VDR domain objects, adapter boundaries and RESTfulAPI integration foundation. |
| Phase 9.x-29.x | Completed | Multi-Backend Runtime | Backend registry, snapshots, change feed and live transport foundation. |
| Phase 30.x-36.x | Completed | Recording Actions | Recording validation and guarded execution foundation. |
| Phase 37.x-44.x | Completed | Recording Runtime Hardening | Real-backend validation, regression coverage and safety transition. |
| Phase 45.x | Completed | EPG Search | Selective EPG query/search foundation and REST surface. |
| Phase 46.x | Completed | Metadata and People | Classification, metadata, people and character search foundations. |
| Phase 47.x-49.x | Completed | SearchTimer Backend | Backend foundation, RESTfulAPI compatibility and native validation. |
| Phase 50.0-50.50 | Completed | SearchTimer Workflow | Dry-run, safety gates, readback verification and controlled execution. |
| Phase 51.x | Completed | Live Parity Discovery | Live source audit and parity-gap foundation. |
| Phase 52.x | Completed | SearchTimer Automation Planning | Read-only planning and automation safety review. |
| Phase 53.x | Completed | SearchTimer Completion Audit | Completion audit and workflow preservation. |
| Phase 54.x | Completed | SearchTimer Preview Runtime | Preview runtime, mutation policy and warm EPG cache architecture. |
| Phase 55.0-55.4e | Completed | Adapter and Runtime Hardening | Adapter audit, contract fixes and daemon lifecycle guardrails. |
| Phase 55.5a-55.5n | Completed | Acceptance and Documentation | Acceptance, native preview capability and documentation coverage. |
| Phase 55.5o | Completed | Phase Map and Roadmap | Canonical phase map and coverage guardrail. |
| Phase 55.6 | Completed | Recording Operations Audit | Recording mutation safety and guarded real-write probes. |
| Phase 56 | Completed | Library Boundary and Packaging | Source boundaries, packaging, staging and developer documentation. |
| Phase 57 | Completed | Multi-Site Backend Administration and Permissions | Backend access modes and server-enforced read-only foundation. |
| Phase 58.0-58.90b | Completed slices; umbrella retained | Frontend and Live Parity | Frontend foundations, EPG input, channel movement and sorting. |
| Phase 59.00-59.15e | Completed | Frontend Client API and Modules | Client API consolidation, module extraction and ownership guards. |
| Phase 60.1-60.15 | Completed | Frontend Platform and Metadata Preparation | Platform bootstrap, lazy Recording cache, metadata and authenticated artwork preparation. |
| Phase 61 | Completed | Suite Metadata and Genre Platform | Persistent backend-scoped Recording/EPG Genre assignments, metadata-backed Genre browser, TVScraper evidence paths and accepted runtime integration. |

---

## Completed Architecture Contract Package

```text
ADR-0042 through ADR-0049
Target Platform Architecture
Domain Dependency Map
Implementation Dependency Map
```

Status:

```text
Completed architecture and planning prerequisite
Not a substitute for runtime implementation
```

---

## Current Position

```text
Latest completed runtime phase
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track
Phase 58 - Frontend and Live Parity

Next runtime implementation phase
Phase 62 - Identity, RBAC and Accountability Foundation
```

The B1-B4 label is a completed post-phase hardening block, not a new numbered roadmap phase.

---

## Planned Phase Sequence

| Order | Range | Status | Track | Goal |
| ---: | --- | --- | --- | --- |
| 1 | Phase 62 | Next | Identity, RBAC and Accountability | Add actor identities, scoped authorization and append-only accountability. |
| 2 | Phase 63 | Planned | Backend Agent and Multi-Site Runtime | Add Agent enrollment, secure transport, generation, lease, health and fenced commands. |
| 3 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler and reconciler. |
| 4 | Phase 65 | Planned | Streaming Gateway | Add authenticated short-lived media sessions over internal providers. |
| 5 | Phase 66 | Planned | Legacy OSD Bridge | Add isolated compatibility sessions with view/control separation. |
| 6 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions and compatibility. |
| 7 | Phase 68 | Vision | Recommendation and Knowledge Graph | Add explainable recommendations after platform foundations mature. |

---

## Numbering Rules

- Completed history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phase 61 is closed for its accepted metadata/Genre runtime scope.
- Optional provider and observability extensions do not silently reopen Phase 61.
- Future numbered milestones continue with Phase 62 through Phase 68.

---

## Maintenance Rules

- This file owns phase numbers and compact status.
- [Roadmap](roadmap.md) owns strict order and exit criteria.
- [Implementation Dependency Map](implementation-dependency-map.md) owns detailed prerequisites.
- [Completed Phases](../development/completed-phases.md) owns chronological implementation history.
- When a runtime phase completes, update this file and the phase-map coverage checker.

Verification:

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Planning Index](index.md)