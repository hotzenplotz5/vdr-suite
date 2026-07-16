# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Domain Dependency Map](planning/domain-dependency-map.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
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
3. [Implementation Dependency Map](planning/implementation-dependency-map.md)
4. [Phase Map](planning/phase-map.md)
5. [Target Platform Architecture](architecture/target-platform-architecture.md)
6. [Domain Dependency Map](planning/domain-dependency-map.md)
7. [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
8. [Completed Architecture Source Audit](development/architecture-source-audit-2026-07-15.md)
9. [Current Project Status](development/current-status.md)
10. [ADR Index](adr/index.md)
11. [Current Architecture State](development/current-architecture-state.md)
12. [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
13. [Completed Phases](development/completed-phases.md) when historical detail is required
14. [GitHub Actions Status Handoff](development/github-actions-status-handoff.md) when CI state matters

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

Completed architecture prerequisite:

```text
ADR-0042 through ADR-0049
Target Platform Architecture
Domain Dependency Map
Implementation Dependency Map
```

Next runtime implementation slice:

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

- [Architecture Source Audit](development/architecture-source-audit-2026-07-15.md) for completed evidence and conclusions;
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md) for current implementation gaps and target phases;
- [Target Platform Architecture](architecture/target-platform-architecture.md) for accepted ownership and communication diagrams;
- [Domain Dependency Map](planning/domain-dependency-map.md) for stable conceptual prerequisites;
- [Implementation Dependency Map](planning/implementation-dependency-map.md) for runtime slice order.

Broad plugin auditing is complete. Additional audits require a concrete feature, adapter, migration or risk question.

---

## Accepted Architecture Package

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
ADR-0046 - Streaming Gateway and Media Session Boundary
ADR-0047 - Legacy OSD Compatibility Bridge
ADR-0048 - Public API Versioning, Error and Compatibility Contract
ADR-0049 - Audit and Security Event Model
```

Core conclusions:

- VDR remains the native runtime authority.
- VDR-Suite remains the external domain, orchestration and platform layer.
- The Control Plane owns global identity, policy, orchestration and public contracts.
- Backend Agents are enrolled, generation-bound site representatives.
- RESTfulAPI, SVDRP, Streamdev, osd2web and plugin contracts remain internal to the Agent/site boundary.
- Agents and plugins do not access the central database directly.
- Remote sites do not expose VDR plugin ports as public platform APIs.
- Stable Suite identity is separate from backend-native identity.
- Backend generation, lease, health, resource revision and event sequence are distinct.
- Read-only and RBAC decisions are enforced server-side.
- Mutations require authorization, revision, idempotency, durable dispatch evidence, verification and accountability.
- Unknown outcomes reconcile before retry.
- Metadata and artwork are Suite-owned while acquisition remains provider based.
- TimerIntent and TimerAssignment are separate from native Timers.
- ProgramEvent is separate from BackendEventRef and MetadataEntity.
- Media and Legacy OSD use isolated short-lived session boundaries.
- Public `/api/v1`, Agent, media, OSD and plugin contracts are separately versioned.
- Accountability history is structured and append-only, not parsed from logs.

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
- server-enforced read-only backend access mode;
- Suite Bridge SB.1 through SB.7 read-only/lifecycle foundations.

---

## Main Incomplete Areas

The detailed list remains in the [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md).

Major runtime gaps include:

- Suite-owned metadata entities, assignments, artwork and provenance;
- real user, service and Agent identity with scoped RBAC;
- append-only accountability persistence and outbox;
- Backend Agent enrollment and secure remote transport;
- backend generation, heartbeat, lease and health runtime;
- universal revision and idempotency for mutations;
- production job claim, retry, verification and compensation;
- TimerIntent, TimerAssignment, scheduler and reconciler;
- canonical ProgramEvent persistence and resolver;
- Streaming Gateway;
- hardened Legacy OSD bridge;
- versioned `/api/v1` migration and compatibility runtime.

Accepted ADRs and target diagrams do not close these gaps.

---

## Immediate Repository Work

Start Phase 60.15 with an evidence-first audit of:

```text
Recording domain objects
Recording serializers and REST representations
Web Client API Recording contracts
lazy Recording loading and cache ownership
current poster and artwork placeholders
metadata/provider coupling risks
```

The first slice must define technical, normalized and provider-derived field ownership before adding provider integration.

Preserve:

- Recording browsing without providers;
- lazy folder loading;
- backend scope;
- frontend module ownership;
- provider-neutral architecture;
- existing Recording regression tests.

Do not begin Phase 60.15 by wiring the frontend directly to TVScraper, scraper2vdr or another provider database.

---

## Strict Future Sequence

```text
1. Phase 60.15 - Recording Metadata Preparation
2. Phase 61 - Suite Metadata Platform
3. Phase 62 - Identity, RBAC and Audit
4. Phase 63 - Backend Agent and Multi-Site Runtime
5. Phase 64 - Timer Intent and Orchestration
6. Phase 65 - Streaming Gateway
7. Phase 66 - Legacy OSD Bridge
8. Phase 67 - Public API and Client Hardening
9. Phase 68 - Recommendation and Knowledge Graph
```

Do not begin a later phase merely because it can be developed independently. The dependency order is authoritative.

---

## Project Workflow Rules

- Architecture and cause analysis come before code changes.
- Inspect current repository state instead of relying on old chat summaries.
- Use the next free canonical ADR number only for a genuinely new long-term decision.
- Do not create duplicate or lowercase ADR sequences.
- Keep RESTfulAPI and all plugins behind adapter boundaries.
- Keep frontend modules free of direct backend/provider fetch ownership.
- Preserve backend identity in all multi-backend reads and actions.
- Enforce read-only and RBAC decisions in backend services, not only in the UI.
- Recording and Timer writes remain guarded and explicit.
- Real destructive probes remain closed by default.
- Update Completed Phases only for finished runtime implementation.
- Update the Gap Matrix only when repository implementation evidence changes.
- Target architecture is not current runtime truth.

---

## Documentation Verification

For broad architecture and planning documentation changes, run at least:

```bash
make test-docs
make test-phase-map-coverage
make test-phase
```

Also run the full fast regression, daemon build and packaging staging before merge.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Current State](CURRENT.md)
