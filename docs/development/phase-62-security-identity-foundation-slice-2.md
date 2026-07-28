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

`BrowserSessionAuthenticator` provides:

- bounded Cookie-header parsing;
- duplicate target-cookie rejection;
- restricted token and secret alphabets;
- invalid, expired, revoked, and authenticated outcomes;
- actor/device/session/credential context construction;
- independent CSRF verification only for a valid active session.

## Atomic browser-session issuance contract

`BrowserSessionIssuanceService` generates all secret material server-side.

### Entropy and identifiers

- entropy is read from Linux `getrandom(2)` with complete-read and `EINTR` handling;
- token ID, session ID, and credential ID each contain 128 random bits encoded as lower-case hexadecimal with distinct prefixes;
- the session secret and CSRF secret each contain 256 random bits encoded as unpadded Base64url;
- the two secrets use independent random 16-character crypt salts;
- session and CSRF hashes use SHA-512 crypt with `rounds=10000`;
- the token ID is a lookup identifier, not an authentication secret.

### Lifetime

- default lifetime: 28,800 seconds (8 hours);
- minimum lifetime: 300 seconds (5 minutes);
- maximum lifetime: 86,400 seconds (24 hours);
- one UTC expiry timestamp is written to the session, credential, and browser-verifier records.

### Issuing identity validation

Inside the write transaction the service re-reads and validates:

- the actor exists, is active, and is not revoked;
- the device exists, belongs to the actor, is active, and is not revoked;
- the issuing credential exists, belongs to the actor, is active, unexpired, and unrevoked.

Caller-supplied lifecycle claims are not trusted.

### Atomic persistence

The service acquires the database transaction lease and starts `BEGIN IMMEDIATE`. It then creates:

1. one `security_sessions` row;
2. one `security_credentials` row of type `browser-session`, with the issuing credential recorded as rotation predecessor;
3. one `security_browser_session_credentials` row containing only lookup identity and one-way hashes.

All three rows commit together. Any validation failure, identifier conflict, repository failure, or commit failure rolls the transaction back. The collision test deliberately lets the first two inserts succeed and then forces the browser token insert to fail; it proves that neither intermediate identity row survives.

### One-time result handling

A successful issuance returns:

- token ID;
- session ID;
- credential ID;
- sensitive cookie value;
- sensitive CSRF token;
- expiry timestamp.

`IssuedBrowserSession` is move-only. Copy construction and copy assignment are disabled. Its destructor and explicit `clearSecrets()` method overwrite the cookie-value and CSRF-token buffers.

This result is not currently exposed by an API and must not be logged.

## SQLite ownership boundary

Phase 62 SQL remains in approved repository implementation units under `core/security/src/`.

- public repository headers contain declarations and domain values only;
- `BrowserSessionIssuanceService` uses the `Database` and repository abstractions and contains no direct `sqlite3_*` call;
- `SecurityIdentityIssuanceRepository.cpp` owns the new session/credential insert statements;
- the daemon and affected tests link the complete `SECURITY_SRC` graph;
- the repository-wide SQLite checker remains narrow and does not allow arbitrary services, headers, REST controllers, helpers, or unregistered tests.

## Route-safety rule

The legacy compatibility bypass belongs only to the configured legacy actor and legacy credential.

A managed Basic identity may use migrated routes only with explicit permission and backend scope. An unmigrated POST returns `503 security_policy_not_migrated` before dispatch.

The staged browser-session issuer and authenticator do not alter this behavior until HTTP and Gate integration are added explicitly.

## Revocation semantics

Current Basic requests fail closed for:

- `actor_revoked`;
- `device_revoked`;
- `credential_expired`;
- `credential_revoked`;
- `session_expired`;
- `session_revoked`;
- missing or mismatched identity as `invalid_credentials`.

The browser-session repository and authenticator independently model active, expiry, and revocation state. Future HTTP integration must also pass the browser context through `PersistentIdentityResolver` so the verifier row never replaces canonical lifecycle state.

## Real-VDR acceptance of the persistence/revocation foundation

The lifecycle increment was installed on yaVDR using `/var/lib/vdr-suite/vdr-suite.db`.

