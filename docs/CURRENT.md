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
- [Phase 62 Runtime Evidence](development/phase-62-runtime-evidence.md)
- [Phase 62 Slice 2H Channel Move](development/phase-62-slice-2h-channel-move-security-migration.md)
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
Active and incomplete, but repository-, CI- and real-runtime accepted through
Slice 2H.

Accepted source/runtime head:
2e0b31f671edf18393d7d48ea6e15697fc3a044d

Successful GitHub Actions run:
#6559
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30627974107

Installed daemon SHA-256:
ff7582b6fdb6a2faa7d0e29f6795ad634ea76d95a42280a6140e005e249cbf52

Installed deferred runtime loader SHA-256:
e4860a2b7c613919f3a084fc625f398bd5f339191ae48133cfc76431c0189ca9
```

Accepted cumulative Phase 62 scope now includes persistent lifecycle and
credentials, browser-session issue/logout, ordinary-route browser
authentication, persisted actor grants, fixed exact-backend Admin/Read-only
roles, Webfrontend memory-only CSRF handling, Remote mutation migration, Timer
create/update/delete migration and both Channel Move aliases.

Phase 61 remains completed. Phase 62 is not complete. Phase 63-67 runtime has
not been advanced.

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
  -> Basic authentication and persistent identity resolution
  -> session.issue.self accountability
  -> atomic browser-session issuance
  -> hardened HttpOnly cookie plus one-time in-memory CSRF token

POST /api/security/browser-sessions/logout
  -> browser-cookie authentication
  -> persistent lifecycle and grant resolution
  -> cookie-bound CSRF verification
  -> session.revoke.self accountability
  -> atomic verifier/session/credential revocation

Ordinary application request
  -> browser cookie has strict precedence when presented
  -> otherwise Legacy Basic or optional Managed Basic
  -> PersistentIdentityResolver
  -> persisted actor-grant resolution for browser contexts
  -> exact route classification
  -> browser CSRF before authorization for migrated mutations
  -> AuthorizationService
  -> append-only pre-dispatch accountability
  -> ApiRouter
  -> independent backend and domain safety checks
```

The currently migrated browser-authenticated business mutations are:

```text
POST /api/vdr/remote/actions
  permission: remote.control@<backend-id>

POST /api/vdr/timers/actions/create
  permission: timers.create@<backend-id>

POST /api/vdr/timers/actions/update
  permission: timers.modify@<backend-id>

POST /api/vdr/timers/actions/delete
  permission: timers.delete@<backend-id>

POST /api/vdr/channels/move
POST /api/vdr/channels/actions/move
  permission: channels.move@<backend-id>
```

Query strings are stripped for exact-route classification. Trailing-slash and
unrelated browser POST variants remain fail-closed.

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

## Cumulative acceptance through Slice 2H

Repository regression, daemon build, packaging and GitHub Actions CI #6559 all
passed at the accepted source head.

The guarded installed-runtime pass verified:

- exact installed daemon and deferred-loader fingerprints;
- direct and public frontend loader delivery;
- Legacy Basic compatibility for both Channel Move aliases;
- browser login and hardened cookie issuance;
- missing and invalid CSRF denial before authorization;
- direct permission and exact backend-scope enforcement;
- fixed Admin allowance and Read-only precedence;
- cross-backend Read-only isolation;
- wildcard role rows remaining ineffective as concrete roles;
- backend-policy rejection after successful actor authorization;
- secret-free `channels.move` accountability;
- logout and revoked-cookie replay denial;
- restoration of all temporary grant rows;
- `PRAGMA quick_check = ok`;
- active daemon after acceptance;
- zero real Channel Move operations.

## Open Phase 62 work

Phase 62 still requires:

- migration of the remaining mutating route families, one bounded family per
  slice;
- explicit classification of non-mutating stateful POST routes;
- completion/outcome accountability and stronger transactional coupling;
- refresh, idle expiry, cleanup, concurrency and recovery policy;
- protected identity, credential, grant and role administration;
- native and service credential enrollment, rotation and revocation;
- generic role definitions and assignments beyond the fixed catalogue;
- common revision, idempotency and operation lifecycle contracts;
- protected audit queries, export, redaction and retention;
- final compatibility-retirement readiness and full Phase 62 closeout.

## Compatibility boundary

The local browser remains transitionally compatible through Legacy Basic.
Browser sessions now drive ordinary application authentication and all
explicitly migrated frontend mutations.

Managed Basic and browser actors do not inherit the Legacy Basic compatibility
bypass. Unmigrated POST routes fail closed with
`security_policy_not_migrated`.

Fixed `role.admin` and `role.read-only` assignments are exact-backend grants,
not wildcard enterprise roles. Backend read-only and capability policy remain
independent of actor authorization.

## Pull request classification

| PR | Repository truth |
| ---: | --- |
| #112 | Open old-base Draft; not current runtime truth. |
| #113 | Closed unmerged; superseded by PR #115. |
| #114 | Merged documentation truth refresh. |
| #115 | Merged configurable photorealistic Remote; current `main` baseline. |
| #116 | Open Draft Android/client feasibility; proposed ADR-0051 is not accepted runtime truth. |
| #117 | Open Draft Phase 62 implementation; repository, CI and real-runtime accepted through Slice 2H. Must remain Draft and must not be auto-merged. |

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
