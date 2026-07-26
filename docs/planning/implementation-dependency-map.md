# VDR-Suite Implementation Dependency Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Current State](../CURRENT.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)

---

## Purpose

This document translates ADR-0042 through ADR-0049 into runtime prerequisites, bounded slices and forbidden shortcuts. It distinguishes completed foundations from future implementation.

---

## Governing Sequence

```text
completed architecture contracts and dependency maps
  -> completed Phase 60.15 Recording metadata preparation
  -> completed Phase 61 Suite metadata and Genre platform
  -> completed Post-Phase 61 Performance Hardening (B1-B4)
  -> next Phase 62 Identity, RBAC and accountability
  -> Phase 63 Backend Agent and multi-site runtime
  -> Phase 64 Timer intent and orchestration
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD bridge
  -> Phase 67 Public API and client hardening
  -> Phase 68 Recommendation and knowledge graph
```

Later phases may not move policy into the frontend, Agent, plugin or provider to bypass an earlier prerequisite.

---

## Existing Foundations to Reuse

The next phases build on:

- daemon composition and HTTP server boundary;
- SQLite repositories, migrations and transaction lease;
- BackendNode, BackendRegistry and backend access modes;
- backend-aware snapshots, partial refresh and change feed;
- RESTfulAPI adapter and HTTP abstractions;
- guarded Recording request, preview, validation and execution;
- native Timer action boundary and readback foundations;
- SearchTimer domain, preview and real-backend validation;
- backend-scoped EPG and Recording caches;
- persistent metadata/Genre assignments and indexed browse queries;
- TVScraper evidence acquisition behind Suite-owned boundaries;
- lazy Recording loading and frontend module ownership;
- DOM-free Web Client API wrapper;
- backend-neutral remote actions and live overlay;
- packaging, staging and real-system acceptance;
- SuiteBridge read-only and lifecycle foundations.

These foundations may require migration or hardening, but they must be reused rather than replaced by parallel systems.

---

# Completed Prerequisite - Architecture Package

Completion evidence:

```text
ADR-0042 through ADR-0049 accepted
AND target platform architecture published
AND domain dependency map published
AND implementation dependency map published
AND phase map and roadmap coverage checks present
```

This was a documentation and contract prerequisite, not a runtime phase number.

---

# Completed Prerequisite - Phase 60.15

Phase 60.15 prepared Recording representation and artwork boundaries:

```text
technical/native fields
  != normalized Suite fields
  != provider-derived fields
```

It preserved no-provider operation, lazy loading and provider-neutral artwork references, enabling Phase 61 persistence and read models.

---

# Completed Runtime - Phase 61 Suite Metadata and Genre Platform

Status: **Completed.**

Accepted dependency path:

```text
backend-scoped Recording/EPG target identity
  -> provider and derived evidence
  -> canonical Genre assignment
  -> assignment state and conflict handling
  -> indexed query/read model
  -> Suite REST
  -> Web Client API
  -> frontend navigation/detail reuse
```

Completed slices:

```text
61.1 metadata and Genre identity/value foundations
61.2 persistent target binding and assignment schema
61.3 migrations and repository tests
61.4 canonical normalization and alias handling
61.5 provider evidence, assignment state and conflict semantics
61.6 artwork/reference integration through existing Suite boundaries
61.7 Recording and EPG Genre read models
61.8 TVScraper/plugin-backed evidence acquisition for accepted paths
61.9 bounded asynchronous refresh and invalidation
61.10 restart persistence and real-system acceptance
61.11 frontend Genre navigation and existing detail-owner reuse
```

Completion gate met:

- Suite-owned assignments and read models are authoritative for the feature;
- provider outages do not remove the ability to browse cached Recordings and Genres;
- evidence state is explicit;
- provider transport is absent from HTTP GET and frontend rendering;
- migrations and restart behavior are tested;
- accepted runtime was installed and verified on the real yaVDR system.

Closeout evidence is recorded in [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md).

---

# Completed Operational Hardening - B1-B4

Status: **Completed, non-numbered.**

Dependency result:

```text
indexed candidate and window reads
  + atomic evidence writes
  + no-op Recording synchronization
  + no-op EPG event upserts
  + throttled completed ETYPES cycles
```

