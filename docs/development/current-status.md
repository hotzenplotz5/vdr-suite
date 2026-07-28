# VDR-Suite Current Project Status

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

Current Phase 62 status:
Active. Slice 1 is real-runtime validated. Slice 2 lifecycle persistence and managed Basic are real-runtime accepted. Browser-session verifier and atomic server-side issuance foundations are implemented and CI validated, but no HTTP login/logout, Set-Cookie, cookie authentication, or real CSRF enforcement is active.
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase 63-67 runtime has not been advanced.

## Stable implemented scope

- daemon-owned SQLite, migrations, repositories and backend registry;
- backend-scoped snapshots, change feed, SSE foundation and server-enforced read-only mode;
- channels, EPG timeline, channel-day view, Recording and Timer read paths;
- Recordings 2 with metadata, people, artwork, Genres and guarded actions;
- SearchTimer list, preview, validation and controlled mutation foundations;
- persistent Recording/EPG metadata, people and Genre read models;
- query-only provider-free Genre and global-search GET paths;
- backend-neutral RemoteAction and LiveOverlay paths;
- isolated remote pressed-state and duplicate-dispatch guard from PR #110;
- backend-scoped global search from PR #111;
- merged 360×1220 PNG Remote and extended help/navigation/REC behaviour from PR #115;
- modular frontend Client API ownership and install/runtime staging.

## Phase 62 Slice 1

Implemented and real-runtime validated:

- `RequestSecurityContext` with actor, device, session, credential, grants, request ID and correlation ID;
- centralized `AuthorizationService` with exact/wildcard permission and backend scope decisions;
- explicit legacy compatibility and fail-closed enforced modes;
- server-side protection of `POST /api/vdr/remote/actions` with `remote.control`;
- append-only accountability persistence before dispatch;
- stable security errors without credential reflection;
- real yaVDR evidence for anonymous denial, invalid-credential denial and authenticated Browser Remote dispatch.

`BackendAccessPolicy` remains an independent backend-state guard and never replaces actor authorization.

## Phase 62 Slice 2

### Lifecycle persistence — implemented and real-runtime accepted

- additive `security_actors`, `security_devices`, `security_sessions` and `security_credentials` tables;
- compatibility bootstrap without storing Authorization secrets;
- persisted actor/device/session/credential ownership bindings;
- request-time `PersistentIdentityResolver` before authorization;
- persisted expiry and revocation enforcement;
- restart-safe bootstrap that never reactivates revoked records;
- real yaVDR revoke/restore evidence without daemon restart.

### Managed Basic verifier — implemented and real-runtime accepted

- optional separate managed actor/device/session/credential provisioning;
- `security_basic_credential_verifiers` login-to-credential binding;
- yescrypt or SHA-512 crypt one-way password hashes;
- strict bounded Basic parsing and thread-safe `crypt_r` verification;
- no managed identity or permission enabled by default;
- startup failure for partial, unsupported or conflicting configuration;
- no legacy bypass for managed identities;
- real yaVDR positive GET, wrong-password 401, unmigrated Timer 503, migrated Remote 200 and accountability evidence.

### Browser-session verifier — implemented and CI validated, not HTTP-wired

- additive `security_browser_session_credentials` table;
- actor/device/session/browser-credential/issuing-credential bindings;
- non-secret lookup token plus separate one-way session and CSRF hashes;
- bounded `vdr_suite_session` parsing and duplicate rejection;
- independent `X-CSRF-Token` verification;
- active, expiry and revocation outcomes;
- positive and negative cookie/CSRF tests;
- architecture guards against raw browser-secret persistence.

### Atomic browser-session issuance — implemented and CI validated, not HTTP-exposed

- Linux `getrandom(2)` as production CSPRNG with full-read and `EINTR` handling;
- independent 128-bit token/session/credential IDs;
- independent 256-bit Base64url session and CSRF secrets;
- independent SHA-512 crypt salts and `rounds=10000` verifier hashes;
- bounded lifetime: 5 minutes minimum, 8 hours default, 24 hours maximum;
- actor/device/issuing-credential revalidation inside `BEGIN IMMEDIATE`;
- atomic creation of session, browser credential and verifier rows;
- rollback on validation, collision, repository or commit failure;
- forced-collision test proving no intermediate lifecycle rows survive;
- move-only issuance result whose cookie and CSRF buffers are explicitly wiped;
- issued material successfully consumed by the staged authenticator and CSRF verifier.

The active daemon still authenticates requests only through legacy or managed Basic. The issuance service is linked but not invoked by `TestHttpServer` or `SecurityHttpGate`, so installed request behaviour is unchanged.

## Open Phase 62 limitations

The platform still lacks:

- HTTP browser-login request parsing and response contract;
- logout and atomic coupled session/credential revocation;
- documented secure `Set-Cookie` attributes and HTTP/HTTPS deployment behaviour;
- browser-cookie authentication precedence and `PersistentIdentityResolver`/`SecurityHttpGate` integration;
- actual CSRF rejection before mutation dispatch;
- browser issuance/login/logout/CSRF accountability events;
- refresh, idle expiry, cleanup, concurrent-session policy and recovery;
- protected managed/native/service credential lifecycle administration;
- persisted roles, grants and backend/resource scopes;
- complete mutation and sensitive-read permission migration;
- universal revision and idempotency contracts;
- mutation outcome and transactional-outbox delivery;
- complete security event catalogue and protected audit query/retention;
- full failure injection and real-runtime closeout across all migrated routes.

## Pull request truth

- PR #115 is merged and defines current Remote runtime truth.
- PR #113 is closed as superseded by #115.
- PR #112 remains an old-base Draft and is not current runtime truth.
- PR #116 remains an open Draft; proposed ADR-0051 is consumer context only.
- PR #117 is the active Phase 62 Draft and must not be merged or auto-merged by this workflow.

## Immediate implementation focus

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Implement HTTP login/logout and secure cookie response construction. Then connect browser authentication to `PersistentIdentityResolver` and `SecurityHttpGate`, enforce CSRF before router dispatch, and add accountability before moving to persisted roles/grants and complete route migration.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](phase-62-security-identity-foundation-slice-2.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Current Architecture State](current-architecture-state.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)
