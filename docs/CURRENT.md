# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Phase 62 Slice 1](development/phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](development/phase-62-security-identity-foundation-slice-2.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [Architecture Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](development/completed-phases.md)
- [ADR Index](adr/index.md)

## Repository baseline

This document was reconciled from `origin/main` commit `cb77ff66e11dca7db2eafa36525762dcde35102d`, the merge of PR #115, plus the active Draft PR #117 branch.

The SHA is a time-bound evidence point. Every task must fetch the relevant branch and inspect the worktree before editing.

## Current verified position

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
Active. Slice 1 is real-runtime validated. Slice 2 lifecycle and managed Basic are real-runtime accepted. Browser-session verification, atomic issuance and two isolated HTTP lifecycle routes are implemented and CI validated. Ordinary application-route cookie authentication, business-mutation CSRF enforcement and real-yaVDR acceptance of the HTTP lifecycle remain open.
```

Phase 61 remains completed. Phase 62 is not complete. Phase 63-67 runtime has not been advanced.

## Implemented runtime truth on `main`

Current `main` contains:

- daemon-owned SQLite persistence, migrations and domain repository boundaries;
- BackendRegistry, backend-scoped snapshots, change feed and read-only policy;
- channels, EPG, Recordings 2 and Timer/SearchTimer foundations;
- persistent metadata, people, Genre evidence and query-only browse paths;
- backend-neutral RemoteAction and LiveOverlay APIs;
- backend-scoped global search;
- the merged configurable 360×1220 PNG Remote with 35 hotspots;
- packaging, install staging, daemon builds and real-system acceptance workflows.

## Active Phase 62 runtime request paths

```text
POST /api/security/browser-sessions
  -> BrowserSessionHttpGate
       -> legacy or managed Basic authentication
       -> PersistentIdentityResolver
       -> session.issue.self accountability
  -> BrowserSessionHttpService
       -> atomic BrowserSessionIssuanceService
       -> hardened Set-Cookie plus one-time CSRF JSON

POST /api/security/browser-sessions/logout
  -> BrowserSessionHttpGate
       -> BrowserSessionAuthenticator
       -> PersistentIdentityResolver
       -> X-CSRF-Token verification
       -> session.revoke.self accountability
  -> BrowserSessionHttpService
       -> atomic verifier/session/credential revocation
       -> expired hardened cookie

