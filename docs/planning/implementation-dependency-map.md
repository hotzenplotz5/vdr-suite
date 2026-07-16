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

---

## Purpose

This document translates ADR-0042 through ADR-0049 into a strict runtime implementation order.

It defines prerequisites, bounded implementation slices, exit gates and forbidden shortcuts. It does not mark a planned phase complete.

---

## Governing Sequence

```text
architecture contracts and dependency maps
  -> Phase 60.15 Recording metadata representation preparation
  -> Phase 61 Suite metadata platform
  -> Phase 62 Identity, RBAC and accountability
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

The implementation builds on:

- daemon composition and HTTP server boundary;
- SQLite repositories, services and migration foundation;
- BackendNode, BackendRegistry and backend access modes;
- backend-aware snapshots, partial refresh and change feed;
- RESTfulAPI adapter and HTTP abstractions;
- runtime logging and diagnostics separation;
- guarded Recording request, preview, validation and execution;
- native Timer action boundary;
- SearchTimer domain, preview and real-backend validation;
- backend-scoped EPG cache and queries;
- lazy Recording loading and frontend module ownership;
- DOM-free Web Client API wrapper;
- packaging, install and library boundaries;
- Suite Bridge SB.1 through SB.7 read-only foundations.

These foundations may require migration and hardening. They are not proof that the future platform contracts are already implemented.

---

# Step 1 - Architecture Package Closeout

Required completion evidence:

```text
ADR-0042 through ADR-0049 accepted
AND target architecture diagrams published
AND domain dependency map published
AND implementation dependency map published
AND roadmap, phase map, handoff and current state aligned
AND full repository CI green
```

This step changes documentation and planning truth only.

---

# Step 2 - Phase 60.15 Recording Metadata Representation Preparation

## Inputs

- current lazy Recording browser and detail UX;
- current Recording domain objects, serializers and Client API;
- ADR-0014 Recording identity direction;
- ADR-0038 provider-neutral metadata direction;
- ADR-0048 public representation rules.

## Internal order

```text
60.15a audit current Recording field ownership
60.15b separate technical, normalized and provider-derived fields
60.15c introduce a provider-neutral artwork reference shape
60.15d provide deterministic no-provider placeholders
60.15e adapt Recording list and detail presentation
60.15f preserve lazy folder loading and cache behavior
60.15g add compatibility and regression coverage
60.15h document temporary contracts for Phase 61 migration
```

## Not allowed in this phase

- direct frontend dependency on TVScraper, scraper2vdr or another provider;
- provider lookup in synchronous Recording list rendering;
- provider database treated as VDR-Suite truth;
- permanent public artwork identity based only on an external URL.

## Exit gate

- technical and enriched fields are distinguishable;
- the UI works without provider data;
- artwork references are provider neutral or explicitly temporary;
- lazy loading behavior remains covered and green.

---

# Step 3 - Phase 61 Suite Metadata Platform

## Prerequisites

- Phase 60.15 representation preparation complete;
- stable Recording identity used for assignments;
- ADR-0038 and ADR-0045 vocabulary retained.

## Internal order

```text
61.1 metadata identity and value types
61.2 metadata, provider, evidence and assignment schema
61.3 migrations and repository tests
61.4 normalization and resolver services
61.5 field provenance, confidence and conflict states
61.6 artwork asset identity, storage and delivery
61.7 Recording enrichment read model
61.8 sidecar, import and plugin-backed provider adapters
61.9 asynchronous refresh and invalidation work
61.10 migration, backup and recovery tests
61.11 frontend enrichment beyond placeholders
```

## Dependency graph

```text
MetadataEntity persistence
        +--> MetadataAssignment persistence
        +--> ProviderEvidence and field provenance
        +--> ArtworkAsset persistence and delivery
                         |
                         v
             Recording enrichment read model
                         |
                         v
                enriched presentation
```

## Exit gate

- Suite-owned entities are authoritative;
- provider outages do not break Recording browsing;
- provenance and stale or missing states are explicit;
- artwork delivery uses Suite-owned identity;
- migrations and recovery are tested.

---

# Step 4 - Phase 62 Identity, RBAC and Accountability

## Prerequisites

- Phase 61 persistence and identity patterns proven;
- ADR-0041 identity and trust direction;
- ADR-0042 operation context;
- ADR-0048 request and correlation context;
- ADR-0049 accountability model.

## Internal order

```text
62.1 ActorIdentity and actor-type value objects
62.2 user, service-account and system-actor persistence
62.3 authentication-session references
62.4 roles, permissions and resource/backend scopes
62.5 centralized AuthorizationDecision service
62.6 server-side policy enforcement adapters
62.7 AccountabilityEvent catalogue and schema version
62.8 append-only repository and migrations
62.9 transactional outbox with protected operations
62.10 request, correlation and actor context propagation
62.11 authentication, session and authorization events
62.12 role, permission and backend-access-mode events
62.13 protected queries, redaction and cursor model
62.14 retention, corrections and audit-of-audit
62.15 failure-injection and outage behavior
```

## Critical gate

```text
central authorization service
        before production mutation authorization migration