This block reduces query cost, lock duration, WAL traffic and repeated VDR schedule scans without changing the public Genre contract.

---

# Next Runtime - Phase 62 Identity, RBAC and Accountability

## Prerequisites

- completed Phase 61 persistence and identity patterns;
- current BackendAccessMode/read-only enforcement retained;
- ADR-0041 identity and trust direction;
- ADR-0042 operation context and safe mutation rules;
- ADR-0048 request/correlation context;
- ADR-0049 accountability model.

## Internal order

```text
62.1 ActorIdentity and actor-type value objects
62.2 user, service-account, Agent and system-actor persistence
62.3 authentication session and credential-reference contracts
62.4 role, permission and backend/resource scope values
62.5 centralized AuthorizationDecision service
62.6 server-side policy enforcement adapters
62.7 actor, request and correlation context propagation
62.8 AccountabilityEvent catalogue and schema
62.9 append-only repository and migrations
62.10 transactional outbox for protected operations
62.11 authentication/session/authorization events
62.12 mutation, backend-access and security events
62.13 protected query, redaction and cursor model
62.14 retention, correction and audit-of-audit rules
62.15 deny-path, outage and failure-injection acceptance
```

## Critical gate

```text
central authorization service
  before production mutation authorization migration

append-only accountability persistence and outbox
  before new privileged or remote dispatch
```

## Exit gate

- different users can have different rights on the same backend;
- denial is enforced server-side;
- the second-house read-only scenario remains proven;
- every real privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch evidence failure prevents dispatch;
- Agent credentials can be represented for Phase 63.

---

# Phase 63 Backend Agent and Secure Multi-Site Runtime

## Prerequisites

- Phase 62 actor, authorization and accountability foundations;
- ADR-0039 through ADR-0043;
- ADR-0049 producer-evidence model.

## Internal order

```text
63.1 Agent identity and enrollment records
63.2 credential lifecycle, rotation and revocation
63.3 protected outbound Agent transport
63.4 protocol negotiation and compatibility tests
63.5 BackendGeneration creation and fencing
63.6 heartbeat, lease and deterministic health
63.7 capability and snapshot publication
63.8 durable command inbox and result outbox
63.9 fenced read-only operations
63.10 reconnect receipt/result reconciliation
63.11 local provider and SuiteBridge selection
63.12 protected write commands after all gates
63.13 offline buffering and sequence-gap handling
63.14 two-site failure acceptance
```

## Exit gate

- remote read-only operation works without public legacy ports;
- stale generations cannot complete commands;
- lease expiry changes health deterministically;
- reconnect deduplicates receipts, results and evidence;
- write dispatch remains closed until every gate passes.

---

# Phase 64 Timer Intent and Multi-Backend Orchestration

## Prerequisites

- Phase 62 authorization/accountability;
- Phase 63 backend lifecycle and command fencing;
- canonical event support from ADR-0045;
- durable operation and reconciliation rules.

## Internal order

```text
64.1 TimerIntent identity and persistence
64.2 intent revision and lifecycle
64.3 observed native Timer normalization
64.4 NativeTimerBinding persistence and ownership
64.5 explicit adoption and provenance
64.6 TimerAssignment identity and persistence
64.7 backend/channel/capability eligibility
64.8 deterministic scheduler decisions
64.9 operation/job binding for provisioning
64.10 native readback and verification
64.11 reconciler and drift classification
64.12 SearchTimer/epgsearch proposal-to-intent conversion
64.13 duplicate and ambiguity policy
64.14 primary and deliberate replica policy
64.15 failover and reconnect recovery
```

Blocking rules:

- persist an assignment before native dispatch;
- reconcile possible prior dispatch before failover;
- do not adopt a timer by title/time similarity alone;
- SearchTimer may not bypass central orchestration.

---

# Phase 65 Streaming Gateway

## Prerequisites

- Phase 62 media authorization and accountability;
- Phase 63 Agent transport and backend lifecycle;
- stable Recording and Channel identities;
- ADR-0046 session and route model.

## Internal order

