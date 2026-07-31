# VDR-Suite Current Architecture State

## Purpose

This document describes implemented architecture. Accepted target contracts that are not yet connected to runtime remain in ADRs and planning documents.

Baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` on 2026-07-27, plus the active Phase 62 Draft branch.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugins

VDR-Suite
  -> backend identity and scope
  -> credential verification and session issuance
  -> actor/device/credential/session/request context
  -> persistent identity lifecycle
  -> server-side authorization and CSRF policy
  -> guarded operations and accountability
  -> client-facing REST and Client API contracts

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge
```

Frontend modules do not call private backend protocols directly and do not own authentication, lifecycle, CSRF or authorization decisions.

## Backend and snapshot foundation

BackendRegistry, backend-scoped reads, capability reporting, read-only policy, snapshots, change feed and live-update foundations are implemented. Secure remote Agent identity and command fencing remain Phase 63 work.

## Active general Phase 62 runtime security boundary

```text
HttpServerRequest
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
  -> controller/service/domain safety checks
```

The migrated mutation catalogue currently includes Remote, Timer
create/update/delete and both Channel Move aliases. Each requires its canonical
permission at the exact requested backend plus a durable pre-dispatch decision.

Actor, device, credential, session, authentication state, grants, request ID and correlation ID are represented explicitly. Persistent inactive, expired, revoked, missing or mismatched state fails closed before router dispatch.

The installed Webfrontend exchanges Basic credentials for a browser session
and then uses browser-cookie authentication with memory-only CSRF state.
Legacy Basic remains a transitional compatibility path. Managed Basic and
browser identities have their own lifecycle and grants and do not inherit the
Legacy Basic unmigrated-POST bypass.

## Active browser-session and ordinary-route security boundary

The installed daemon exposes two exact browser lifecycle POST routes through
a dedicated lifecycle gate. Valid browser cookies also authenticate ordinary
application routes through the general security gate:

```text
POST /api/security/browser-sessions
  -> BrowserSessionHttpGate
       -> LegacyBasicAuthenticator or ManagedBasicAuthenticator
       -> PersistentIdentityResolver
       -> session.issue.self accountability
  -> BrowserSessionHttpService
       -> BrowserSessionIssuanceService
            -> getrandom CSPRNG
            -> independent identifiers, secrets and salts
            -> one-way verifier hashes
            -> BEGIN IMMEDIATE transaction
                 -> validate actor/device/issuing credential
                 -> create session identity
                 -> create browser credential identity
                 -> create browser verifier
               COMMIT or ROLLBACK
            -> move-only one-time result
       -> hardened Set-Cookie
       -> one-time CSRF JSON

POST /api/security/browser-sessions/logout
  -> BrowserSessionHttpGate
       -> BrowserSessionAuthenticator
            -> strict vdr_suite_session parsing
            -> token-id lookup
            -> BrowserSessionCredentialRepository
            -> session-secret verification
            -> independent X-CSRF-Token verification
       -> PersistentIdentityResolver
       -> session.revoke.self accountability
  -> BrowserSessionHttpService
       -> BrowserSessionLifecycleService
       -> atomic verifier/session/credential revocation
       -> expired hardened Set-Cookie
```

The lifecycle routes remain separately handled, but their issued cookie is now
accepted by `SecurityHttpGate` for ordinary application requests. Browser
credentials take precedence when presented and never fall back to Basic after
an invalid cookie.

### Issuance material

- token/session/credential IDs: independent 128-bit values with distinct prefixes;
- session secret: independent 256-bit unpadded Base64url;
- CSRF secret: independent 256-bit unpadded Base64url;
- independent random crypt salts;
- SHA-512 crypt hashes with `rounds=10000`;
- lifetime bounded to 5 minutes minimum, 8 hours default and 24 hours maximum.

Entropy comes from Linux `getrandom(2)` with complete-read and `EINTR` handling.

### Atomicity

The issuer re-reads actor, device and issuing credential inside `BEGIN IMMEDIATE`. It rejects inactive, expired, revoked, missing or cross-actor state. Session identity, browser credential identity and verifier row commit together.

A forced token-collision test causes failure after the lifecycle inserts and proves that rollback removes both intermediate rows.

### Result ownership

`IssuedBrowserSession` is move-only. It returns IDs, cookie value, CSRF token and expiry to the dedicated HTTP lifecycle service. Its destructor and `clearSecrets()` overwrite the sensitive cookie and CSRF buffers. It must not be logged or persisted.

### Verifier

The browser verifier stores only a non-secret lookup token and separate one-way session/CSRF hashes bound to actor, device, session, browser credential and issuing credential.

Strict parsing rejects malformed and duplicate target cookies. Authentication distinguishes anonymous, invalid, expired, revoked and authenticated states. CSRF verification is independent and succeeds only for an active valid session.

