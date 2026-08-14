# VDR-Suite Implementation Dependency Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Domain Dependency Map](domain-dependency-map.md)

---

## Purpose

This map translates accepted architecture into a strict **runtime dependency order**. It defines what later work may depend on and what shortcuts are forbidden.

It intentionally does not say which PR is active, which branch is current or which exact slice comes next. Those volatile facts belong only in [Current State](../CURRENT.md).

## Governing sequence

```text
identity, authorization and accountability
  -> secure Backend Agent lifecycle and observation continuity
  -> durable command/result and protected-write safety
  -> Timer Intent and Multi-Backend Orchestration
  -> Streaming Gateway and Media Sessions
  -> Legacy OSD Compatibility Bridge
  -> stable Public API and Client Compatibility Hardening
  -> Recommendation and Content Knowledge Graph
```

The numbered phase mapping for these dependencies is maintained by the [Strict Roadmap](roadmap.md) and [Phase Map](phase-map.md).

Later phases may not bypass earlier identity, authorization, accountability, lifecycle, revision, fencing or stable-domain prerequisites.

## Existing reusable foundations

The implementation must continue to reuse rather than duplicate:

- daemon composition and HTTP server boundaries;
- SQLite migrations, repositories and transaction ownership;
- BackendNode/BackendRegistry and backend access policy;
- backend-scoped snapshots, caches, partial refresh and change feed;
- RESTfulAPI/SVDRP/SuiteBridge adapter boundaries;
- Recordings 2 and guarded Recording action workflows;
- native Timer and SearchTimer foundations;
- persistent Recording/EPG metadata, people, artwork and Genre assignments;
- query-oriented Genre/global-search repositories;
- backend-neutral RemoteAction and LiveOverlay contracts;
- modular frontend ownership through `VdrSuiteClientApi`;
- persistent actor identity, browser sessions, RBAC and accountability;
- Backend Agent enrollment/lifecycle/generation fencing;
- Agent observation ingestion, durable commands/results and explicit provider ownership;
- protected-write safety and unknown-outcome reconciliation.

These foundations may be hardened or generalized only when a concrete domain requires it; they must not be replaced by parallel frontend-, plugin- or provider-owned systems.

## Dependency A — Identity, authorization and accountability

Required before protected multi-site or cross-backend effects:

```text
persistent ActorIdentity
  -> authenticated request/session context
  -> exact backend/resource scope
  -> AuthorizationDecision
  -> required pre-dispatch accountability evidence
  -> classified protected outcome evidence
```

Critical rules:

- authentication establishes identity, not permission;
- frontend visibility is not authorization;
- read-only/backend policy remains independent from actor permission;
- secret material is not copied into durable accountability;
- protected dispatch fails closed when required authorization/accountability evidence cannot be persisted.

## Dependency B — Backend Agent lifecycle and trustworthy observations

Required before safe remote/site-local execution:

```text
Agent enrollment and technical identity
  -> credential lifecycle
  -> backend generation + Agent instance fencing
  -> heartbeat/lease and health
  -> truthful capabilities
  -> complete observation baseline
  -> exact-next changes / replay / gap handling
  -> resync-required when continuity is lost
```

Critical rules:

- backend generation, Agent instance, snapshot generation, producer sequence and resource revision are distinct;
- stale generations/instances cannot advance current state;
- observations are evidence with provenance, not hidden provider authority;
- repository code owns SQLite.

## Dependency C — Durable commands and protected-write safety

Required before a new native mutation may be trusted across process/network boundaries:

```text
durable Operation
  -> idempotency scope
  -> durable Job/Attempt or equivalent execution record
  -> recorded dispatch boundary
  -> Agent command/receipt/result lifecycle
  -> backend/provider/resource fences
  -> native execution
  -> authoritative readback
  -> verified success / verified no-effect / outcome unknown
  -> reconciliation before unsafe retry
```

Critical rules:

- a timeout after possible dispatch is not proof of failure;
- a retry does not create a new logical actor request;
- resource-scoped leases/concurrency keys prevent conflicting protected writes;
- provider availability does not grant provider authority;
- active execution never silently switches provider;
- stale backend generation, provider epoch or job claim cannot complete current work.

## Dependency D — Timer Intent and Multi-Backend Orchestration

The Timer engine depends on A-C and the relevant current EPG/channel evidence.