append-only accountability persistence and outbox
        before new privileged or remote dispatch
```

## Exit gate

- users can have different rights on the same backend;
- denial is enforced server-side;
- every real mutation has actor, decision and outcome evidence;
- required pre-dispatch evidence failure prevents dispatch;
- the second-house read-only scenario is proven.

---

# Step 5 - Phase 63 Backend Agent and Multi-Site Runtime

## Prerequisites

- Phase 62 actor, authorization and accountability foundations;
- ADR-0039 through ADR-0043;
- ADR-0049 producer-evidence model.

## Internal order

```text
63.1 Agent identity and enrollment records
63.2 credential lifecycle and revocation
63.3 protected outbound Agent transport
63.4 protocol negotiation and compatibility tests
63.5 BackendGeneration creation and fencing
63.6 heartbeat, lease and deterministic health state
63.7 capability publication and degradation
63.8 snapshot and change publication
63.9 durable command inbox and result outbox
63.10 fenced read-only operation
63.11 reconnect receipt and result reconciliation
63.12 local provider and plugin adapter selection
63.13 protected write commands after all gates
63.14 offline buffering and sequence-gap handling
63.15 two-site deployment and failure acceptance
```

## Dependency graph

```text
Enrollment
  -> protected transport
  -> active BackendGeneration
  -> heartbeat and lease
  -> capability and snapshot publication
  -> read-only operations
  -> durable command and result handling
  -> fenced write operations
```

## Suite Bridge integration rules

- negotiate the local contract version;
- copy native values under bounded VDR rules;
- pass no raw VDR pointer or lock across the boundary;
- perform no network, database or filesystem work in callbacks;
- report capability degradation truthfully;
- require live VDR acceptance for every native slice.

Plugin mutation remains blocked until authorization, revision, generation, idempotency, accountability and readback work end to end.

## Exit gate

- enrollment and revocation work;
- a remote read-only backend works without public legacy ports;
- stale generations cannot complete commands;
- lease expiry changes health deterministically;
- reconnect deduplicates receipts and evidence.

---

# Step 6 - Phase 64 Timer Intent and Orchestration

## Prerequisites

- Phase 63 backend lifecycle and command fencing;
- Phase 62 authorization and accountability;
- canonical event support from ADR-0045;
- ADR-0042 and ADR-0043 runtime.

## Internal order

```text
64.1 TimerIntent identity and persistence
64.2 intent revisions and lifecycle services
64.3 observed native timer normalization
64.4 NativeTimerBinding persistence and ownership state
64.5 adoption and provenance workflow
64.6 TimerAssignment identity and persistence
64.7 backend, channel and capability eligibility
64.8 deterministic scheduler decision model
64.9 operation and job binding for provisioning
64.10 native readback and binding verification
64.11 reconciler and drift classification
64.12 SearchTimer and epgsearch proposal-to-intent conversion
64.13 duplicate and ambiguous-review policy
64.14 explicit primary and replica policy
64.15 failover and reconnect recovery
```

## Blocking rules

- persist an assignment before native dispatch;
- reconcile possible prior dispatch before failover;
- do not adopt a timer from title and time similarity alone;
- do not allow SearchTimer to bypass the central scheduler.

## Exit gate

- intent survives backend-native identity changes;
- one primary assignment exists by default;
- deliberate replicas are explicit;
- scheduler decisions are reproducible and auditable;
- unknown outcomes reconcile before reassignment.

---

# Step 7 - Phase 65 Streaming Gateway

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
65.7 ProviderStreamLease and capacity tracking
65.8 Live pass-through acceptance
65.9 Recording range, seek and reconnect
65.10 growing Recording behavior
65.11 provider failure and route invalidation
65.12 optional remux boundary
65.13 optional transcode boundary
65.14 multi-site bandwidth and policy hardening
```

## Blocking rules

- do not expose permanent Streamdev URLs;
- do not treat a media session ID as access proof;
- do not require public VDR or Agent ports;
- keep playback and download permissions separate.

## Exit gate

- grants expire and revoke deterministically;
- internal routes stay hidden;
- generation and route epoch fence stale access;
- capacity exhaustion is explicit;
- Live and Recording playback work through the Agent boundary.

---

# Step 8 - Phase 66 Legacy OSD Bridge

## Prerequisites

- Phase 62 separate view and control permissions;
- Phase 63 sequencing and generation fencing;
- ADR-0047 session and controller-lease model.

## Internal order

```text
66.1 read-only OSD surface snapshot
66.2 immutable OsdFrame representation
66.3 ordered Agent delta representation
66.4 LegacyOsdSession persistence
66.5 viewer binding and fan-out
66.6 sequence-gap detection and full resync
66.7 controller-lease arbitration and expiry
66.8 allowlisted input value object
66.9 fenced local input execution
66.10 rate, deadline and reconnect protection
66.11 read-only backend control denial
66.12 physical remote coexistence tests
66.13 accountability and privacy hardening
```

