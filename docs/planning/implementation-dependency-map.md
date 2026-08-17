# VDR-Suite Implementation Dependency Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)

---

## Purpose

This map translates accepted/proposed architecture into strict **runtime dependency direction**. It defines what later work may depend on and what shortcuts remain forbidden.

It does not identify the active PR/head or authorize a numbered phase. Exact operational phase status belongs in [Current State](../CURRENT.md); strict phase sequencing belongs in the [Roadmap](roadmap.md).

## Governing sequence

```text
identity, authorization and accountability
  -> secure Backend Agent lifecycle and observation continuity
  -> durable command/result and protected-write safety
  -> Timer Intent and Multi-Backend Orchestration [completed]
  -> Streaming Gateway and Media Sessions
  -> Broadcast Companion Services: Teletext and HbbTV
  -> Legacy OSD Compatibility Bridge
  -> stable Public API and Client Compatibility Hardening
  -> Recommendation and Content Knowledge Graph
```

Cross-cutting product work such as account/backend access administration, broad Timer UI, audit/operations surfaces and client-family rollout may proceed when its own prerequisites are met. It does not silently reorder the numbered runtime sequence.

Later domains may not bypass earlier identity, authorization, accountability, lifecycle, revision, fencing or stable-domain prerequisites.

---

## Existing reusable foundations

The implementation must continue to reuse rather than duplicate:

- daemon composition and HTTP server boundaries;
- SQLite migrations, repositories and transaction ownership;
- BackendNode/BackendRegistry and backend access policy;
- backend-scoped snapshots, caches, partial refresh and change feed;
- RESTfulAPI/SVDRP/SuiteBridge adapter boundaries;
- Recordings 2 and guarded Recording workflows;
- native Timer and SearchTimer foundations;
- persistent Recording/EPG metadata, people, artwork and Genre assignments;
- manual metadata/cast assignment and local image contracts;
- query-oriented Genre/global-search repositories;
- backend-neutral RemoteAction and LiveOverlay contracts;
- modular frontend ownership through `VdrSuiteClientApi`;
- persistent actor identity, browser sessions, RBAC and accountability;
- Backend Agent enrollment/lifecycle/generation fencing;
- Agent observation ingestion, durable commands/results and explicit provider ownership;
- shared mutation-operation persistence, no-blind-retry and unknown-outcome reconciliation;
- completed TimerIntent/TimerAssignment/NativeTimerBinding orchestration and failover.

These foundations may be hardened/generalized only when a concrete domain requires it. Do not create parallel frontend-, plugin- or provider-owned systems.

---

## Dependency A — Identity, authorization and accountability

Required before protected multi-site/cross-backend effects:

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

---

## Dependency B — Backend Agent lifecycle and trustworthy observations

Required before safe remote/site-local work:

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
- observations are evidence with provenance, not hidden authority;
- repository code owns SQLite.

---

## Dependency C — Durable commands and protected-write safety

Required before a new native mutation can be trusted across process/network boundaries:

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

- timeout after possible dispatch is not proof of failure;
- retry does not create a new logical actor request;
- resource-scoped concurrency prevents conflicting protected writes;
- provider availability does not grant provider authority;
- active execution never silently switches provider;
- stale backend generation, provider epoch or job claim cannot complete current work.

---

## Dependency D — Timer Intent and Multi-Backend Orchestration

Status: completed Phase-64 foundation.

```text
TimerIntent
  -> current backend/channel/capability evidence
  -> deterministic eligible-backend decision
  -> TimerAssignment
  -> deliberate primary/replica role
  -> durable protected native operation
  -> NativeTimerBinding
  -> authoritative native Timer readback
  -> reconciliation / controlled reassignment
```

Rules retained for later work:

- SearchTimer/epgsearch may create proposals/intents but do not bypass central ownership;
- no failover while prior native dispatch is unresolved;
- an unexplained duplicate is drift, not a replica;
- broad Timer UI consumes this engine; it does not replace its state model.

---

## Dependency E — Streaming Gateway and Media Sessions

Phase 65 depends on A-D plus stable Recording/Channel identity and current Agent/provider capabilities.

```text
MediaResourceRef
  -> authenticated playback request
  -> authorization + admission
  -> MediaSession
  -> compatible selected media profile
  -> MediaRoute + routeEpoch
  -> short-lived MediaAccessGrant
  -> Gateway connection
  -> Agent provider route
  -> explicitly owned ProviderStreamLease
  -> private source
  -> bytes
```

Required rules:

- no permanent private provider URL is public;
- provider identity is not client-selectable;
- route replacement gets a new epoch and fences the old route;
- disconnect/revocation releases bounded resources;
- slow clients/media processing stay outside VDR callbacks/locks;
- transformation preference is pass-through, then remux/repackage, then transcode only when needed;
- growing Recording/range/seek capability is truthful;
- ordinary Live playback does not silently imply timeshift;
- product acceptance proves real picture/sound and cleanup.

Client execution direction:

```text
selected MediaSession profile
  -> small Suite playback abstraction
  -> platform-native/mature playback engine
```

VDR-Suite does not vendor one universal decoder/rendering core.

---

## Dependency F — Broadcast Companion Services

Phase 66 depends on A-B and on Phase-65 media semantics where HbbTV/application media uses Suite-owned resources.

Architecture is defined by accepted ADR-0054; runtime remains unauthorized until Phase 65 closes and Phase 66 is explicitly started.

### Teletext dependency direction

```text
Live Channel / broadcast service
  -> backend-local Teletext provider observation
  -> TeletextServiceRef
  -> TeletextPageRef
  -> TeletextPage / TeletextSubpage
  -> client rendering/navigation
```