```text
65.1 MediaResourceRef and MediaSession persistence
65.2 media authorization policy
65.3 MediaRoute and route epoch
65.4 short-lived MediaAccessGrant lifecycle
65.5 Gateway validation and connection ownership
65.6 Agent provider route protocol
65.7 ProviderStreamLease and capacity
65.8 Live pass-through acceptance
65.9 Recording range, seek and reconnect
65.10 growing Recording behavior
65.11 provider failure and route invalidation
65.12 optional remux/transcode boundaries
```

Blocking rules:

- no permanent Streamdev URL is public;
- session ID alone is not access proof;
- no public VDR or Agent provider port is required;
- playback and download permissions remain separate.

---

# Phase 66 Legacy OSD Bridge

## Prerequisites

- Phase 62 separate view/control permissions;
- Phase 63 sequencing and generation fencing;
- ADR-0047 session and controller-lease model.

## Internal order

```text
66.1 read-only OSD surface snapshot
66.2 immutable OsdFrame
66.3 ordered Agent deltas
66.4 LegacyOsdSession persistence
66.5 viewer binding and fan-out
66.6 sequence-gap detection and full resync
66.7 controller lease and expiry
66.8 allowlisted input value object
66.9 fenced local input execution
66.10 rate/deadline/reconnect protection
66.11 read-only backend control denial
66.12 physical remote coexistence tests
```

Viewing must be implemented before control; no arbitrary command tunnel is allowed.

---

# Phase 67 Public API and Client Hardening

## Prerequisites

- implemented domain resources from Phases 61 through 66;
- Phase 62 actor context;
- stable revisions, operations and errors;
- ADR-0048.

## Internal order

```text
67.1 public response/header abstraction
67.2 request and correlation ID middleware
67.3 common problem response serializer
67.4 /api/v1 root and capability discovery
67.5 ETag and If-Match
67.6 idempotency and operation exposure
67.7 cursor collections and partial-result metadata
67.8 resource-by-resource route migration
67.9 server-side legacy aliases
67.10 deprecation and successor metadata
67.11 structured Client API errors
67.12 remove speculative mutation fallback
67.13 schema and compatibility tests
```

A route enters `/api/v1` only after identity, backend scope, authorization, revision, error, collection and accountability behavior are explicit.

---

# Phase 68 Recommendation and Knowledge Graph

## Prerequisites

- stable metadata and provenance;
- actor privacy and authorization;
- stable Recording, ProgramEvent and Timer identities;
- mature public API and accountability boundaries.

## Direction

```text
68.1 graph identity and edge vocabulary
68.2 provenance for graph facts
68.3 preference and privacy boundaries
68.4 deterministic non-AI baseline
68.5 explainable ranking evidence
68.6 optional provider-neutral AI enrichment
68.7 local/offline provider support
68.8 feedback and correction model
```

No recommendation work may use unstable identities or hide provider authority.

---

## Cross-Cutting Test Order

```text
value-object tests
  -> repository and migration tests
  -> service and policy tests
  -> adapter/provider contract tests
  -> failure-injection tests
  -> controller/API tests
  -> frontend client-contract tests
  -> packaging/install tests
  -> live backend or VDR acceptance when native behavior changes
```

| Concern | Required proof |
| --- | --- |
| Identity | Stability, scope and non-interchangeability. |
| Persistence | Migration, restart and uniqueness behavior. |
| Authorization | Allow, deny, existence hiding and read-only enforcement. |
| Revision | Stale and missing preconditions rejected before dispatch. |
| Idempotency | Same-request reuse and different-request conflict. |
| Accountability | Actor, decision, outcome and protected outbox behavior. |
| Agent | Enrollment, generation, lease, reconnect and revocation. |
| Reconciliation | Unknown outcome preserved until evidence resolves it. |
| Metadata | Provider failure and provenance independence. |
| Timer | Assignment persistence, duplicate policy and readback. |
| Media | Grant expiry, route fencing, capacity and reconnect. |
| Client | Stable errors, versioning and compatibility behavior. |

---

## Maintenance Rule

Completed phases stay marked complete. New optional work must be assigned to a later phase or backlog rather than reopening old status text.
---

## Back

- [Back to Strict Roadmap](roadmap.md)
- [Back to Phase Map](phase-map.md)
- [Back to Planning Index](index.md)
- [Back to Current State](../CURRENT.md)
