# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [ADR Index](../adr/index.md)
- [Completed Phases](../development/completed-phases.md)
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)
- [Recording Metadata Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Purpose

This file defines the **forward execution order** of VDR-Suite.

It is not the chronological phase archive. Completed history belongs in [Completed Phases](../development/completed-phases.md), compact phase numbering belongs in [Phase Map](phase-map.md), and detailed prerequisite/slice order belongs in the [Implementation Dependency Map](implementation-dependency-map.md).

The roadmap has one rule:

> Work is read from top to bottom. A later phase does not begin before the required decisions and exit criteria of the earlier phase are complete.

---

## Current Verified Position

```text
Completed major project block
Phase 57 - Multi-Site Backend Administration and Permissions

Historical umbrella implementation track
Phase 58 - Frontend and Live Parity

Latest completed implementation slice
Phase 60.14k - Recording Detail UX Polish

Completed architecture prerequisite
ADR-0042 through ADR-0049 plus target diagrams and dependency maps

Next runtime implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation
```

The Phase 58 umbrella label describes broad product history. It does not order future work.

---

## Completed Architecture Contract Baseline

Accepted decisions:

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

Package-closeout documents:

- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)

The contract package is complete. It does not mark the planned runtime components as implemented.

---

# Strict Execution Order

## Step 1 - Phase 60.15: Recording Metadata and Poster Preparation

Status: **Next runtime implementation slice.**

Goal:

- Prepare the existing Recording API and frontend for metadata and artwork without implementing the full metadata platform yet.

Start with an evidence-first repository audit of:

- current Recording domain objects;
- serializer and REST field ownership;
- Web Client API Recording contracts;
- lazy Recording cache and folder loading;
- current poster/artwork placeholders;
- direct or accidental provider coupling.

Scope:

- separate technical VDR data from normalized and provider-derived metadata;
- introduce provider-neutral artwork references;
- add poster and artwork placeholders that work without provider data;
- keep lazy Recording folder loading and Recording detail navigation unchanged;
- keep external lookup latency outside synchronous list rendering;
- prevent direct frontend coupling to TVScraper, scraper2vdr or provider databases;
- document temporary representation fields and their Phase 61 migration path.

Non-goals:

- no final metadata database schema;
- no provider database as Suite authority;
- no synchronous provider lookup in Recording list rendering;
- no public artwork identity based only on a temporary external URL;
- no RBAC, Agent, Timer scheduler, streaming or OSD runtime work.

Exit criteria:

- technical, normalized and provider-derived fields are distinguishable;
- the Recording API can represent provider-neutral artwork references or explicit placeholders;
- the frontend remains fully usable without enriched metadata;
- existing lazy loading and Recording browser behavior remain covered and green;
- no direct provider coupling is introduced.

---

## Step 2 - Phase 61: Suite Metadata Database and External Providers

Status: Planned after Phase 60.15.

Goal:

- Build the Suite-owned normalized metadata platform defined by ADR-0038.

Implementation order:

1. metadata entity and assignment identities;
2. database schema and migrations;
3. provider, provenance, evidence and confidence contracts;
4. artwork asset identity, storage and delivery;
5. Recording enrichment read model;
6. sidecar, imported and plugin-backed provider adapters;
7. asynchronous refresh, retry and invalidation work;
8. frontend enrichment beyond Phase 60.15 placeholders;
9. migration, backup and operational hardening.

Exit criteria:

- no external provider database is authoritative for VDR-Suite;
- provider failures do not break Recording browsing;
- metadata and artwork are backend neutral;
- stale, disputed and missing provider data have explicit states;
- enrichment work is observable and retryable;
- migrations and recovery behavior are tested.

---

## Step 3 - Phase 62: Identity, RBAC and Accountability Foundation

Status: Planned after Phase 61.

Goal:

- Replace broad backend read-only/read-write hints with production-grade user, service and Agent authorization foundations.

Scope:

- user, service-account, Agent and system actor identities;
- secure sessions and credential references;
- roles, permissions and backend/resource scopes;
- centralized server-side authorization decisions;
- preservation of existing read-only backend policy;
- append-only AccountabilityEvent persistence;
- transactional outbox for protected operations;
- authentication, permission, mutation and security events;
- redaction, retention and protected audit queries.

Exit criteria:

- different users can have different rights on the same backend;
- the second-house read-only use case is enforced server-side;
- every real mutation has an actor, decision and outcome record;
- required pre-dispatch accountability failure prevents dispatch;
- audit queries are scoped and themselves audited;
- Agent credentials can be represented for later enrollment, rotation and revocation.

---

## Step 4 - Phase 63: Backend Agent and Secure Multi-Site Runtime

Status: Planned after Phase 62.

Goal:

- Implement the Control Plane and Backend Agent boundary for remote VDR sites.

Scope:

- Agent enrollment and device identity;
- protected outbound Agent connections;
- protocol negotiation;
- persistent BackendId and native identity mapping;
- backend generation, heartbeat, lease and health;
- capability and snapshot publication;
- durable command inbox and result outbox;
- fenced read-only and later write command dispatch;
- offline, reconnecting and degraded states;
- local provider and Suite Bridge selection;
- no public exposure of RESTfulAPI, SVDRP, Streamdev, osd2web or plugin ports.

Exit criteria:

- a remote read-only backend works through the Agent boundary;
- stale Agent generations cannot complete current commands;
- lease expiry changes backend availability deterministically;
- reconnect deduplicates receipts, results and producer evidence;
- remote writes remain closed unless authorization, revision, idempotency, audit and verification gates pass.

---

## Step 5 - Phase 64: Timer Intent and Multi-Backend Orchestration

Status: Planned after Phase 63.

Goal:

- Separate user and automation intent from backend-native Timer execution.

Scope:

- TimerIntent identity, persistence and lifecycle;
- TimerAssignment identity and persistence;
- NativeTimerBinding observation and ownership;
- explicit external-timer adoption;
- deterministic scheduler and reconciler;
- backend capability, health, channel and event availability checks;
- duplicate and ambiguity handling;
- primary and deliberate replica policy;
- reassignment and failure recovery;
- epgsearch, SearchTimer and other providers producing proposals/intents instead of independent global native writes.

Exit criteria:

- every managed native Timer can be traced to an intent and assignment;
- one active primary assignment exists by default;
- deliberate replicas are explicit;
- backend failure does not silently create duplicate timers;
- native execution is read back and reconciled;
- failover never follows an uncertain prior dispatch without reconciliation.

---

## Step 6 - Phase 65: Streaming Gateway and Media Sessions

Status: Planned after Phase 64.

Goal:

- Provide authenticated Live TV and Recording playback without exposing internal provider endpoints.

Scope:

- MediaResourceRef and MediaSession persistence;
- media authorization;
- MediaRoute and route epoch;
- short-lived MediaAccessGrant lifecycle;
- Streaming Gateway connection ownership;
- Agent provider routes;
- ProviderStreamLease and capacity;
- Live pass-through;
- Recording range, seek, reconnect and growing-file behavior;
- Streamdev as an internal provider only;
- optional remux/transcode boundaries after pass-through is proven.

Exit criteria:

- grants expire and revoke deterministically;
- clients never receive permanent internal provider URLs;
- backend generation and route epoch fence stale access;
- capacity and unavailable providers are explicit;
- Live and Recording playback work through the Agent boundary;
- playback permission remains separate from download/export permission.

---

## Step 7 - Phase 66: Legacy OSD Compatibility Bridge

Status: Planned after Phase 65.

Goal:

- Provide controlled compatibility access to VDR/plugin functions without a native Suite domain UI.

Scope:

- read-only OSD surface snapshots;
- immutable frames and ordered deltas;
- LegacyOsdSession and viewer bindings;
- sequence-gap detection and full resynchronization;
- one fenced OsdControllerLease;
- allowlisted and rate-limited input;
- read-only backend control denial;
- coexistence with physical remote control;
- no arbitrary command tunnel;
- no use as primary Web or TV frontend architecture.

Exit criteria:

