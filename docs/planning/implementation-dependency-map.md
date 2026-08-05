# VDR-Suite Implementation Dependency Map

## Purpose

This map translates accepted ADRs and implemented foundations into a strict runtime dependency order. It distinguishes completed prerequisites from future work and forbids shortcuts that would move policy into clients, plugins or providers.

## Governing sequence

```text
Completed architecture contracts and dependency maps
  -> Completed Phase 60.15 Recording metadata preparation
  -> Completed Phase 61 Suite Metadata and Genre Platform
  -> Completed Post-Phase 61 Performance Hardening (B1-B4)
  -> Completed Remote/Live Overlay hardening (#110)
  -> Completed Backend-scoped Global Search (#111)
  -> Completed Phase 62 Identity, RBAC and Accountability
  -> Active Phase 63 Backend Agent and Secure Multi-Site Runtime
  -> Phase 64 Timer Intent and Orchestration
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD Bridge
  -> Phase 67 Public API and Client Hardening
  -> Phase 68 Recommendation and Knowledge Graph
```

Later phases may not bypass earlier identity, authorization, accountability, lifecycle, revision or stable-domain prerequisites.

## Existing foundations to reuse

- daemon composition and HTTP server boundary;
- SQLite migrations, repositories and transaction boundaries;
- BackendNode/BackendRegistry and backend access modes;
- backend-scoped snapshots, caches, partial refresh and change feed;
- RESTfulAPI and SVDRP adapter abstractions;
- Recordings 2 and guarded Recording action workflow;
- native Timer and SearchTimer foundations;
- persistent Recording/EPG metadata, people, artwork and Genre assignments;
- query-only Genre and global-search repositories;
- TVScraper/SuiteBridge evidence acquisition behind Suite boundaries;
- backend-neutral RemoteAction and LiveOverlay contracts;
- modular frontend ownership through `VdrSuiteClientApi`;
- packaging, staging and real-system acceptance;
- SuiteBridge read-only/capability/lifecycle foundations.

These foundations may be migrated or hardened, but must not be replaced with parallel frontend- or provider-owned systems.

## Completed runtime dependency — Phase 61

```text
backend-scoped metadata target identity
  -> provider/native/derived evidence
  -> canonical Genre assignment and state
  -> indexed query-only read model
  -> Suite REST
  -> VdrSuiteClientApi
  -> Genre navigation
  -> existing Recordings 2 / EPG detail owners
```

Completion gates met:

- Suite persistence/read models own public behaviour;
- provider outages do not remove cached browse capability;
- evidence state/provenance is explicit;
- normal GETs remain provider-free;
- migrations/restart persistence/backend isolation are tested;
- real-system acceptance completed.

## Completed operational dependency — B1-B4

```text
faster bounded candidate/window reads
  + atomic evidence writes
  + unchanged Recording synchronization no-op
  + unchanged EPG upsert no-op
  + completed ETYPES cycle throttling
```

This reduces query cost, lock duration, WAL traffic and repeated native scans without changing the public Phase 61 contract.

## Completed cross-cutting read/control features

### Remote and Live Overlay (#110)

```text
backend capability/read-only state
  -> allowlisted RemoteAction / LiveOverlay service
  -> VdrSuiteClientApi
  -> isolated button pressed-state
  -> one in-flight dispatch guard
```

This does not implement streaming or a legacy OSD bridge.

### Backend-scoped Global Search (#111)

```text
persisted Recording/EPG titles, subtitles and people
  -> query-only GlobalSearchRepository
  -> service/controller/runtime
  -> VdrSuiteClientApi
  -> grouped results
  -> existing Recordings 2 / EPG detail owners
```

The first slice deliberately searches one selected backend. Any later aggregator must authorize each backend independently and merge bounded pages without sharing provider state.

# Future implementation order

## Phase 62 — Identity, RBAC and Accountability

Prerequisites:

- current server-enforced read-only policy;
- ADR-0041 identity/trust direction;
- ADR-0042 mutation context;
- ADR-0048 request/correlation context;
- ADR-0049 accountability model;
- current persistent identity/repository patterns.

Internal order:

```text
ActorIdentity and actor types
  -> actor/session/credential persistence
  -> roles, permissions and backend/resource scopes
  -> centralized AuthorizationDecision
  -> server-side enforcement adapters
  -> actor/request/correlation context
  -> AccountabilityEvent catalogue and schema
  -> append-only repository
  -> transactional outbox
  -> protected query/redaction/retention
  -> deny/outage/failure-injection acceptance
```

