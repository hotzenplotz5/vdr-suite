# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Domain Dependency Map](planning/domain-dependency-map.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Completed Phases](development/completed-phases.md)

---

## Purpose

This is the compact project handoff that a new chat should read first. It records current repository truth and prevents old feature-branch or pending-acceptance assumptions from being repeated.

---

## Required First Reading

Read these files in this order:

1. [Current State](CURRENT.md)
2. [Strict Roadmap](planning/roadmap.md)
3. [Phase Map](planning/phase-map.md)
4. [Implementation Dependency Map](planning/implementation-dependency-map.md)
5. [Target Platform Architecture](architecture/target-platform-architecture.md)
6. [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
7. [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
8. [Domain Dependency Map](planning/domain-dependency-map.md)
9. [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
10. [Completed Phases](development/completed-phases.md)

---

## Current Repository Truth

```text
Latest completed runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next runtime implementation phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 is merged and accepted. Do not describe it as a feature branch, implementation under review or pending real-system acceptance.

PR #100 merged the metadata-backed Genre runtime. PRs #102 through #108 completed the production-measured EPG and metadata performance hardening.

---

## Phase 61 Completed Scope

The accepted Phase 61 vertical slice includes:

- persistent backend-scoped Recording and EPG Genre bindings and assignments;
- canonical aliases, unknown identities and unclassified state;
- multiple Genres per target and explicit active/missing/unknown/stale/conflict states;
- provider evidence and derived EPG browse-class persistence;
- indexed distinct counts and paged backend-scoped queries;
- provider-neutral Suite REST and DOM-free Web Client API routes;
- Genre frontend navigation for Recordings and EPG;
- bounded asynchronous EPG enrichment through the daemon worker;
- Recording Genre materialization through the Recording cache worker;
- restart persistence and real-system acceptance;
- preserved Recording, EPG timeline and LiveRemote owners.

The closeout does not claim every future provider adapter, recommendation feature or long-term diagnostics capability.

---

## Performance Closeout Truth

The B1-B4 hardening block established:

- 3.24x faster measured EPG refresh-candidate selection;
- one atomic Genre write transaction per enriched EPG candidate;
- no writer transaction for unchanged Recording Genre synchronization;
- approximately 19.6x faster measured integer EPG window query;
- zero SQLite updates for identical EPG event upserts;
- a 15-minute pause between completed ETYPES cycles, with ten-second continuation for incomplete cursors.

Use the exact measurements and qualifications in [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md); do not turn a single startup timing comparison into a general benchmark claim.

---

## Verified Runtime Foundation

Do not describe these areas as wholly missing:

- daemon and REST runtime;
- RESTfulAPI adapter boundary;
- BackendNode and BackendRegistry;
- server-enforced read-only backend access mode;
- backend-aware snapshots, EPG cache and Recording cache;
- snapshot change feed and SSE foundation;
- guarded Recording actions;
- native Timer action boundaries;
- SearchTimer list, preview, validation and controlled mutation foundations;
- metadata, people and artwork read paths;
- persistent Genre read model;
- remote-control and live-overlay foundations;
- Web Client API wrapper and modular frontend ownership;
- packaging, staging and real-system acceptance workflows.

---

## Accepted Architecture Package

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

The accepted contracts and diagrams define later ownership and safety requirements. They do not mean the later Agent, streaming, OSD or `/api/v1` runtime phases are already implemented.

---

## Main Incomplete Areas

The next major runtime gaps are:

- actor identities, scoped RBAC and centralized authorization;
- append-only accountability persistence and transactional outbox;
- secure Backend Agent enrollment, generation, lease and reconnect runtime;
- universal revision and durable idempotency for privileged mutations;
- production job claim/retry/reconciliation infrastructure;
- TimerIntent, TimerAssignment and scheduler/reconciler;
- Streaming Gateway and media sessions;
- legacy OSD viewing/control bridge;
- versioned `/api/v1` and client compatibility hardening;
- recommendation and knowledge-graph behavior.

---

## Immediate Work

The strict next runtime phase is:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Begin with actor identity and authorization contracts before migrating privileged operations. Append-only accountability and pre-dispatch evidence must precede later Agent-backed remote writes.

---

## Ecosystem Comparison Rule

The canonical comparison targets are:

```text
VDR Core
Live
VDR-Suite
epgsearch
RESTfulAPI
```

There is no VDR-Suite component named `Wikipedia Search`. When that phrase appears in conversation, verify whether `epgsearch` was intended rather than creating a fictitious comparison target.

Use [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md) for current status and open gaps.

---

## Boundary Rules

- VDR remains native runtime authority.
- VDR-Suite remains the external domain, policy and orchestration layer.
- Browsers do not call RESTfulAPI, SVDRP, Streamdev, TVScraper or SuiteBridge directly.
- Provider data is evidence, not hidden Suite authority.
- Frontend code does not own authorization decisions.
- Completed phases are not reopened by optional extensions.
- Claims about parity require source, test or live evidence.

---

## Back

- [Back to Current State](CURRENT.md)
- [Back to Documentation Index](index.md)
- [Back to README](../README.md)