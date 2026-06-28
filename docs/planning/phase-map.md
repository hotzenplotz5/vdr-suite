# VDR-Suite Phase Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)

---

## Purpose

This file is the canonical compact phase map for VDR-Suite.

It exists so new chats, documentation updates and roadmap changes can quickly see what is completed, what is current and what is planned without reconstructing state from the full chronological phase history.

Detailed phase-by-phase history remains in [Completed Phases](../development/completed-phases.md).

---

## Completed Phase Ranges

| Range | Status | Track | Result |
| --- | --- | --- | --- |
| Phase 1.x-7.x | Completed | Core Platform | Database, repositories, services, REST boundaries and daemon foundation. |
| Phase 8.x | Completed | VDR Backend | VDR domain objects, adapter boundaries and RESTfulAPI integration foundation. |
| Phase 9.x-29.x | Completed | Multi-Backend Runtime | Backend registry, snapshots, change feed and live transport foundation. |
| Phase 30.x-36.x | Completed | Recording Actions | Recording action validation and guarded execution foundation. |
| Phase 37.x-44.x | Completed | Recording Runtime Hardening | Recording action runtime completion, real-backend validation, regression coverage and safety transition. |
| Phase 45.x | Completed | EPG Search | Selective EPG query/search foundation and EPG REST API surface. |
| Phase 46.x | Completed | Metadata and People | Content classification, metadata domain foundations, person metadata, recording-person search and character search. |
| Phase 47.x-49.x | Completed | SearchTimer Backend | SearchTimer backend foundation, RESTfulAPI compatibility, real VDR validation and native capability validation. |
| Phase 50.0-50.50 | Completed | SearchTimer Workflow | User workflow, dry-run and execution planning, safety gates, readback verification and controlled execution. |
| Phase 51.x | Completed | Live Parity Discovery | Live plugin parity discovery and gap visibility foundation. |
| Phase 52.x | Completed | SearchTimer Automation Planning | Read-only planning, dry-run models, preview endpoint, daemon scheduling plan and automation safety review. |
| Phase 53.x | Completed | SearchTimer Completion Audit | Title-only REST/workflow preservation and completion audit. |
| Phase 54.x | Completed | SearchTimer Preview Runtime | Preview runtime, mutation policy wiring and warm EPG cache architecture. |
| Phase 55.0-55.4e | Completed | Adapter and Runtime Hardening | Feature parity and adapter audit, RESTfulAPI contract fixes, discovery wiring and daemon shutdown reset guardrail. |
| Phase 55.5a-55.5n | Completed | Acceptance and Documentation | Preview engine contract, native preview capability, real VDR acceptance, lifecycle hardening and roadmap historical coverage. |
| Phase 55.5o | Completed | Phase Map and Roadmap | Canonical phase map, simplified roadmap and phase-map coverage guardrail. |
| Phase 55.6 | Completed | Recording Operations Audit | Recording mutation safety policy, default-blocked real write probes and guardrail coverage. |

---

## Current and Planned Phase Ranges

| Range | Status | Track | Goal |
| --- | --- | --- | --- |
| Phase 56 | Next | Library Boundary and Packaging | Separate reusable libraries from daemon/application surfaces. |
| Phase 57 | Planned | Multi-Site Backend Administration and Permissions | Support multi-site operation with backend-specific permissions and read-only secondary-site mode. |
| Phase 58 | Planned | Frontend and Live Parity | Build frontend-ready everyday recording, timer, channel and EPG views after safe API and permission foundations. |
| Phase 59 | Planned | Suite Metadata Database and External Providers | Build a suite-owned metadata database while using mature external scraper/catalog providers behind boundaries. |
| Phase 60 | Vision | Recommendation and Content Knowledge Graph | Build recommendation and graph primitives after metadata and frontend foundations mature. |

---

## Maintenance Rules

- This file is the compact source of truth for phase-range coverage.
- The roadmap should summarize this file instead of duplicating completed milestone details.
- Planned phase numbers must not conflict with completed phase ranges.
- When a new completed phase range appears, update this file and run the phase-map coverage check.

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