Critical gate:

```text
central authorization before privileged policy migration
append-only accountability/outbox before new remote dispatch
```

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: active through bounded Slice 1 in Draft PR #137. Prerequisites are completed Phase 62, ADR-0039 through ADR-0043 and the existing accountability producer.

```text
Slice 1 active:
Agent identity/enrollment
  -> credential rotation/revocation and local recovery
  -> protected outbound transport
  -> exact protocol compatibility
  -> backend generation
  -> heartbeat/lease and derived health
  -> bounded read-only capabilities
  -> reconnect reconciliation

Later Phase-63 slices, not implemented here:
snapshot/change publication
  -> durable command inbox/result outbox
  -> fenced native operations
  -> local provider/SuiteBridge selection
  -> protected writes only after all gates
```

[Binding Slice-1 contract](../development/phase-63-backend-agent-foundation.md)

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Prerequisites: Phase 62 authorization/accountability; Phase 63 lifecycle/fencing; canonical event support; durable operations.

```text
TimerIntent
  -> intent revision/lifecycle
  -> observed native Timer normalization
  -> NativeTimerBinding/adoption/provenance
  -> TimerAssignment
  -> eligibility and deterministic scheduling
  -> operation/job binding
  -> native readback and reconciliation
  -> epgsearch/SearchTimer proposal-to-intent conversion
  -> duplicate/ambiguity/replica/failover policy
```

No provider or SearchTimer path may bypass central orchestration.

## Phase 65 — Streaming Gateway

Prerequisites: Phase 62 media authorization/accountability; Phase 63 Agent lifecycle; stable media identities; ADR-0046.

```text
MediaResourceRef / MediaSession
  -> authorization
  -> MediaRoute and route epoch
  -> short-lived access grants
  -> Gateway connection ownership
  -> Agent provider route and capacity lease
  -> Live pass-through
  -> Recording range/seek/reconnect/growing file
  -> provider failure invalidation
  -> optional remux/transcode
```

No permanent Streamdev or private provider URL becomes public.

## Phase 66 — Legacy OSD Bridge

Prerequisites: Phase 62 view/control permissions; Phase 63 generation/sequence fencing; ADR-0047.

```text
read-only OSD snapshot
  -> immutable frames and ordered deltas
  -> viewer session/fan-out
  -> sequence-gap full resync
  -> controller lease
  -> allowlisted/rate-limited input
  -> read-only backend denial
  -> physical remote coexistence
```

Viewing precedes control; no arbitrary command tunnel.

## Phase 67 — Public API and Client Hardening

Prerequisites: implemented domain resources; actor context; stable revisions/operations/errors; ADR-0048.

```text
request/correlation middleware
  -> common problem responses
  -> /api/v1 root/capabilities
  -> ETag / If-Match
  -> idempotency/operation exposure
  -> cursor/partial-result semantics
  -> resource migration
  -> legacy aliases/deprecation metadata
  -> structured Client API errors
  -> schema/compatibility tests
```

## Phase 68 — Recommendation and Knowledge Graph

Prerequisites: stable metadata/provenance, actor privacy, stable Recording/ProgramEvent/Timer identities, mature accountability and public API.

```text
graph identity/edges
  -> fact provenance
  -> privacy/preferences
  -> deterministic baseline
  -> explainable ranking
  -> optional provider-neutral AI
  -> local/offline providers
  -> feedback/correction
```

## Cross-cutting test order

Each slice must progress through:

```text
value/domain tests
  -> repository/migration tests
  -> service/controller tests
  -> architecture guards
  -> frontend contract tests where relevant
  -> aggregate regressions
  -> build/package/install validation
  -> real-system acceptance where native behaviour changes
```

## Forbidden shortcuts

- no frontend-owned authorization or provider calls;
- no direct provider/shared database as public Suite authority;
- no new remote write before identity/accountability/revision/idempotency/fencing;
- no Timer failover before unresolved prior dispatch is reconciled;
- no public private-provider URL;
- no accepted ADR presented as completed runtime without evidence.

## Related documents

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Phase 61 Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Closeout](../development/post-phase-61-platform-runtime-closeout.md)