Every other request
  -> SecurityHttpGate
       -> LegacyBasicAuthenticator
       -> optional ManagedBasicAuthenticator
            -> CredentialVerifierRepository
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
```

The browser lifecycle gate recognizes only the two exact POST routes above. Browser cookies do not authenticate ordinary GET routes, Remote, Timer, Recording or other application APIs.

## Phase 62 Slice 1 — implemented and real-runtime validated

- canonical actor, device, session, credential and request context;
- anonymous, authenticated, invalid, expired and revoked states;
- backend-scoped permissions and centralized authorization;
- stable credential-safe errors;
- protected `POST /api/vdr/remote/actions` using `remote.control@backend`;
- append-only pre-dispatch accountability;
- request/correlation ID propagation;
- explicit `legacy-basic` and `enforced` rollout modes;
- real yaVDR anonymous, invalid and authenticated Remote evidence.

## Phase 62 Slice 2 — lifecycle and managed Basic real-runtime accepted

- additive actor, device, session, credential and Basic-verifier tables;
- restart-safe lifecycle bootstrap;
- request-time persistent lifecycle resolution;
- expiry/revocation enforcement;
- optional separate managed identity;
- strict Basic parsing and `crypt_r` verification;
- no managed defaults;
- no legacy bypass for managed identities;
- real yaVDR positive GET, wrong-password 401, unmigrated Timer 503, migrated Remote 200 and accountability evidence.

## Phase 62 Slice 2 — browser verifier and atomic issuance CI validated

- `security_browser_session_credentials` with exact identity and issuing-credential bindings;
- non-secret token ID and independent one-way session/CSRF hashes;
- strict bounded cookie parser and duplicate rejection;
- independent CSRF verifier;
- expiry and revocation outcomes;
- Linux `getrandom(2)` production entropy;
- 128-bit token/session/credential IDs;
- independent 256-bit session and CSRF secrets;
- bounded 5-minute to 24-hour lifetime, 8-hour default;
- actor/device/issuing-credential validation inside `BEGIN IMMEDIATE`;
- atomic session, browser credential and verifier persistence;
- rollback proof through a forced token collision;
- move-only result with explicit secret-buffer wiping.

## Phase 62 Slice 2 — isolated HTTP lifecycle CI validated

### Login exchange

`POST /api/security/browser-sessions`:

- accepts an already authenticated legacy or managed Basic identity;
- does not accept a plaintext-password JSON body;
- returns `200` with only `csrfToken`, `expiresAt` and `requestId`;
- emits the session secret only through `Set-Cookie`;
- uses `Path=/`, `Max-Age=28800`, `HttpOnly`, `Secure` and `SameSite=Strict`;
- emits no `Domain` attribute;
- uses `Cache-Control: no-store` and `Pragma: no-cache`.

### Logout

`POST /api/security/browser-sessions/logout`:

- requires the browser cookie and matching `X-CSRF-Token`;
- does not accept Basic as a substitute;
- rejects missing/wrong CSRF with `403 csrf_validation_failed`;
- revokes verifier, canonical session and browser credential atomically;
- returns `204` and an expired hardened cookie.

Complete cookie values, raw session secrets, raw CSRF values, plaintext passwords and submitted Authorization/Cookie headers are not persisted or reflected.

## Open Phase 62 work

Phase 62 still requires:

- install and real-yaVDR acceptance of the new login/logout routes and HTTPS proxy behaviour;
- browser authentication precedence for ordinary application routes;
- controlled cookie-context integration with persistent lifecycle and centralized authorization;
- browser grant loading;
- actual CSRF enforcement before Remote, Timer, Recording and other applicable business mutations;
- frontend login/logout and in-memory CSRF handling;
- complete issuance/revocation outcome accountability and transactional coupling/outbox;
- refresh, idle expiry, cleanup, concurrency and recovery policy;
- protected managed/native/service credential administration;
- persisted roles, permissions, grants and scopes;
- complete route authorization migration;
- universal revision, idempotency and operation lifecycle;
- protected audit reads, redaction and retention;
- failure injection and real-runtime closeout.

## Compatibility boundary

The local browser remains compatible through `legacy-basic`. Only the exact legacy actor/credential receives the temporary unmigrated-POST bypass.

Managed Basic identities can use authenticated reads and explicitly migrated routes, but unmigrated POSTs fail with `security_policy_not_migrated`.

After installation, the two browser lifecycle routes add session issue/revoke behaviour without changing ordinary application-route authentication. General application requests remain on the existing Basic path until the next controlled integration increment.

## Pull request classification

| PR | Repository truth |
| ---: | --- |
| #112 | Open old-base Draft; not current runtime truth. |
| #113 | Closed unmerged; superseded by PR #115. |
| #114 | Merged documentation truth refresh. |
| #115 | Merged configurable photorealistic Remote; current `main` baseline. |
| #116 | Open Draft Android/client feasibility; proposed ADR-0051 is not accepted runtime truth. |
| #117 | Open Draft Phase 62 implementation; Basic lifecycle is real-runtime accepted, browser verifier/issuer and isolated HTTP lifecycle are CI validated. Must not be auto-merged. |

## Later phase boundaries

- Phase 63: Backend Agent and remote-site runtime.
- Phase 64: TimerIntent and multi-backend orchestration.
- Phase 65: Streaming Gateway and media sessions.
- Phase 66: legacy OSD compatibility runtime.
- Phase 67: stable public `/api/v1`, SDK and compatibility release.

The current `/api/...` routes are compatibility routes, not a stable public API.

## Boundary rules

- VDR remains native runtime authority.
- VDR-Suite owns external identity, policy, orchestration, persistence and client contracts.
- Browsers never call private backend protocols directly.
- Credential verification, authentication, lifecycle resolution, CSRF and authorization are separate server decisions.
- The isolated lifecycle gate may not become a shortcut around general application authorization.
- Backend read-only and capability checks remain independent of actor permissions.
- Frontends never own authorization or CSRF policy.
- Secrets do not belong in URLs, logs, errors, request IDs, lifecycle rows or accountability payloads.
