# VDR-Suite Phase Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Current Project Status](../development/current-status.md)
- [Completed Phases](../development/completed-phases.md)

---

## Purpose

This file is the canonical compact phase-number map for VDR-Suite.

The detailed chronological history remains in [Completed Phases](../development/completed-phases.md). The strict future execution order is defined in [Roadmap](roadmap.md).

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
| Phase 60.1-60.14k | Completed | Frontend Platform and Recording UX | Platform bootstrap, lazy Recording cache, folder navigation and detail UX. |

---

## Current Position

```text
Latest completed major project block
Phase 57 - Multi-Site Backend Administration and Permissions

Current umbrella implementation track
Phase 58 - Frontend and Live Parity

Latest completed implementation slice
Phase 60.14k - Recording Detail UX Polish

Next planned implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation
```

The Phase 58 umbrella label is retained for historical product grouping. It does not override the numbered execution sequence below.

---

## Planned Phase Sequence

| Order | Range | Status | Track | Goal |
| ---: | --- | --- | --- | --- |
| 1 | Architecture package ADR-0042-ADR-0049 | In progress; ADR-0042 through ADR-0044 accepted | Core contracts | Complete the remaining EPG, streaming, OSD, API and audit decisions plus diagrams and dependency map. |
| 2 | Phase 60.15 | Planned next implementation slice | Recording Metadata Preparation | Add provider-neutral metadata and artwork hooks while preserving lazy Recording behavior. |
| 3 | Phase 61 | Planned | Suite Metadata Platform | Build normalized suite-owned metadata, provider, provenance and artwork services. |
| 4 | Phase 62 | Planned | Identity, RBAC and Audit | Add user, service and Agent identities, scoped authorization and mutation audit foundation. |
| 5 | Phase 63 | Planned | Backend Agent and Multi-Site Runtime | Implement Agent enrollment, secure transport, generation, lease, health and fenced commands. |
| 6 | Phase 64 | Planned | Timer Intent and Orchestration | Separate intent, assignment and native timers; add scheduler and reconciler. |
| 7 | Phase 65 | Planned | Streaming Gateway | Add authenticated short-lived media sessions over internal providers. |
| 8 | Phase 66 | Planned | Legacy OSD Bridge | Add isolated compatibility sessions with viewer and controller permissions. |
| 9 | Phase 67 | Planned | Public API and Client Hardening | Stabilize `/api/v1`, errors, revisions, compatibility and client contracts. |
| 10 | Phase 68 | Vision | Recommendation and Knowledge Graph | Add explainable recommendations after metadata and platform foundations mature. |

This table is authoritative for future phase order. Later phases do not start before the dependency and exit criteria in [Roadmap](roadmap.md) are complete.

---

## Numbering Rules

- Completed phase history is never renumbered.
- Phase 58 remains a historical umbrella label only.
- Phase 59 and Phase 60 are already-used implementation ranges and are not reused for new major milestones.
- Future milestones continue sequentially with Phase 61 through Phase 68.
- Architecture ADR work is listed before Phase 60.15 because it defines contracts, not a runtime milestone.

---

## Maintenance Rules

- This file owns phase numbers and compact status.
- [Roadmap](roadmap.md) owns the strict execution order and phase exit criteria.
- [Completed Phases](../development/completed-phases.md) owns chronological implementation history.
- When a new phase completes, update this file and run the phase-map coverage check.
- Accepted ADRs do not by themselves mark runtime phases as complete.

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