## Blocking rules

- implement viewing before control;
- trust deltas only after full snapshot and sequencing exist;
- require a current controller lease before input;
- provide no arbitrary command tunnel;
- replay no input after disconnect.

## Exit gate

- multiple viewers work;
- only one Suite controller lease is active per surface and epoch;
- stale generation, surface epoch or lease epoch is rejected;
- sequence loss recovers through full resync;
- VDR callback safety is proven live.

---

# Step 9 - Phase 67 Public API and Client Hardening

## Prerequisites

- implemented domain resources from Phases 61 through 66;
- Phase 62 actor context;
- stable revisions, operations and errors for every migrated resource;
- ADR-0048.

## Internal order

```text
67.1 public response and header abstraction
67.2 request ID and accepted correlation ID middleware
67.3 common problem response serializer
67.4 /api/v1 contract root and capability discovery
67.5 ETag and If-Match support
67.6 idempotency and operation-resource exposure
67.7 cursor collections and partial-result metadata
67.8 resource-by-resource route migration
67.9 server-side legacy alias adapters
67.10 deprecation and successor metadata
67.11 structured Client API errors
67.12 remove speculative mutation fallback
67.13 schema and compatibility test suite
67.14 unsupported-client and version tests
```

A route enters `/api/v1` only after proving stable identity, explicit backend scope, authorization, capability enforcement, revision behavior, error semantics, collection rules and accountability classification.

## Exit gate

- bundled clients use canonical v1 paths;
- mutations do not use speculative route fallback;
- request IDs and structured errors survive the client wrapper;
- ETags protect mutable resources;
- legacy routes have explicit compatibility status.

---

# Step 10 - Phase 68 Recommendation and Knowledge Graph

## Prerequisites

- stable metadata and provenance;
- actor privacy and authorization;
- stable Recording, ProgramEvent and Timer identities;
- mature public API and accountability boundaries.

## Internal order

```text
68.1 graph identity and edge vocabulary
68.2 provenance for graph facts
68.3 preference and privacy boundaries
68.4 deterministic non-AI baseline
68.5 explainable ranking evidence
68.6 provider-neutral optional AI enrichment
68.7 local and offline provider support
68.8 feedback and correction model
68.9 public API and frontend surfaces
```

## Blocking rules

- no recommendations over unstable identities;
- no provider becomes hidden authority;
- no provider-specific type enters the core domain;
- no sensitive preference crosses unauthorized site scope.

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
| Identity | stability, scope and non-interchangeability |
| Persistence | migration, restart and uniqueness behavior |
| Authorization | allow, deny, existence hiding and read-only enforcement |
| Revision | stale and missing preconditions rejected before dispatch |
| Idempotency | same request reuse and different-request conflict |
| Job claim | atomic ownership, expiry and stale-worker fencing |
| Agent | enrollment, generation, lease, reconnect and revocation |
| Reconciliation | unknown outcome preserved until evidence resolves it |
| Metadata | provider failure and provenance independence |
| Timer | assignment persistence, duplicate policy and readback |
| Media | grant expiry, route fencing, range/seek and capacity |
| OSD | sequencing, resync, controller arbitration and callback safety |
| API | status, headers, schema, compatibility and deprecation |
| Accountability | append-only history, redaction, outbox and outage behavior |

---

## Deployment and Migration Gate

Before changing durable persistence or remote protocols, a phase defines:

- supported previous versions;
- migration order;
- queued payload compatibility;
- Agent and plugin contract compatibility;
- interrupted-upgrade behavior;
- backup and restore expectations;
- recovery or forward-fix path.

No migration silently reinterprets durable identity, operation, job, provenance or accountability history.

---

## PR Ownership Review

Every implementation PR answers:

```text
Which layer owns the decision?
Which stable identity is used?
Which revision, generation or sequence fences stale work?
What happens after timeout, restart or reconnect?
What proves success?
Which bounded accountability evidence is required?
Which tests prove the boundary?
```

---

## Forbidden Shortcuts

- frontend-only authorization;
- direct central database access from Agents or plugins;
- public legacy VDR transport ports;
- path, title or native list position as global identity;
- executor acknowledgement treated as verified success;
- retry after uncertain dispatch without reconciliation;
- provider schema exposed as public domain contract;
- SearchTimer acting as global scheduler;
- session identifiers used as unrestricted access proof;
- arbitrary remote command tunnels;
- runtime logs parsed as accountability history;
- accepted ADR described as completed runtime.

---

## Current Next Step

After this closeout, runtime work begins with:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

The first slice starts with a repository-backed audit of the current Recording domain, serializers, REST responses, frontend Client API and lazy-loading behavior. It does not begin with direct provider integration.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Domain Dependency Map](domain-dependency-map.md)
- [Back to Strict Roadmap](roadmap.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