```text
TimerIntent
  -> intent revision/lifecycle
  -> current backend/channel/capability evidence
  -> deterministic eligible-backend decision
  -> TimerAssignment
  -> deliberate primary/replica role
  -> durable protected native operation
  -> NativeTimerBinding
  -> authoritative native Timer readback
  -> reconciliation / drift handling
```

Automation sources such as SearchTimer/epgsearch may create proposals/intents, but they do not bypass central authorization, assignment ownership or reconciliation.

No failover/reassignment is allowed while a prior native mutation may have succeeded and remains outcome-unknown.

A broad polished Timer UI is not automatically an engine-completion prerequisite; UI product gates are defined separately from the reliable orchestration engine gate.

## Dependency E — Streaming Gateway and Media Sessions

Media runtime depends on the established identity/trust/Agent boundaries plus stable media resource identity.

```text
MediaResourceRef
  -> authenticated playback request
  -> authorization and admission
  -> MediaSession
  -> selected compatible media profile
  -> MediaRoute + routeEpoch
  -> short-lived MediaAccessGrant
  -> Gateway connection ownership
  -> Agent provider route
  -> explicitly owned ProviderStreamLease
  -> private VDR/StreamProvider source
  -> bytes
```

Required rules:

- no permanent Streamdev/private provider URL becomes public;
- `mediaSessionId` is an identifier, not a bearer credential;
- provider identity is not client-selectable;
- route replacement receives a new epoch and fences the old route;
- disconnect/revocation releases bounded provider resources;
- slow clients and media processing stay outside VDR callbacks/locks;
- transformation preference is pass-through, then remux/repackage, then transcode only when materially required;
- growing Recording/range/seek capability is reported truthfully;
- ordinary live playback does not silently imply timeshift.

Product acceptance must include real picture/sound and deterministic cleanup, not only fake-provider CI.

## Dependency F — Legacy OSD Compatibility Bridge

Legacy OSD compatibility depends on identity/authorization and backend generation/sequence fencing.

```text
read-only OSD observation
  -> immutable ordered frame/delta
  -> LegacyOsdSession
  -> viewer fan-out
  -> gap/full-resync semantics
  -> separate controller permission
  -> one fenced controller lease
  -> allowlisted/rate-limited input
```

Viewing precedes control. No arbitrary command tunnel is created. This subsystem is separate from LiveOverlay and media streaming.

## Dependency G — Stable Public API and Client Hardening

Stable independent-client contracts depend on implemented domain resources and mature identity/revision/error semantics.

```text
request/correlation context
  -> common problem/error model
  -> stable resource IDs and revisions
  -> conditional mutation / idempotency exposure
  -> cursor and partial-result semantics
  -> compatibility/deprecation rules
  -> schema/client contract tests
```

Internal transition routes and first-party wrappers are not automatically stable public API commitments.

## Dependency H — Recommendation and Content Knowledge Graph

Recommendation/knowledge-graph work depends on mature content identity/provenance, actor privacy/preferences and stable resource semantics.

```text
stable content identities
  -> provenance-aware facts/edges
  -> privacy/preferences
  -> deterministic baseline
  -> explainable ranking
  -> optional provider-neutral AI
  -> feedback/correction
```

It must not become a hidden authority that mutates Timer, metadata or access policy without the owning domain contract.

## Cross-cutting test order

Each coherent slice should progress through the smallest applicable set of:

```text
domain/value tests
  -> repository/migration tests
  -> service/controller tests
  -> architecture/static guards
  -> frontend/client contract tests where relevant
  -> aggregate regression
  -> build/package/install validation
  -> real-system acceptance where native behaviour changes
  -> Golden User Journey acceptance where product behaviour changes
```

A slice should be small enough to review safely, but not so small that it creates an artificial intermediate state with no independently useful or coherent contract.

## Forbidden shortcuts

- no frontend-owned authorization or private provider calls;
- no direct provider/shared database as public Suite authority;
- no new remote/native write without identity/accountability/idempotency/fencing/readback semantics appropriate to the resource;
- no blind retry after an unknown mutation outcome;
- no Timer failover while prior dispatch is unresolved;
- no silent provider fallback based on reachability/priority alone;
- no public permanent private-provider URL;
- no accepted ADR presented as completed runtime without evidence;
- no historical slice document treated as current implementation authorization;
- no active PR/head/CI checkpoint copied into this dependency map.

## Status rule

This map answers **what must precede what**. It does not answer **what is active right now**.

For exact current completed/active/next phase position and authorized implementation checkpoint, use [Current State](../CURRENT.md).

## Related documents

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)

## Back

- [Back to Planning Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
