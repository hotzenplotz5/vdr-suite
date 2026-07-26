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
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Completed Phases](../development/completed-phases.md)

---

## Purpose

This file defines the strict forward execution order of VDR-Suite.

It is not the chronological archive. Completed history belongs in [Completed Phases](../development/completed-phases.md), compact numbering belongs in the [Phase Map](phase-map.md), and detailed prerequisites belong in the [Implementation Dependency Map](implementation-dependency-map.md).

Roadmap rule:

> Work is read from top to bottom. Later phases may not bypass identity, authorization, accountability, lifecycle fencing or stable-domain prerequisites by moving policy into the frontend, a plugin or an external provider.

---

## Current Verified Position

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

Phase 61 was merged through PR #100 and accepted on the real yaVDR installation. PRs #102 through #108 completed the measured post-phase EPG and metadata hardening. Old text that describes Phase 61 as a feature branch or pending acceptance is stale.

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

The contract package is complete. It defines the requirements for later phases but does not claim those later runtimes already exist.

---

# Strict Execution Order

## Step 1 - Phase 60.15: Recording Metadata and Poster Preparation

Status: **Completed.**

Completed result:

- separated technical, normalized and provider-derived Recording fields;
- introduced provider-neutral artwork references and deterministic placeholders;
- retained lazy folder loading and cached fallback;
- persisted additive metadata through the existing Recording cache;
- provided Suite-owned opaque artwork identities and authenticated local delivery;
- prevented direct frontend coupling to provider databases or filesystem paths.

This step prepared the Recording representation for the persistent metadata/Genre runtime delivered in Phase 61.

---

## Step 2 - Phase 61: Suite Metadata and Genre Platform

Status: **Completed.**

Accepted scope:

- persistent backend-scoped Recording and EPG target bindings;
- canonical Genre assignments with provider and derived evidence;
- explicit active, missing, unknown, stale and conflict states;
- persistent TVScraper media-type evidence and derived EPG browse classes;
- indexed backend-scoped counts and limit/offset result pages;
- provider-neutral Suite REST and DOM-free Web Client API routes;
- Recording and EPG Genre frontend navigation;
- bounded asynchronous EPG enrichment and Recording cache materialization;
- restart persistence and real-system acceptance;
- preservation of existing Recording, EPG timeline and LiveRemote ownership.

Completion evidence:

- PR #100 merged the accepted Phase 61 vertical slice;
- focused, regression, architecture and daemon tests passed during acceptance;
- the installed runtime was restarted and verified on the real yaVDR system;
- SQLite persistence, real counts, navigation and ownership boundaries were accepted;
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md) records the final scope and evidence.

Post-phase hardening:

```text
Post-Phase 61 Performance Hardening (B1-B4)
```

PRs #102 through #108 added query fast paths, atomic writes, no-op synchronization, integer-window indexing, no-op event upserts and a 15-minute completed-snapshot cadence. This is a completed operational hardening block, not a new numbered phase.

Completion boundary:

- Phase 61 is closed for the accepted metadata/Genre platform scope;
- optional future provider adapters do not silently reopen it;
- recommendations remain Phase 68 work;
- long-term diagnostics and broader operational tooling remain backlog unless assigned explicitly.

---

## Step 3 - Phase 62: Identity, RBAC and Accountability Foundation

Status: **Next.**

Goal:

- replace broad backend access hints with production-grade actor identity, scoped authorization and structured accountability foundations.

Implementation order:

1. `ActorIdentity` and actor-type value objects;
2. user, service-account, Agent and system-actor persistence;
3. authentication-session and credential-reference contracts;
4. role, permission and backend/resource scope models;
5. centralized `AuthorizationDecision` service;
6. server-side enforcement adapters preserving current read-only behavior;
7. actor, request and correlation context propagation;
8. append-only `AccountabilityEvent` catalogue and schema;
9. transactional outbox for protected operations;
10. authentication, authorization, mutation and security events;
11. protected queries, redaction, retention and audit-of-audit;
12. deny-path, outage and failure-injection acceptance.

Exit criteria:

- different users can have different rights on the same backend;
- denial is enforced server-side rather than in clients;
- the current second-house/read-only scenario remains enforced;
- every real privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch accountability failure prevents dispatch;
- later Agent credentials can be enrolled, rotated and revoked without redesigning identity.

Forbidden shortcuts:

- no frontend-owned role or permission decision;
- no parsing ordinary logs as the accountability database;
- no new remote privileged dispatch before authorization and accountability gates exist;
- no weakening of existing read-only backend enforcement during migration.

---

## Step 4 - Phase 63: Backend Agent and Secure Multi-Site Runtime

Status: **Planned after Phase 62.**

Goal:

- implement the Control Plane and enrolled Backend Agent boundary for remote VDR sites.

Scope:

- Agent enrollment, device identity and credential lifecycle;
- protected outbound Agent connections and protocol negotiation;
- persistent backend identity, generation, heartbeat, lease and health;
- capability, snapshot and change publication;
- durable command inbox and result outbox;
- fenced read-only operations before write operations;
- offline, reconnecting and degraded states;
- local provider and SuiteBridge selection;
- no public exposure of RESTfulAPI, SVDRP, Streamdev, osd2web or plugin ports.

Exit criteria:

- a remote read-only backend works through the Agent boundary;
- stale generations cannot complete current commands;
- lease expiry changes availability deterministically;
- reconnect deduplicates receipts, results and producer evidence;
- remote writes remain closed unless authorization, revision, idempotency, accountability and verification gates pass.

---

## Step 5 - Phase 64: Timer Intent and Multi-Backend Orchestration

Status: **Planned after Phase 63.**

