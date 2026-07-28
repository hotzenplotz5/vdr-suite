# Phase 62 Persistent Identity Lifecycle — Slice 2

Status: lifecycle and managed Basic increments are real-VDR accepted; browser-session verifier and atomic issuance foundations are implemented and CI validated but are not yet connected to HTTP login, cookie authentication, or CSRF enforcement; Phase 62 remains open

## Purpose

Phase 62 Slice 1 established explicit request identity, centralized authorization, the first protected mutation, and append-only pre-dispatch accountability. Slice 2 removes transient-only identity assumptions and builds the browser/native credential lifecycle in small fail-closed increments.

The slice now contains four cumulative increments:

1. persistent actor, device, session, and credential lifecycle state;
2. an optional separately provisioned managed Basic verifier;
3. a persistent browser-session credential repository and cookie/CSRF verifier;
4. atomic server-side browser-session material generation and persistence.

The active daemon request path still authenticates only the transitional legacy Basic credential and the optional managed Basic credential. The browser-session issuer and authenticator are linked and tested foundations, not active HTTP behavior.

## Current runtime boundary

```text
HttpServerRequest
  -> LegacyBasicAuthenticator (transitional browser compatibility)
  -> optional ManagedBasicAuthenticator
       -> CredentialVerifierRepository
  -> PersistentIdentityResolver
       -> SecurityIdentityRepository
  -> SecurityHttpGate
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
```

A successful Basic verifier result is always followed by persistent actor, device, credential, and session lifecycle resolution before authorization.

## Staged browser-session boundary

```text
future authenticated login request
  -> BrowserSessionIssuanceService
       -> Linux getrandom CSPRNG
       -> independent identifiers, session secret, CSRF secret, and salts
       -> SHA-512 crypt verifier hashes
       -> BEGIN IMMEDIATE
            -> validate actor/device/issuing credential
            -> create security_sessions row
            -> create security_credentials row
            -> create security_browser_session_credentials row
          COMMIT or ROLLBACK
       -> one-time move-only issuance result

future Cookie request
  -> BrowserSessionAuthenticator
       -> strict vdr_suite_session parsing
       -> token-id lookup
       -> session-secret verification
       -> independent X-CSRF-Token verification
  -> future PersistentIdentityResolver
  -> future SecurityHttpGate integration
```

Neither `TestHttpServer` nor `SecurityHttpGate` invokes these browser-session components yet. No login/logout route, `Set-Cookie`, authentication precedence, real mutation CSRF denial, or browser-session accountability is claimed.

## Implemented lifecycle and managed Basic scope

- additive `security_actors`, `security_devices`, `security_sessions`, and `security_credentials` tables;
- active, expiry, and revocation state evaluated for every authenticated Basic request;
- restart-safe compatibility bootstrap with `INSERT OR IGNORE`;
- optional separately provisioned managed actor/device/session/credential;
- dedicated `security_basic_credential_verifiers` table;
- strict bounded Basic parsing;
- thread-safe `crypt_r` verification of yescrypt (`$y$`) and SHA-512 crypt (`$6$`) hashes;
- constant-time verifier-result comparison;
- managed identities receive no default permission and never inherit the legacy unmigrated-POST bypass;
- stable fail-closed lifecycle and credential errors before router dispatch.

## Browser-session storage and verifier scope

`BrowserSessionCredentialRepository` owns `security_browser_session_credentials`. Each row stores:

- non-secret token ID;
- actor, device, session, browser credential, and issuing credential bindings;
- separate one-way modular hashes for the session and CSRF secrets;
- active, expiry, revocation, creation, and update state.

The conceptual cookie value is:

```text
<token-id>.<high-entropy-session-secret>
```

The complete cookie value, raw session secret, raw CSRF token, submitted Cookie header, Authorization header, plaintext password, and reversible secret are not persisted.

`BrowserSessionAuthenticator` provides bounded parsing, duplicate rejection, invalid/expired/revoked/authenticated outcomes, context construction, and independent CSRF verification only for a valid active session.

## Atomic browser-session issuance contract

`BrowserSessionIssuanceService` generates all secret material server-side.

### Entropy and identifiers

- entropy is read from Linux `getrandom(2)` with complete-read and `EINTR` handling;
- token ID, session ID, and credential ID each contain 128 random bits with distinct prefixes;
- session secret and CSRF secret each contain independent 256 random bits encoded as unpadded Base64url;
- the two secrets use independent random crypt salts;
- both hashes use SHA-512 crypt with `rounds=10000`;
- token ID is a lookup identifier, not an authentication secret.

### Lifetime

- default: 28,800 seconds (8 hours);
- minimum: 300 seconds (5 minutes);
- maximum: 86,400 seconds (24 hours);
- one UTC expiry is written to session, credential, and verifier records.

### Issuing identity validation

Inside the write transaction the service re-reads and validates:

- active, unrevoked actor;
- active, unrevoked device owned by that actor;
- active, unexpired, unrevoked issuing credential owned by that actor.

Caller-supplied lifecycle claims are not trusted.

### Atomic persistence