Required rules:

- provider/plugin cache formats remain private;
- page identity is service/channel scoped, not global page-number identity;
- freshness/revision is explicit;
- normal Teletext browsing does not depend on LegacyOsdSession;
- client page navigation is domain input, not raw VDR remote replay.

### HbbTV dependency direction

```text
Live Channel / broadcast service
  -> AIT/DSM-CC or other proven discovery evidence
  -> BroadcastApplicationDescriptor
  -> BroadcastApplicationRef
  -> authorization
  -> BroadcastApplicationSession
  -> isolated HbbTV-capable runtime
  -> normalized session-scoped input
```

When Suite media is involved:

```text
BroadcastApplicationSession
  -> Phase-65 MediaSession / Gateway
```

Required rules:

- raw URL/JavaScript/key/plugin command channels are private implementation details;
- broadcaster application execution is isolated from Suite credentials;
- backend generation/discovery revision/session identity are fenced;
- channel change closes/invalidates stale app context;
- HbbTV runtime is not the Legacy OSD bridge.

---

## Dependency G — Legacy OSD Compatibility Bridge

Phase 67 depends on identity/authorization and backend generation/sequence fencing. It follows Broadcast Companion so structured television-domain features are not prematurely treated as opaque OSD compatibility.

```text
read-only native OSD observation
  -> immutable ordered full frame
  -> optional delta
  -> LegacyOsdSession
  -> viewer fan-out
  -> gap/full-resync semantics
  -> separate controller permission
  -> one fenced controller lease
  -> allowlisted/rate-limited input
```

Viewing precedes control. No arbitrary command tunnel is created. This subsystem is separate from LiveOverlay, MediaSession, Teletext and HbbTV application sessions.

---

## Dependency H — Stable Public API and Client Hardening

Phase 68 depends on mature implemented resources from earlier domains. It should stabilize what exists rather than freeze transitional internals prematurely.

```text
request/correlation context
  -> common problem/error model
  -> stable resource IDs and revisions
  -> conditional mutation / idempotency exposure
  -> deterministic collection/pagination semantics
  -> partial multi-backend result semantics
  -> compatibility/deprecation rules
  -> schema/client contract tests
```

Independent version domains remain separate:

```text
Public /api/v1
Agent protocol
Media Plane
Broadcast Companion provider/session schemas
Legacy OSD frame/input schemas
plugin-local contracts
```

Internal transition routes and first-party wrappers are not automatically stable public API commitments.

---

## Dependency I — Recommendation and Content Knowledge Graph

Phase 69 requires a dedicated accepted ADR and depends on mature content identity/provenance, actor privacy/preferences and stable resource semantics.

```text
stable content identities
  -> provenance-aware facts/edges
  -> privacy/preferences
  -> deterministic baseline
  -> explainable ranking
  -> optional provider-neutral AI
  -> feedback/correction
```

It must not become hidden mutation authority for Timer, metadata, Recording or access policy.

---

# Cross-cutting product dependencies

## Account/backend access administration

```text
Phase-62 identity/RBAC foundation
  -> safe administration product surface
  -> broad Timer Product UI mutation controls
```

Core policy remains server-side. Administration UI/API must not invent permissions or backend scope.

## Broad Timer Product UI

```text
Phase 62 [done]
+ Phase 64 Timer engine [done]
+ required access administration [open]
  -> intent-first Timer UI
```

The UI consumes TimerIntent/Assignment/Binding/Operation state. It never treats raw native VDR Timers as the durable product authority.

## Audit/operations product

Append-only accountability is already a foundation. Reader/filter/export/redaction/retention surfaces may be added without reopening Phase 62.

## First-party clients

Browser validates Phase-65 media first. TV/native/Kodi adapters reuse Suite semantics and platform-appropriate playback/application engines. Third-party public compatibility is formalized in Phase 68.

---

# Cross-cutting test order

Each coherent slice progresses through the smallest applicable subset of:

```text
domain/value tests
  -> repository/migration tests
  -> service/controller tests
  -> provider/Agent contract tests
  -> architecture/static guards
  -> frontend/client contract tests where relevant
  -> aggregate regression
  -> production build
  -> packaging/install validation
  -> real-system acceptance where native/media/broadcast behaviour changes
  -> Golden User Journey acceptance where product behaviour changes
  -> rollback verification
```

A slice should be reviewable but not mechanically tiny if that would create an artificial intermediate state with no coherent safety or product value.

---

# Forbidden shortcuts

- no frontend-owned authorization or private provider calls;
- no direct provider/shared database as public Suite authority;
- no new remote/native write without identity/accountability/idempotency/fencing/readback semantics appropriate to the resource;
- no blind retry after unknown mutation outcome;
- no Timer failover while prior dispatch is unresolved;
- no silent provider fallback based on reachability alone;
- no public permanent private-provider URL;
- no HbbTV public arbitrary URL/JavaScript/plugin command tunnel;
- no Teletext product contract defined as OSD pixels/cache files when structured page data can be modeled;
- no Legacy OSD shortcut for normal domain-first EPG/Timer/Recording/Streaming/Teletext/HbbTV surfaces;
- no accepted/proposed ADR presented as completed runtime without evidence;
- no historical slice document treated as current implementation authorization;
- no active PR/head/CI checkpoint copied into this dependency map.

---

## Status rule

This map answers **what must precede what**. It does not answer **what is active right now**.

For current completed/active/next phase position use [Current State](../CURRENT.md). For strict numbered order and completion gates use the [Roadmap](roadmap.md).

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
