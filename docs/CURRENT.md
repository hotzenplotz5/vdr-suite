# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Domain Dependency Map](planning/domain-dependency-map.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](development/completed-phases.md)
- [Architecture Decision Records](adr/index.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)

---

## Current Verified Position

```text
Latest completed implementation phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next implementation focus:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 is merged into `main` through PR #100 and was accepted on the real yaVDR installation. PRs #102 through #108 completed the measured EPG and metadata performance hardening. The old feature-branch and pending-acceptance wording is obsolete.

---

## Verified Runtime Foundation

The repository and installed runtime currently include:

- daemon-owned SQLite persistence and migration foundations;
- backend-neutral REST controllers and Web Client API wrappers;
- BackendNode, BackendRegistry and server-enforced read-only backend access mode;
- backend-aware status, channels, EPG, recordings and timers;
- snapshot cache, partial refresh, change feed and SSE/live transport foundations;
- lazy SQLite-backed Recording cache and hierarchical Recording browser;
- guarded Recording rename, move and VDR-trash workflows;
- Recording metadata, people, poster references and authenticated artwork delivery;
- persistent Recording and EPG Genre target bindings and assignments;
- provider and derived Genre evidence with explicit state handling;
- indexed metadata-backed Genre browsing for Recordings and EPG;
- TVScraper-backed EPG detail metadata without direct browser/provider coupling;
- SearchTimer list, discovery, preview, validation and controlled mutation foundations;
- native Timer action boundaries and readback verification foundations;
- backend-neutral remote-control actions and live-overlay snapshots;
- modular frontend ownership and deferred runtime loading;
- packaging, staging, manpages and real-system acceptance workflows.

---

## Completed Foundation Continuity

The current runtime builds on these completed major foundations:

- Phase 56 - Library Boundary, Packaging and Developer Documentation
- Phase 57 - Multi-Site Backend Administration and Permissions

Phase 56 established source, package, install and developer boundaries.
Phase 57 established backend administration and server-enforced
read-only access foundations used by the later runtime phases.

---

## Phase 61 Runtime Truth

The accepted Phase 61 path is:

```text
persistent Recording and EPG sources
  -> backend-scoped bindings
  -> provider and derived evidence
  -> canonical Genre assignments
  -> indexed SQL counts and pages
  -> Suite REST and Web Client API
  -> Genre navigation
  -> existing Recording and EPG detail owners
```

The phase established the first complete Suite-owned metadata/Genre vertical slice. External providers remain acquisition sources, not Suite authority.

See [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md) for implementation scope, measurements, PR mapping and live acceptance evidence.

---

## Performance State

The completed B1-B4 block provides:

- a measured 3.24x EPG refresh-candidate query improvement;
- one atomic Genre write transaction per enriched EPG candidate instead of three;
- no write transaction for unchanged Recording Genre synchronization;
- an approximately 19.6x production EPG integer-window query improvement;
- zero row updates for identical EPG event upserts;
- a 15-minute idle interval between completed TVScraper type-snapshot cycles while incomplete cursors continue every ten seconds.

These claims are bounded to the recorded production measurements and regression contracts in the closeout document.

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

The accepted package defines future ownership and safety requirements. It does not claim that later runtime phases are already implemented.

---

## Main Incomplete Areas

The major remaining runtime gaps are:

- real user, service and Agent identities with scoped RBAC;
- centralized authorization decisions beyond current backend access modes;
- append-only accountability persistence and transactional outbox;
- Backend Agent enrollment, secure transport, generation, lease and reconnect runtime;
- universal revision and durable idempotency across privileged mutations;
- production job claims, retry, compensation and reconciliation;
- TimerIntent, TimerAssignment, scheduler and reconciler;
- authenticated Streaming Gateway and media sessions;
- hardened legacy OSD viewing/control bridge;
- versioned `/api/v1`, ETags, common errors and compatibility metadata;
- broader optional metadata/provider adapters and long-term diagnostics;
- recommendation and knowledge-graph behavior.

The detailed external comparison is maintained in [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md).

---

## Immediate Repository Work

The next runtime implementation phase is:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

The first Phase 62 slices should establish:

1. actor identity value objects and persistence;
2. role, permission and backend/resource scope models;
3. a centralized server-side authorization decision service;
4. actor/request/correlation context propagation;
5. append-only AccountabilityEvent persistence;
6. a transactional outbox for protected operations;
7. deny-path, read-only and failure-injection tests.

No later Agent, remote privileged command or multi-site write path should bypass these foundations.

---

## Documentation Reading Rule

Before planning new work, read in this order:

1. [Current State](CURRENT.md)
2. [Strict Roadmap](planning/roadmap.md)
3. [Phase Map](planning/phase-map.md)
4. [Implementation Dependency Map](planning/implementation-dependency-map.md)
5. [Target Platform Architecture](architecture/target-platform-architecture.md)
6. [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
7. [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
8. [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
9. [Completed Phases](development/completed-phases.md)

---

## Boundary Rules

- VDR remains the native runtime authority.
- VDR-Suite owns the external domain, policy, orchestration and client-facing contracts.
- RESTfulAPI, SVDRP, Streamdev, TVScraper and SuiteBridge are private backend/provider boundaries.
- Stable Suite identity remains separate from backend-native identity.
- Provider data must not become hidden Suite authority.
- Frontend code must not own authorization or provider transport decisions.
- Completed phases are not silently reopened; deferred extensions require an explicit later phase or backlog assignment.

---

## Back

- [Back to Documentation Index](index.md)
- [Back to README](../README.md)