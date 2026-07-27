# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order. Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md); detailed prerequisites belong in the [Implementation Dependency Map](implementation-dependency-map.md).

> Work is read from top to bottom. Later phases may not bypass identity, authorization, accountability, lifecycle fencing or stable-domain prerequisites by moving policy into a frontend, plugin or provider.

## Current verified position

Baseline reconciled on 2026-07-27 against `origin/main` commit `cb77ff66e11dca7db2eafa36525762dcde35102d`.

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Completed prerequisites and runtime

### Phase 60.15 — Recording metadata and artwork preparation

Status: **Completed.**

Separated technical/native, normalized Suite and provider-derived Recording fields; established provider-neutral artwork references; preserved lazy loading and no-provider fallback.

### Phase 61 — Suite Metadata and Genre Platform

Status: **Completed.**

Delivered persistent backend-scoped Recording/EPG target bindings, people relations, provider/derived evidence, canonical Genre assignments, explicit assignment states, query-only indexed browse paths, EPG Film/Serie/Dokumentation/Sport hierarchy and frontend integration through existing Recordings 2 and EPG detail owners.

Completion evidence:

- PR #100 merged;
- focused, regression, architecture, build/install and real-system acceptance completed;
- restart persistence, backend isolation, provider-failure isolation and navigation verified;
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md).

Completion boundary: optional providers, broader diagnostics and recommendations do not reopen Phase 61.

### Post-Phase-61 Performance Hardening (B1-B4)

Status: **Completed, non-numbered.**

PRs #102 through #108 completed EPG candidate fast paths, architecture-contract correction, atomic evidence writes, Recording Genre no-op synchronization, integer EPG window indexing, unchanged-event upsert suppression and completed ETYPES-cycle throttling.

### Post-Phase-61 platform runtime features

Status: **Completed, non-numbered.**

- PR #110: current mobile VDR Remote pressed-state and duplicate-dispatch behaviour.
- PR #111: backend-scoped global search over persisted Recording/EPG titles, subtitles and people.
- PR #115: current configurable photorealistic 360×1220 PNG remote, retained 35 hotspots, help/assignment view and mobile mappings.

These are documented in [Post-Phase-61 Platform Runtime Closeout](../development/post-phase-61-platform-runtime-closeout.md) and the current state documents. No new phase number is invented for them.

# Strict execution order

## Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Next.**

Goal: replace broad backend access hints with production-grade actor identity, scoped server-side authorization and append-only accountability.

Required order:

1. `ActorIdentity` and actor types for user, service, Agent and system actors;
2. actor/session/credential-reference persistence;
3. role, permission and backend/resource scope models;
4. centralized `AuthorizationDecision` service;
5. server-side policy enforcement preserving current read-only behaviour;
6. actor, request and correlation context propagation;
7. append-only `AccountabilityEvent` schema and repository;
8. transactional outbox for protected operations;
9. authentication, authorization, mutation and security event catalogue;
10. protected audit queries, redaction, retention and audit-of-audit;
11. deny-path, outage and failure-injection acceptance.

Exit criteria:

- different actors can hold different rights on the same backend;
- denial is enforced server-side;
- the second-house/read-only scenario remains proven;
- every privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch accountability failure prevents dispatch;
- Agent identities can be represented for Phase 63.

Forbidden shortcuts:

- no frontend-owned role decision;
- no ordinary log parsing as the accountability database;
- no new remote privileged dispatch before authorization/accountability gates exist.

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Planned after Phase 62.**

Scope:

- Agent enrollment, device identity and credential lifecycle;
- protected outbound transport and protocol negotiation;
- backend generation, heartbeat, lease and deterministic health;
- capability, snapshot and change publication;
- durable command inbox/result outbox;
- fenced read-only operations before writes;
- offline, reconnecting and degraded states;
- local provider and SuiteBridge selection;
- no public exposure of private VDR/plugin ports.

Exit criteria include stale-generation rejection, deterministic lease expiry, reconnect deduplication and closed write dispatch until all mutation gates pass.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Planned after Phase 63.**

Scope:

- durable `TimerIntent`, `TimerAssignment` and `NativeTimerBinding`;
- explicit adoption/provenance for externally created timers;
- deterministic scheduler and reconciler;
- backend capability, health, channel and event eligibility;
- duplicate, ambiguity, primary and deliberate-replica policies;
- operation/job binding, native readback and drift classification;
- SearchTimer/epgsearch proposals producing intents instead of bypassing orchestration;
- reassignment and uncertain-dispatch recovery.

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Planned after Phase 64.**