The service acquires the transaction lease and starts `BEGIN IMMEDIATE`. It creates:

1. one `security_sessions` row;
2. one `security_credentials` row of type `browser-session`, with the issuing credential recorded as rotation predecessor;
3. one `security_browser_session_credentials` row containing lookup identity and one-way hashes.

All three rows commit together. Any validation failure, identifier conflict, repository failure, or commit failure rolls back. The collision test deliberately lets the first two inserts succeed and forces the verifier insert to fail; neither intermediate identity row survives.

### One-time result handling

A successful issuance returns IDs, sensitive cookie value, sensitive CSRF token, and expiry. `IssuedBrowserSession` is move-only; copy construction and assignment are disabled. Its destructor and `clearSecrets()` overwrite the sensitive buffers. The result is not exposed by an API and must not be logged.

## SQLite ownership boundary

Phase 62 SQL remains in approved repository implementation units under `core/security/src/`.

- public repository headers contain declarations and domain values only;
- `BrowserSessionIssuanceService` uses `Database` and repository abstractions and has no direct `sqlite3_*` call;
- `SecurityIdentityIssuanceRepository.cpp` owns session/credential inserts;
- the daemon and affected tests link the complete `SECURITY_SRC` graph;
- arbitrary services, headers, REST controllers, helpers, and unregistered tests remain outside the direct SQLite boundary.

## Route-safety rule

The legacy compatibility bypass belongs only to the configured legacy actor and credential. Managed Basic identities use migrated routes only with explicit permission and scope; unmigrated POSTs fail before dispatch.

The staged browser-session issuer and authenticator do not alter runtime behavior until HTTP and Gate integration is added.

## Real-VDR acceptance of the persistence/revocation foundation

The lifecycle increment was installed on yaVDR using `/var/lib/vdr-suite/vdr-suite.db`.

Observed on 2026-07-28:

- startup created compatibility identity bindings;
- revoking `legacy-basic-credential` blocked browser authentication immediately without restart;
- accountability recorded `credential_revoked` and `dispatch_denied`;
- restoring the credential restored browser access without restart;
- the next Remote request was authorized as `remote.control@default` and recorded as `dispatch_authorized`.

## Real-VDR acceptance of the managed verifier increment

Observed on 2026-07-28:

- separate managed actor `user-phase62-admin`, device, session, credential, and `$6$` verifier were provisioned;
- correct credentials returned `200` for `GET /api/backends`;
- wrong password returned `401 invalid_credentials` with `no-store` and no reflection;
- unmigrated Timer POST returned `503 security_policy_not_migrated` before dispatch;
- Remote operation `phase62-managed-1785212278` succeeded with `remote.control@default`;
- accountability recorded invalid, denied/unmapped, and allowed/dispatch-authorized decisions with expected identity attribution.

The browser-session foundations have not been installed for real-runtime acceptance because no daemon request path invokes them.

## Automated validation

GitHub Actions VDR-Suite CI run 6367 completed successfully on atomic issuance code head `b79e3fdc597dd8eb245641ba2c7363dd9542e631`.

It passed documentation/structure, Make/test inventory, frontend regression, complete fast regression, issuance architecture and service tests, full daemon build, and packaging/install staging.

The issuance test covers deterministic IDs/expiry, secret formats, one-way persistence, lifecycle creation, successful authenticator/CSRF consumption, forced-collision rollback, lifetime limits, entropy failure, revoked issuing credential, move-only semantics, and secret clearing.

## Explicitly not included

- HTTP login/logout;
- request parsing or login response;
- `Set-Cookie` construction or deployment policy;
- browser authentication precedence;
- resolver/Gate browser integration;
- real CSRF rejection before dispatch;
- browser lifecycle accountability;
- refresh, idle timeout, cleanup, concurrent-session policy, recovery, or enrollment;
- managed/native/service credential administration;
- persisted roles, grants, or scopes;
- complete route migration;
- universal revision, idempotency, operation lifecycle, or outbox completion;
- Phase 63-67 runtime or clients.

## Acceptance evidence

| Criterion | Evidence |
|---|---|
| Persistent lifecycle is enforced and recoverable | repository tests and real-VDR revoke/restore |
| Managed Basic is independent from legacy compatibility | tests and real-VDR acceptance |
| Browser secrets persist only as independent one-way hashes | repository/authenticator/issuance/architecture tests |
| Entropy is server-generated | `getrandom(2)` and architecture guard |
| Lifetime is bounded | constants and negative tests |
| Issuing identity is revalidated inside transaction | implementation and revoked-source test |
| Session, credential, and verifier are atomic | forced-collision rollback test |
| Issued secrets are move-only and wiped | static assertions and clearing test |
| Issued material works with staged verifier | positive authentication and CSRF test |
| Browser runtime is not overstated | explicit no-HTTP/Gate wiring guard |
| Daemon and packaging link the service | CI run 6367 |

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Phase 62 remains open. The next strict Slice 2 increment is HTTP login/logout and secure cookie response construction, followed by authentication precedence, resolver/Gate integration, accountability, and real CSRF enforcement.