Observed on 2026-07-28:

- daemon startup created the compatibility actor, device, session, and credential bindings;
- revoking `legacy-basic-credential` blocked browser authentication immediately without daemon restart;
- accountability recorded `credential_revoked` and `dispatch_denied`;
- restoring the credential restored browser access without restart;
- the next Remote request was authorized as `remote.control@default` and recorded as `dispatch_authorized`.

## Real-VDR acceptance of the managed verifier increment

Observed on 2026-07-28:

- separate managed actor `user-phase62-admin`, device, session, credential, and `$6$` verifier were provisioned;
- correct credentials returned `200` for `GET /api/backends`;
- a wrong password returned `401 invalid_credentials` with `Cache-Control: no-store` and no reflection;
- an unmigrated Timer POST returned `503 security_policy_not_migrated` before dispatch;
- Remote operation `phase62-managed-1785212278` succeeded with `remote.control@default`;
- accountability recorded the invalid, denied/unmapped, and allowed/dispatch-authorized decisions with expected identity attribution.

The browser-session verifier and issuance foundations have not been installed for real-runtime acceptance because no daemon request path invokes them.

## Automated validation

GitHub Actions VDR-Suite CI run 6367 completed successfully on atomic issuance code head `b79e3fdc597dd8eb245641ba2c7363dd9542e631`.

The run passed:

- documentation and repository-structure checks;
- strict Make/test inventory and complete test-graph dry-run;
- frontend regression;
- complete fast C++/runtime regression;
- browser-session issuance architecture contract;
- atomic issuance service test;
- complete daemon build;
- packaging and install staging.

The issuance test covers:

- deterministic identifier and expiry generation;
- restricted 256-bit session and CSRF secrets;
- one-way verifier persistence;
- lifecycle identity creation;
- successful authentication and CSRF verification of an issued session;
- transaction rollback after a forced token collision;
- minimum and maximum lifetime rejection;
- entropy-source failure;
- revoked issuing-credential rejection;
- explicit secret-buffer clearing;
- move-only result semantics.

## Explicitly not included

- HTTP login or logout routes;
- request parsing for browser login;
- `Set-Cookie` response generation or deployment policy;
- browser-cookie authentication precedence;
- `PersistentIdentityResolver`/`SecurityHttpGate` browser integration;
- real CSRF rejection before mutation dispatch;
- browser-session issuance/logout/accountability events;
- refresh, idle timeout, cleanup, concurrent-session policy, recovery, or device enrollment;
- managed password generation/change or protected lifecycle administration;
- native bearer tokens or service credentials;
- persisted roles, assignments, grants, or backend/resource scopes;
- migration of all remaining protected routes;
- revision, idempotency, operation lifecycle, or transactional outbox completion;
- Phase 63-67 runtime or client implementation.

## Acceptance evidence

| Criterion | Evidence |
|---|---|
| Persistent lifecycle is enforced and recoverable | repository tests and real-VDR revoke/restore acceptance |
| Managed Basic verifier is independent from legacy compatibility | managed authenticator/Gate tests and real-VDR acceptance |
| Browser secrets are stored only as independent one-way hashes | repository, authenticator, architecture, and issuance tests |
| Issuance entropy is server-generated | `getrandom(2)` implementation and architecture guard |
| Issuance lifetime is bounded | 5-minute/24-hour constants and negative tests |
| Issuing identity is revalidated inside the transaction | service implementation and revoked-source test |
| Session, credential, and verifier rows are atomic | forced token-collision rollback test |
| Issued secrets are not copyable and are wiped | move-only static assertions and `clearSecrets()` test |
| Issued material authenticates through the staged verifier | issuance-to-authenticator positive test with CSRF |
| Browser runtime is not overstated | no TestHttpServer/Gate wiring and explicit staged-boundary guard |
| Daemon and packaging link the new service | CI run 6367 |

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Phase 62 remains open. The next strict Slice 2 increment is HTTP login/logout and secure cookie response construction, followed by authentication precedence, persistent resolver/Gate integration, and real CSRF enforcement before router dispatch.
