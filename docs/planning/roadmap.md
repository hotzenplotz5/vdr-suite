# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order. Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md); detailed prerequisites belong in the [Implementation Dependency Map](implementation-dependency-map.md).

> Work is read from top to bottom. Later phases may not bypass identity, authorization, accountability, lifecycle fencing or stable-domain prerequisites by moving policy into a frontend, plugin or provider.

## Current verified position

Baseline reconciled on 2026-07-27 against `origin/main` commit `cb77ff66e11dca7db2eafa36525762dcde35102d`, the merge of PR #115.

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current Phase 62 state:
Active; Slice 1 is real-runtime validated and the persistence/revocation foundation of Slice 2 is implemented on Draft PR #117. Phase 62 remains incomplete.
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
- PR #115: current 360×1220 PNG Remote, help/navigation integration and guarded REC workflow.

These are completed prerequisites. No new phase number is invented for them.

# Strict execution order

## Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Active; incomplete.**

Goal: replace broad backend access hints with production-grade actor identity, scoped server-side authorization and append-only accountability.

### Implemented Slice 1

- canonical actor, device, session and request security context values;
- centralized exact/wildcard permission and backend-scope decisions;
- explicit legacy local-browser compatibility mode;
- enforced mode with anonymous GET and fail-closed unmigrated POST handling;
- server-side authorization for `POST /api/vdr/remote/actions` using `remote.control@backend`;
- append-only pre-dispatch allow/deny accountability;
- stable credential-safe security errors and request/correlation IDs;
- focused unit, repository, HTTP-gate and architecture tests;
- real yaVDR validation for anonymous denial, invalid-credential denial and authenticated Browser Remote dispatch.

### Implemented Slice 2 persistence and revocation foundation

- additive actor, device, session and credential metadata repositories;
- actor/device/session/credential ownership bindings;
- request-time persistent identity resolution before authorization;
- persisted expiry and revocation enforcement;
- restart-safe compatibility bootstrap that does not reactivate revoked records;
- credential identifiers and lifecycle metadata without secret persistence;
- explicit credential expiry/revocation errors and focused repository/HTTP tests.

This foundation does not complete Slice 2: secure per-user/service issuance and verification, browser cookie/CSRF, native-token lifecycle, logout/recovery/rotation and protected lifecycle administration remain open.

Evidence:

- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](../development/phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](../development/phase-62-security-identity-foundation-slice-2.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)

### Remaining required order

1. complete secure user/service credential issuance and verification, browser/native session lifecycle and protected lifecycle administration;
2. persist roles, permissions and backend/resource scopes;
3. migrate all mutations and sensitive reads to centralized authorization;
4. preserve and prove backend read-only behaviour under actor permissions;
5. complete actor, device, credential, session, request, correlation and operation context propagation;
6. extend append-only accountability to authentication, mutation completion and security lifecycle events;
7. add transactional outbox delivery for protected operations;
8. complete revision and `If-Match` rules per mutable resource;
9. add durable idempotency-key and operation replay records;
10. add protected audit queries, redaction, retention and audit-of-audit;
11. complete deny-path, outage, failure-injection, full-suite and real-runtime acceptance.

Exit criteria:

- different actors can hold different rights on the same backend;
- denial is enforced server-side for every protected route;
- the second-house/read-only scenario remains proven;
- every privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch accountability failure prevents dispatch;
- revision and idempotency contracts are enforced where required;
- Agent identities can be represented for Phase 63.

Forbidden shortcuts:

- no frontend-owned role decision;
- no ordinary log parsing as the accountability database;
- no compatibility-mode claim as final authentication;
- no plaintext, reversible or submitted credential persistence in identity tables;
- no new remote privileged dispatch before authorization/accountability gates exist;
- no Phase 63-67 runtime declared through Phase 62 interface preparation.

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
- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](../development/completed-phases.md)
- [Post-Phase-61 Platform Runtime Closeout](../development/post-phase-61-platform-runtime-closeout.md)
