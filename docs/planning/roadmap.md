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
Active; Slice 1 is real-runtime validated. Slice 2 lifecycle persistence and managed Basic increments are real-runtime accepted. Browser-session verification, atomic issuance and isolated HTTP issue/logout routes are implemented and CI validated on Draft PR #117. Ordinary application-route cookie authentication and business-mutation CSRF remain open. Phase 62 is incomplete.
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

- canonical actor, device, session, credential and request security context values;
- centralized exact/wildcard permission and backend-scope decisions;
- explicit legacy local-browser compatibility mode;
- enforced mode with anonymous GET and fail-closed unmigrated POST handling;
- server-side authorization for `POST /api/vdr/remote/actions` using `remote.control@backend`;
- append-only pre-dispatch allow/deny accountability;
- stable credential-safe security errors and request/correlation IDs;
- focused unit, repository, HTTP-gate and architecture tests;
- real yaVDR validation for anonymous denial, invalid-credential denial and authenticated Browser Remote dispatch.

### Implemented Slice 2 lifecycle foundation

- additive actor, device, session and credential metadata repositories;
- actor/device/session/credential ownership bindings;
- request-time persistent identity resolution before authorization;
- persisted expiry and revocation enforcement;
- restart-safe compatibility bootstrap that does not reactivate revoked records;
- explicit credential expiry/revocation errors;
- real yaVDR revoke/restore acceptance without daemon restart.

### Implemented Slice 2 managed verifier increment

- optional separate managed actor/device/session/credential provisioning;
- dedicated login-to-credential verifier repository;
- one-way yescrypt or SHA-512 crypt hash persistence;
- strict bounded Basic parsing and thread-safe `crypt_r` verification;
- constant-time verifier comparison;
- no managed identity or permission enabled by default;
- startup rejection for partial configuration, unsupported hash or conflicting persisted metadata;
- managed access only to authenticated reads and explicitly migrated routes;
- fail-closed denial of managed access to legacy unmigrated POST routes;
- focused provisioning, verifier, wrong-password, revocation and route-boundary tests;
- real yaVDR acceptance for valid GET, wrong-password 401, unmigrated Timer 503, migrated Remote 200 and actor/device/session accountability.

### Implemented Slice 2 browser-session verifier foundation

- additive `security_browser_session_credentials` repository;
- browser token ID bound to actor, device, session, browser credential and issuing credential;
- separate one-way modular hashes for the session and CSRF secrets;
- strict bounded `vdr_suite_session` cookie parsing with duplicate target-cookie rejection;
- independent `X-CSRF-Token` verification;
- persistent active, expiry and revocation semantics;
- positive, wrong-secret, unknown-token, malformed-token, duplicate-cookie, expired, revoked and CSRF-negative tests;
- architecture guards forbidding raw cookie/session/CSRF persistence.

### Implemented Slice 2 atomic browser-session issuance

- Linux `getrandom(2)` with full-read and `EINTR` handling;
- independent 128-bit token/session/credential IDs;
- independent 256-bit session and CSRF secrets;
- separate SHA-512 crypt `rounds=10000` verifier hashes;
- bounded 5-minute to 24-hour lifetime with 8-hour default;
- actor/device/issuing-credential revalidation inside `BEGIN IMMEDIATE`;
- atomic session, browser credential and verifier creation;
- rollback on validation, collision, repository or commit failure;
- move-only one-time result and explicit secret-buffer wiping.

### Implemented Slice 2 isolated HTTP lifecycle

- exact `POST /api/security/browser-sessions` Basic-to-browser credential exchange;
- no plaintext-password JSON endpoint;
- successful response contains only CSRF token, expiry and request ID;
- hardened host-only cookie with `Path=/`, `Max-Age=28800`, `HttpOnly`, `Secure` and `SameSite=Strict`;
- exact `POST /api/security/browser-sessions/logout` cookie-plus-CSRF route;
- Basic does not substitute for browser-cookie logout;
- missing/wrong CSRF fails before lifecycle mutation;
- verifier, canonical session and browser credential revoke atomically;
- expired hardened cookie on success;
- append-only pre-dispatch issue/revoke/authentication/CSRF decisions;
- tests prove browser cookies cannot authenticate ordinary application GET or POST routes.

This HTTP lifecycle increment is CI validated but still requires installation and real-yaVDR/HTTPS acceptance. It intentionally does not enable cookie authentication for Remote, Timer, Recording or other application routes.

Evidence:

- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](../development/phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](../development/phase-62-security-identity-foundation-slice-2.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)

### Remaining required order

1. install and real-runtime validate browser-session issue/logout plus HTTPS proxy cookie behaviour;
2. define ordinary application-route authentication precedence and connect browser contexts through `PersistentIdentityResolver` and centralized authorization;
3. load browser permissions/backend scopes without inheriting legacy compatibility grants;
4. classify every POST and enforce/audit browser CSRF before each applicable business mutation dispatch;
5. add frontend login/logout and memory-only CSRF handling without durable client secret storage;
6. complete issuance/revocation outcome accountability and transactional coupling/outbox;
7. complete managed password generation/change, native/service credential lifecycle, refresh/idle expiry/cleanup/recovery and protected lifecycle administration;
8. persist roles, permissions and backend/resource scopes;
9. migrate all mutations and sensitive reads to centralized authorization and explicit CSRF classification;
10. preserve and prove backend read-only behaviour under actor permissions;
11. complete actor, device, credential, session, request, correlation and operation context propagation;
12. extend append-only accountability to authentication, session, CSRF, mutation completion and security lifecycle events;
13. add transactional outbox delivery for protected operations;
14. complete revision and `If-Match` rules per mutable resource;
15. add durable idempotency-key and operation replay records;
16. add protected audit queries, redaction, retention and audit-of-audit;
17. complete deny-path, outage, failure-injection, full-suite and real-runtime acceptance.

Exit criteria:

- different actors can hold different rights on the same backend;
- denial is enforced server-side for every protected route;
- browser sessions are securely issued, expired, revoked and CSRF-protected;
- the second-house/read-only scenario remains proven;
- every privileged mutation has actor, decision and outcome evidence;
- required pre-dispatch accountability failure prevents dispatch;
- revision and idempotency contracts are enforced where required;
- Agent identities can be represented for Phase 63.

Forbidden shortcuts:

- no frontend-owned role or CSRF decision;
- no ordinary log parsing as the accountability database;
- no compatibility-mode claim as final authentication;
- no plaintext, reversible or submitted credential persistence;
- no complete cookie, raw session secret or raw CSRF persistence;
- one-way verifier hashes belong only in dedicated verifier repositories;
- no lifecycle-only browser gate may authenticate ordinary application routes;
- no managed identity may inherit the legacy unmigrated-POST bypass;
- no browser-cookie business mutation dispatch without server-side CSRF verification;
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
- **Mutation gate:** authentication, browser CSRF where applicable, authorization, revision, idempotency, durable dispatch evidence, verification and accountability.
- **Native boundary gate:** no raw VDR pointer or lock crosses into asynchronous/network/database work.
- **Client gate:** clients consume Suite-owned contracts, never private plugin/provider details.
- **Acceptance gate:** focused tests, regressions, build/package validation and real-system acceptance where native behaviour changes.

## Related documents

- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [Architecture Gap Matrix](architecture-audit-gap-matrix.md)
