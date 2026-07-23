# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Metadata-Backed Genre Browser](architecture/metadata-genre-browser.md)
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
6. [Metadata-Backed Genre Browser](architecture/metadata-genre-browser.md)
7. [Domain Dependency Map](planning/domain-dependency-map.md)
8. [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
9. [Completed Architecture Source Audit](development/architecture-source-audit-2026-07-15.md)
10. [Current Project Status](development/current-status.md)
11. [ADR Index](adr/index.md)
12. [Current Architecture State](development/current-architecture-state.md)
13. [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
14. [Completed Phases](development/completed-phases.md) when historical detail is required
15. [GitHub Actions Status Handoff](development/github-actions-status-handoff.md) when CI state matters

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
Phase 60.15 - Recording Metadata and Poster Preparation
```

Current feature-branch implementation slice:

```text
Phase 61 - persistent normalized Genre index and metadata-backed Genre browser
Branch: feature/phase61-metadata-genre-browser
Status: implementation under review; repository-local build and real-system acceptance pending
```

Completed architecture prerequisite:

```text
ADR-0042 through ADR-0049
Target Platform Architecture
Domain Dependency Map
Implementation Dependency Map
```

Current runtime implementation phase:

```text
Phase 61 - Suite Metadata Database and External Provider Integration
```

The Phase 58 umbrella label is historical. Future execution follows the strict numbered sequence from Phase 61 onward.

---

## Phase 61 Genre Slice Truth

The feature branch currently implements:

- backend-scoped persistent Genre target bindings and assignments;
- canonical aliases, stable unknown identities and explicit unclassified state;
- multiple Genres per Recording or EPG event;
- active, missing, unknown, stale and derived conflict states;
- SQL distinct counts and limit/offset result pages;
- asynchronous bounded EPG provider enrichment through the existing EPG worker;
- Recording Genre materialization through the existing Recording cache worker;
- provider-neutral Suite REST and DOM-free Client API routes;
- a `genres` frontend module;
- reuse of the existing Recordings 2 card/detail owner;
- reuse of the existing EPG detail card;
- unchanged EPG timeline and preserved PR #99 LiveRemote routing precedence.

Do not describe this slice as completed history until focused tests, daemon build and real-system browser acceptance have passed.

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
- Suite Bridge SB.1 through SB.7 read-only/lifecycle foundations;
- Phase 61 metadata identity/schema foundation;
- feature-branch persistent Genre read-model runtime.

---

## Main Incomplete Areas

The detailed list remains in the [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md).

Major runtime gaps include:

- broader MetadataEntity resolution, provider evidence operations and artwork asset lifecycle beyond the Genre slice;
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

Validate the Phase 61 Genre vertical slice on `/home/yavdr/vdr-suite`:

```text
focused metadata, controller, architecture and frontend tests
full fast regression and daemon build
service restart and SQLite restart persistence
Recording and EPG Genre API counts
large desktop/mobile Genre cards
Recordings 2 and EPG detail return navigation
read-only backend isolation
unchanged EPG timeline and PR #99 live remote/overlay behavior
```

Do not merge until the real-system verification is explicitly approved.

After acceptance, continue the remaining Phase 61 provider, artwork, migration, backup and operational-hardening slices.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Current State](CURRENT.md)