The lifecycle boundary issues hardened HTTP responses, enforces cookie-bound
CSRF on logout and writes lifecycle authorization accountability. The general
gate now enforces browser CSRF and exact backend authorization for Remote,
Timer create/update/delete and Channel Move. Every other browser business POST
remains fail-closed.

## Independent safety decisions

The following remain separate and cumulative:

1. credential verification;
2. authentication context construction;
3. persistent identity lifecycle resolution;
4. browser CSRF verification;
5. actor permission and backend scope authorization;
6. backend availability/read-only policy;
7. capability policy;
8. mutation validation, revision, idempotency and operation policy;
9. accountability decision and outcome evidence.

No frontend state replaces these server decisions.

## Current persistence boundaries

- SQLite is the central Suite-owned metadata/read-model database.
- Domain repositories own SQL; controllers and frontend modules do not.
- `SecurityIdentityRepository` owns lifecycle reads and state changes.
- `SecurityIdentityProvisioningRepository` owns idempotent configured bootstrap.
- `SecurityIdentityIssuanceRepository.cpp` owns issuance-specific session/credential inserts.
- `CredentialVerifierRepository` owns managed Basic verifiers.
- `BrowserSessionCredentialRepository` owns browser token binding, one-way hashes, expiry and revocation.
- `SecurityPermissionGrantRepository` owns active backend-scoped actor grants used by browser contexts and fixed roles.
- `BrowserSessionIssuanceService` owns entropy, hashing and transaction orchestration without direct SQLite calls.
- `AccountabilityEventRepository` owns append-only authorization evidence.
- Database triggers reject accountability updates and deletes.
- submitted Authorization/Cookie headers, plaintext passwords, complete cookie values, raw session secrets and raw CSRF values are not persisted.
- ADR-0050 remains the domain-repository SQLite boundary.

## Implemented safety boundaries

- backend-scoped reads and read-only policy;
- guarded Recording and Timer-related domain operations;
- allowlisted Remote actions and operation IDs;
- centralized authorization for Remote, Timer create/update/delete and both
  Channel Move aliases;
- fixed exact-backend Admin expansion and Read-only precedence;
- persisted lifecycle and browser-grant resolution before authorization;
- managed Basic verification and legacy-bypass separation;
- fail-closed expiry/revocation and unmigrated-route handling;
- append-only pre-dispatch accountability;
- strict isolated browser-cookie and independent CSRF verification;
- CSPRNG-backed atomic browser-session issuance;
- rollback proof and move-only secret handling;
- architecture guards against raw secret persistence and premature ordinary-route cookie wiring;
- no frontend-owned authorization or provider lookup on documented query-only paths.

## Domain architecture retained

- Recordings 2 remains the sole delivered recording browser and owns folder/card/detail/action workflows.
- Phase 61 metadata, people, Genre and EPG read models remain backend-scoped and provider-free on normal reads.
- Global Search uses the Suite Client API and query-only SQLite connection.
- Remote/LiveOverlay remain backend-neutral Suite APIs over private adapters.
- SearchTimer and Timer foundations remain partial and are not central Phase 64 orchestration.

## Important incomplete architecture

| Area | Current state | Roadmap owner |
|---|---|---|
| Actor/request model | Basic and isolated browser lifecycle boundaries are real-runtime accepted and share the canonical identity model | Phase 62 |
| Credential verification | Managed Basic and isolated browser verifier accepted; native/service and protected password lifecycle missing | Phase 62 |
| Browser session issuance | Exact Basic-to-session endpoint, hardened cookie and atomic three-row persistence real-runtime accepted | Phase 62 |
| Browser authentication and CSRF | Browser-cookie precedence is active; logout, Remote, Timer create/update/delete and Channel Move enforce matching CSRF; remaining browser POSTs fail closed | Phase 62 |
| Logout/session management | Atomic verifier/session/credential logout accepted; refresh, idle timeout, cleanup and recovery remain open | Phase 62 |
| Roles/grants/scopes | Backend-scoped actor grants and fixed exact-scope Admin/Read-only roles accepted; generic definitions and protected administration missing | Phase 62 |
| Complete server authorization | Remote, Timer create/update/delete and Channel Move migrated; remaining route families still require classification and migration | Phase 62 |
| Accountability | Lifecycle and migrated-mutation pre-dispatch decisions are real-runtime accepted; completion, outbox and queries remain open | Phase 62 |
| Revision/idempotency | Partial domain mechanisms only | Phase 62 |
| Backend Agent | Contract/foundation only | Phase 63 |
| TimerIntent orchestration | Missing | Phase 64 |
| Streaming Gateway | Missing | Phase 65 |
| Legacy OSD bridge | Missing | Phase 66 |
| Stable `/api/v1` and SDK | Partial/unreleased | Phase 67 |

## Related documents

- [Current State](../CURRENT.md)
- [Security and Identity Foundation](../architecture/security-identity-foundation.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](phase-62-security-identity-foundation-slice-2.md)
- [Strict Roadmap](../planning/roadmap.md)