Scope:

- `MediaResourceRef`, `MediaSession`, `MediaRoute` and route epoch;
- media authorization and short-lived grants;
- Gateway-owned connections and Agent provider routes;
- provider capacity leases;
- Live pass-through and Recording range/seek/reconnect;
- growing-file behaviour;
- Streamdev as a private provider;
- optional remux/transcode only after pass-through is proven.

Clients must never receive permanent private provider URLs.

## Phase 66 — Legacy OSD Compatibility Bridge

Status: **Planned after Phase 65.**

Scope:

- read-only OSD snapshots, immutable frames and ordered deltas;
- viewer sessions and full resynchronization;
- one fenced controller lease;
- allowlisted, rate-limited input;
- read-only backend control denial;
- coexistence with the physical remote;
- no arbitrary command tunnel.

The implemented RemoteAction/LiveOverlay path does not mean this bridge already exists.

## Phase 67 — Public API and Client Compatibility Hardening

Status: **Planned after Phase 66.**

Scope:

- request and correlation IDs;
- Problem Details-compatible errors;
- `/api/v1` root and capability discovery;
- ETag / `If-Match` and revision exposure;
- durable operation and idempotency exposure;
- cursor pagination and partial-result semantics;
- resource-by-resource route migration;
- server-side legacy aliases and deprecation metadata;
- structured Client API errors;
- schema and compatibility tests.

## Phase 68 — Recommendation and Content Knowledge Graph

Status: **Later vision.**

Prerequisites include stable metadata/provenance, actor privacy, stable Recording/ProgramEvent/Timer identities, mature accountability and public API contracts.

Scope direction:

- graph identity and edge vocabulary;
- provenance for graph facts;
- preference/privacy boundaries;
- deterministic non-AI baseline;
- explainable ranking evidence;
- optional provider-neutral AI enrichment;
- local/offline provider support;
- feedback and correction model.

No recommendation work may hide provider authority or use unstable identities.

# Cross-cutting Android and client feasibility work

Status: **Architecture and planning study; not a numbered runtime phase.**

The [Android, Android TV and Client API Feasibility Study](../architecture/android-client-api-feasibility-study.md) and [Client Capability, API Candidate and Gap Matrix](client-capability-api-gap-matrix.md) map independent clients onto the strict phases above.

Permitted before Phase 67 without changing execution order:

- maintain architecture, capability and API-candidate matrices;
- extract design tokens, stable icons and client-neutral test fixtures;
- prototype Kotlin/Compose phone, tablet and TV navigation against fakes;
- test Android Media3 with non-production test media;
- build a strictly read-only compatibility-adapter PoC for current Suite routes;
- improve the browser toward an intentional PWA under a separate cache/security design.

Binding gates:

- production login, permissions and privileged client actions depend on Phase 62;
- secure remote-site access depends on Phase 63;
- product Timer automation depends on Phase 64;
- product Live TV and Recording playback depend on Phase 65;
- Legacy OSD depends on Phase 66;
- a stable independent-client API and publishable Android SDK depend on Phase 67.

The current `LiveOverlay` and SSE update path are structured state, not video. A PoC must not publish direct Streamdev, VDR, Agent or filesystem URLs and must not freeze unversioned aliases as the public contract.

# Cross-cutting completion gates

- **Identity gate:** stable Suite identity and explicit backend-native binding where applicable.
- **Provider gate:** provider data carries provenance/state and never becomes hidden authority.
- **Mutation gate:** authorization, revision, idempotency, durable dispatch evidence, verification and accountability.
- **Native boundary gate:** no raw VDR pointer or lock crosses into asynchronous/network/database work.
- **Client gate:** clients consume Suite-owned contracts, never private plugin/provider details.
- **Acceptance gate:** focused tests, regressions, build/package validation and real-system acceptance where native behaviour changes.

## Related documents

- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Android, Android TV and Client API Feasibility Study](../architecture/android-client-api-feasibility-study.md)
- [Client Capability, API Candidate and Gap Matrix](client-capability-api-gap-matrix.md)
- [Completed Phases](../development/completed-phases.md)