- multiple viewers work independently;
- only one Suite controller lease is active per surface and epoch;
- stale generation, OSD epoch or lease epoch is rejected;
- sequence loss recovers through a full snapshot;
- disconnected input is not replayed;
- VDR callback and lock safety is proven live.

---

## Step 8 - Phase 67: Public API and Client Compatibility Hardening

Status: Planned after Phase 66.

Goal:

- Stabilize the platform contract for Web, Windows, Linux, Android, iOS and TV clients.

Scope:

- public response-header abstraction;
- request and correlation ID propagation;
- Problem Details-compatible errors;
- `/api/v1` contract root and capability discovery;
- ETag and If-Match support;
- durable operation and idempotency exposure;
- cursor pagination and partial-result semantics;
- resource-by-resource route migration;
- server-side legacy aliases and deprecation metadata;
- structured Web Client API errors;
- removal of speculative mutation fallback;
- machine-readable schema and compatibility tests.

Per-route gate:

- stable Suite identity;
- explicit backend scope;
- authentication and authorization;
- capability and access-policy enforcement;
- revision behavior for mutable resources;
- stable status and error codes;
- collection semantics where applicable;
- accountability classification;
- no backend, Agent or plugin transport leakage.

Exit criteria:

- bundled clients use canonical `/api/v1` paths;
- mutation clients never try a second route after an uncertain first dispatch;
- request IDs and structured errors survive the client wrapper;
- ETags protect mutable resources;
- legacy routes have explicit compatibility and deprecation status;
- public API, Agent, media, OSD and plugin versions remain separate.

---

## Step 9 - Phase 68: Recommendation and Content Knowledge Graph

Status: Later vision.

Goal:

- Build explainable recommendation and graph features only after metadata, identity, multi-site and API foundations are mature.

Prerequisites:

- stable metadata and artwork identities;
- mature provenance;
- stable Recording, ProgramEvent and Timer identities;
- actor privacy and authorization;
- reliable accountability history;
- stable public API contracts.

Scope direction:

- graph identity and edge vocabulary;
- provenance for graph facts;
- preference and privacy boundaries;
- deterministic non-AI baseline;
- explainable ranking evidence;
- optional provider-neutral AI enrichment;
- local and offline provider support;
- feedback and correction model.

Phase 68 is not part of the immediate implementation work.

---

# Global Completion Gates

## Identity Gate

A persisted or synchronized resource requires:

- a stable Suite identity;
- an explicit backend-native binding where applicable;
- revision and ownership semantics;
- migration behavior.

## Authorization Gate

A protected operation requires:

- authenticated actor context;
- server-side authorization;
- backend access-policy evaluation;
- capability validation;
- scoped accountability evidence.

## Mutation Gate

A real mutation requires:

- explicit target backend when native state is backend-owned;
- stale-state protection;
- idempotency behavior;
- durable operation and dispatch evidence;
- result verification;
- visible unknown-outcome and reconciliation behavior.

## Asynchronous Work Gate

Slow provider, filesystem, network or cross-site work belongs in the durable job model. It must not run inside synchronous list rendering or long-held VDR locks.

## Native VDR Gate

Plugin or VDR-native changes require:

- bounded copied values;
- no raw pointer or lock escape;
- no blocking callback work;
- contract tests;
- final shared-object validation;
- live VDR acceptance and cleanup.

## Source Audit Gate

The broad plugin audit is complete. Additional source audits are performed only for a concrete feature, adapter, migration or risk question.

---

## Maintenance Rules

- [Phase Map](phase-map.md) is the compact phase-number source of truth.
- This roadmap defines the forward work order and exit criteria.
- [Implementation Dependency Map](implementation-dependency-map.md) expands internal slice and prerequisite order.
- [Domain Dependency Map](domain-dependency-map.md) defines cross-domain prerequisite direction.
- [Completed Phases](../development/completed-phases.md) owns chronological runtime history.
- A phase number is not reused or renumbered after implementation.
- Accepted ADRs and target diagrams define direction but do not imply runtime completion.
- Later phases do not begin merely because they can be developed independently.

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
- [Back to Current State](../CURRENT.md)
