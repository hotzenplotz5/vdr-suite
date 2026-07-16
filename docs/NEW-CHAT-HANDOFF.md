# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Completed Architecture Source Audit](development/architecture-source-audit-2026-07-15.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)
- [Completed Phases](development/completed-phases.md)
- [GitHub Actions Status Handoff](development/github-actions-status-handoff.md)

---

## Purpose

This is the compact project handoff that a new chat should read first.

It does not replace specialized implementation, acceptance or CI handoffs.

---

## Required First Reading

Read these files in this order:

1. [Current State](CURRENT.md)
2. [Strict Roadmap](planning/roadmap.md)
3. [Phase Map](planning/phase-map.md)
4. [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
5. [Completed Architecture Source Audit](development/architecture-source-audit-2026-07-15.md)
6. [Current Project Status](development/current-status.md)
7. [ADR Index](adr/index.md)
8. [Current Architecture State](development/current-architecture-state.md)
9. [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
10. [Completed Phases](development/completed-phases.md) when historical detail is required
11. [GitHub Actions Status Handoff](development/github-actions-status-handoff.md) when CI state matters

---

## Current Repository Truth

Latest completed major project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Previous completed major project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Historical umbrella implementation track:

```text
Phase 58 - Frontend and Live Parity
```

Latest completed implementation slice:

```text
Phase 60.14k - Recording Detail UX Polish
```

Next planned runtime implementation slice:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

The Phase 58 umbrella label is historical. Future execution follows the strict numbered sequence from Phase 60.15 onward.

---

## Completed Architecture Audit

The completed 2026-07-15 source audit covered:

- VDR Core;
- epgsearch;
- Live;
- RESTfulAPI;
- Streamdev;
- TVScraper;
- scraper2vdr;
- osd2web;
- epg2vdr;
- epgd.

Use:

- [Architecture Source Audit](development/architecture-source-audit-2026-07-15.md) for the completed evidence and conclusions;
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md) for current implementation gaps and their target phases.

Broad plugin auditing is complete. Additional audits require a concrete feature, adapter, migration or risk question.

---

## Accepted Architecture Package

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
```

Core conclusions:

- VDR remains the native runtime authority.
- VDR-Suite remains the external domain, orchestration and platform layer.
- RESTfulAPI, SVDRP, Streamdev, TVScraper, epgsearch and OSD stay behind adapter or provider boundaries.
- Multi-site production architecture uses a Control Plane and local Backend Agents.
- Agents do not receive direct central database access.
- Remote sites do not expose VDR plugin ports as public platform APIs.
- Stable BackendId, native identity, runtime generation, lease and health are separate concepts.
- Read-only backend policy remains server-enforced.
- Metadata is normalized into suite-owned entities and assets while acquisition remains provider-based.

---

## Verified Runtime Foundation

Do not describe these areas as wholly missing:

- daemon and REST runtime;
- RESTfulAPI adapter boundary;
- BackendNode and BackendRegistry;
- backend-aware snapshots and reads;
- snapshot change feed and SSE foundation;
- runtime diagnostics;
- Recording request, preview, validation, planning and execution boundaries;
- guarded real-backend Recording probes;
- native Timer action boundaries;
- backend-neutral SearchTimer workflows;
- backend-scoped EPG cache and queries;
- lazy SQLite-backed Recording cache;
- Web Client API wrapper;
- frontend module ownership and registry;
- server-enforced read-only backend access mode.

---

## Main Incomplete Areas

The detailed list is in the 30-row [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md).

Major gaps include:

- real user, service and Agent identity with scoped RBAC;
- Backend Agent enrollment and secure remote transport;
- backend generation, heartbeat, lease and health runtime;
- expected revision and idempotency for mutations;
- production job claim, retry, verification and compensation;
- TimerIntent, TimerAssignment, scheduler and reconciler;
- canonical ProgramEvent identity and provenance;
- suite-owned metadata and artwork services;
- Streaming Gateway;
- hardened Legacy OSD bridge;
- versioned public API contract;
- mutation audit and security events.

---

## Immediate Repository Work

Complete this architecture contract package before Phase 60.15:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
ADR-0046 - Streaming Gateway and Media Session Boundary
ADR-0047 - Legacy OSD Compatibility Bridge
ADR-0048 - Public API Versioning, Error and Compatibility Contract
ADR-0049 - Audit and Security Event Model
```

Then update architecture diagrams and domain and implementation dependency maps.

---

## Strict Future Sequence

```text
1. ADR-0042 through ADR-0049 and diagrams
2. Phase 60.15 - Recording Metadata Preparation
3. Phase 61 - Suite Metadata Platform
4. Phase 62 - Identity, RBAC and Audit
5. Phase 63 - Backend Agent and Multi-Site Runtime
6. Phase 64 - Timer Intent and Orchestration
7. Phase 65 - Streaming Gateway
8. Phase 66 - Legacy OSD Bridge
9. Phase 67 - Public API and Client Hardening
10. Phase 68 - Recommendation and Knowledge Graph
```

Do not begin a later phase merely because it can be developed independently. The dependency order is authoritative.

---

## Project Workflow Rules

- Architecture and cause analysis come before code changes.
- Inspect current repository state instead of relying on old chat summaries.
- Use the next free canonical ADR number from `docs/adr/index.md`.
- Do not create duplicate or lowercase ADR sequences.
- Keep RESTfulAPI and all plugins behind adapter boundaries.
- Keep frontend modules free of direct backend fetch ownership.
- Preserve backend identity in all multi-backend reads and actions.
- Enforce read-only and RBAC decisions in backend services, not only in the UI.
- Recording and Timer writes remain guarded and explicit.
- Real destructive probes remain closed by default.
- Update Completed Phases only for finished runtime implementation.
- Update the Gap Matrix when repository evidence changes.

---

## Documentation Verification

For documentation and ADR changes, run at least:

```bash
make test-docs
make test-phase-map-coverage
make test-phase
```

Also run the full fast regression and daemon build before merging broad documentation changes that touch guard scripts or test fixtures.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Current State](CURRENT.md)