Goal:

- separate user and automation intent from backend-native Timer execution.

Scope:

- `TimerIntent`, `TimerAssignment` and `NativeTimerBinding` persistence;
- explicit external-timer adoption and provenance;
- deterministic scheduler and reconciler;
- backend capability, health, channel and event eligibility;
- duplicate, ambiguity, primary and deliberate replica policy;
- operation/job binding, native readback and drift classification;
- SearchTimer and epgsearch proposals producing intents rather than bypassing orchestration;
- reassignment and uncertain-dispatch recovery.

Exit criteria:

- every managed native Timer traces to an intent and assignment;
- one active primary assignment exists by default;
- deliberate replicas are explicit;
- backend failure does not silently create duplicates;
- native execution is read back and reconciled;
- failover does not follow an unresolved prior dispatch.

---

## Step 6 - Phase 65: Streaming Gateway and Media Sessions

Status: **Planned after Phase 64.**

Goal:

- provide authenticated Live TV and Recording playback without exposing internal provider endpoints.

Scope:

- `MediaResourceRef`, `MediaSession`, `MediaRoute` and route epoch;
- media authorization and short-lived access grants;
- Streaming Gateway connection ownership;
- Agent provider routes and capacity leases;
- Live pass-through;
- Recording range, seek, reconnect and growing-file behavior;
- Streamdev as an internal provider only;
- optional remux/transcode after pass-through is proven.

Exit criteria:

- grants expire and revoke deterministically;
- clients never receive permanent internal provider URLs;
- backend generation and route epoch fence stale access;
- unavailable providers and capacity exhaustion are explicit;
- Live and Recording playback work through the Agent boundary;
- playback permission remains separate from download/export permission.

---

## Step 7 - Phase 66: Legacy OSD Compatibility Bridge

Status: **Planned after Phase 65.**

Goal:

- provide controlled compatibility access to VDR/plugin functions that lack a native Suite domain UI.

Scope:

- read-only OSD surface snapshots;
- immutable frames and ordered deltas;
- session and viewer bindings;
- sequence-gap detection and full resynchronization;
- one fenced controller lease;
- allowlisted, rate-limited input;
- read-only backend control denial;
- coexistence with the physical remote;
- no arbitrary command tunnel.

Exit criteria:

- multiple viewers work independently;
- only one current controller lease can act;
- stale generation, surface epoch or lease epoch is rejected;
- sequence loss recovers through a full snapshot;
- disconnected input is not replayed;
- VDR callback and lock safety is proven live.

---

## Step 8 - Phase 67: Public API and Client Compatibility Hardening

Status: **Planned after Phase 66.**

Goal:

- stabilize public contracts for Web, desktop, mobile and TV clients.

Scope:

- request and correlation IDs;
- Problem Details-compatible structured errors;
- `/api/v1` contract root and capability discovery;
- ETag and `If-Match` support;
- durable operation and idempotency exposure;
- cursor pagination and partial-result semantics;
- resource-by-resource route migration;
- server-side legacy aliases and deprecation metadata;
- structured Web Client API errors;
- machine-readable schema and compatibility tests.

Exit criteria:

- bundled clients use canonical `/api/v1` paths;
- mutations never try a speculative second route after uncertain dispatch;
- request IDs and structured errors survive client wrappers;
- ETags protect mutable resources;
- legacy routes have explicit compatibility status;
- public API, Agent, media, OSD and plugin versions remain separate.

---

## Step 9 - Phase 68: Recommendation and Content Knowledge Graph

Status: **Later vision.**

Goal:

- build explainable recommendation and graph features only after metadata, identity, multi-site and API foundations are mature.

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
- local/offline provider support;
- feedback and correction model.

Phase 68 is not immediate implementation work.

---

# Cross-Cutting Completion Gates

## Identity Gate

Every persisted or synchronized resource needs a stable Suite identity and explicit backend-native binding where applicable.

## Provider Gate

Provider data must carry provenance and state. No provider database becomes hidden Suite authority.

## Mutation Gate

Privileged operations require authorization, revision handling, idempotency, durable dispatch evidence, verification and accountability.

## Native Boundary Gate

VDR callback and plugin boundaries remain bounded. No raw VDR pointer or lock crosses into asynchronous, network or database work.

## Client Gate

Clients consume Suite-owned contracts. They do not learn private backend plugin URLs, credentials, filesystem paths or transport details.

## Acceptance Gate

A phase closes only after focused tests, regression coverage, packaging/build validation and real-system acceptance where native behavior changes.

---

## Ecosystem Progress View

The maintained comparison with VDR Core, Live, epgsearch and RESTfulAPI is [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md).

Current summary:

- VDR-Suite is strong in backend-neutral reads, Recording workflows, metadata/Genre browsing, safety boundaries and multi-backend policy foundations;
- it is not yet a full Live replacement because streaming, legacy OSD and several polished end-user workflows remain incomplete;
- epgsearch workflow foundations are strong, but exact semantic parity and central Timer orchestration remain later work;
- RESTfulAPI remains an important private adapter, not the public platform contract;
- Phase 62 is the next prerequisite before secure multi-user and later Agent-backed operations.

---

## Maintenance Rules

- The [Phase Map](phase-map.md) owns numbering and compact status.
- This file owns strict execution order and exit criteria.
- The [Implementation Dependency Map](implementation-dependency-map.md) owns detailed prerequisites and slice ordering.
- [Completed Phases](../development/completed-phases.md) owns completed history.
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md) owns the accepted Phase 61 and B1-B4 evidence.
- Completed phases are not silently reopened by optional extensions.

Verification:

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

---

## Back

- [Back to Planning Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)