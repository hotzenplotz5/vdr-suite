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

---

## Purpose

This file is the canonical compact phase-number map for VDR-Suite.

The detailed chronological history remains in [Completed Phases](../development/completed-phases.md). The strict future execution order is defined in [Roadmap](roadmap.md), and its prerequisite expansion is defined in the [Implementation Dependency Map](implementation-dependency-map.md).

---

## Completed Phase Ranges

| Range | Status | Track | Result |
| --- | --- | --- | --- |
| Phase 1.x-7.x | Completed | Core Platform | Database, repositories, services, REST boundaries and daemon foundation. |
| Phase 8.x | Completed | VDR Backend | VDR domain objects, adapter boundaries and RESTfulAPI integration foundation. |
| Phase 9.x-29.x | Completed | Multi-Backend Runtime | Backend registry, snapshots, change feed and live transport foundation. |
| Phase 30.x-36.x | Completed | Recording Actions | Recording action validation and guarded execution foundation. |
| Phase 37.x-44.x | Completed | Recording Runtime Hardening | Runtime completion, real-backend validation, regression coverage and safety transition. |
| Phase 45.x | Completed | EPG Search | Selective EPG query/search foundation and EPG REST API surface. |
| Phase 46.x | Completed | Metadata and People | Classification, metadata foundations, people and character search. |
| Phase 47.x-49.x | Completed | SearchTimer Backend | Backend foundation, RESTfulAPI compatibility and native capability validation. |
| Phase 50.0-50.50 | Completed | SearchTimer Workflow | Dry-run, safety gates, readback verification and controlled execution. |
| Phase 51.x | Completed | Live Parity Discovery | Live plugin parity discovery and gap visibility foundation. |
| Phase 52.x | Completed | SearchTimer Automation Planning | Read-only planning, preview, scheduling plan and automation safety review. |
| Phase 53.x | Completed | SearchTimer Completion Audit | Completion audit and workflow preservation. |
| Phase 54.x | Completed | SearchTimer Preview Runtime | Preview runtime, mutation policy and warm EPG cache architecture. |
| Phase 55.0-55.4e | Completed | Adapter and Runtime Hardening | Adapter audit, contract fixes, discovery wiring and daemon lifecycle guardrails. |
| Phase 55.5a-55.5n | Completed | Acceptance and Documentation | Acceptance, native preview capability, lifecycle hardening and documentation coverage. |
| Phase 55.5o | Completed | Phase Map and Roadmap | Canonical phase map and coverage guardrail. |
| Phase 55.6 | Completed | Recording Operations Audit | Recording mutation safety policy and guarded real-write probes. |
| Phase 56 | Completed | Library Boundary and Packaging | Source boundaries, packaging, staging, manpages and prerequisite audit. |
| Phase 57 | Completed | Multi-Site Backend Administration and Permissions | Backend access modes and server-enforced read-only foundation. |
| Phase 58.0-58.90b | Completed slices; umbrella label retained | Frontend and Live Parity | Frontend foundations, EPG input, event hints, channel move API and sorting. |
| Phase 59.00-59.15e | Completed | Frontend Client API and Modules | Client API consolidation, module extraction and ownership guards. |
| Phase 60.1-60.15 | Completed | Frontend Platform, Recording UX and Metadata Preparation | Platform bootstrap, lazy Recording cache, detail UX, provider-neutral metadata, persistent artwork preparation and authenticated local artwork delivery. |

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
Not a completed runtime phase
```

The package defines the accepted ownership, trust, identity, mutation, orchestration, media, OSD, public API and accountability contracts for the future sequence.

---

## Current Position

```text
Latest completed major project block
Phase 57 - Multi-Site Backend Administration and Permissions

Current umbrella implementation track
Phase 58 - Frontend and Live Parity

Latest completed implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation

Next runtime implementation phase
Phase 61 - Suite Metadata Database and External Provider Integration
```

The Phase 58 umbrella label is retained for historical product grouping. It does not override the numbered execution sequence below.

---

## Planned Phase Sequence

| Order | Range | Status | Track | Goal |
| ---: | --- | --- | --- | --- |
| 1 | Phase 61 | Planned next runtime phase | Suite Metadata Platform | Build normalized suite-owned metadata, provider, provenance and artwork services. |
| 2 | Phase 62 | Planned | Identity, RBAC and Audit | Add user, service and Agent identities, scoped authorization and mutation accountability foundation. |
| 3 | Phase 63 | Planned | Backend Agent and Multi-Site Runtime | Implement Agent enrollment, secure transport, generation, lease, health and fenced commands. |
| 4 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler and reconciler. |
| 5 | Phase 65 | Planned | Streaming Gateway | Add authenticated short-lived media sessions over internal providers. |
| 6 | Phase 66 | Planned | Legacy OSD Bridge | Add isolated compatibility sessions with viewer and controller permissions. |
| 7 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions, compatibility and client contracts. |
| 8 | Phase 68 | Vision | Recommendation and Knowledge Graph | Add explainable recommendations after metadata and platform foundations mature. |

This table is authoritative for phase numbering. The [Roadmap](roadmap.md) owns strict order and exit criteria. The [Implementation Dependency Map](implementation-dependency-map.md) owns detailed prerequisite and slice order.

---

## Numbering Rules

- Completed phase history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phase 59 and Phase 60 are already-used implementation ranges and are not reused for new major milestones.
- Future milestones continue sequentially with Phase 61 through Phase 68.
- The completed architecture package is a contract prerequisite, not a runtime phase number.

---

## Maintenance Rules

- This file owns phase numbers and compact status.
- [Roadmap](roadmap.md) owns strict execution order and phase exit criteria.
- [Implementation Dependency Map](implementation-dependency-map.md) expands phase prerequisites and slice order.
- [Completed Phases](../development/completed-phases.md) owns chronological implementation history.
- When a runtime phase completes, update this file and run the phase-map coverage check.
- Accepted ADRs and target diagrams do not by themselves mark runtime phases complete.

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
