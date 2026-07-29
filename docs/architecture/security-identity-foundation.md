# Security and Identity Foundation

Status: Phase 62 Slice 1 plus real-runtime-accepted lifecycle, managed Basic, browser-session issuance/logout and ordinary-route browser authentication; persisted actor-grant loading is repository-validated and awaiting separate installed-runtime acceptance

## Active runtime boundary

The server owns authentication, lifecycle resolution, authorization and accountability before API dispatch.

```text
HttpServerRequest
  -> BrowserSessionHttpGate for exactly two POST lifecycle routes
       -> Basic login exchange or Cookie + CSRF logout
       -> PersistentIdentityResolver
       -> lifecycle authorization accountability
       -> BrowserSessionHttpService

  -> SecurityHttpGate for every other request
       -> BrowserSessionAuthenticator when a session cookie is presented
            -> BrowserSessionCredentialRepository
            -> SecurityPermissionGrantRepository
       -> otherwise LegacyBasicAuthenticator (transitional)
       -> otherwise optional ManagedBasicAuthenticator
            -> CredentialVerifierRepository
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
  -> controller/service/domain safety checks
```

The dedicated lifecycle gate remains intentionally narrow and owns only issue/logout. Ordinary application requests use the general gate, where a presented browser credential has strict precedence over Basic and invalid, expired or revoked cookies never fall back to another authentication mechanism.

## Identity model

`RequestSecurityContext` carries:

- authentication state;
- actor identity and actor type;
- optional device identity;
- optional session identity;
- optional credential identity;
- permission grants;
- request ID;
- optional correlation ID.

Actor types can represent users, services, agents and system work. Legacy Basic, managed Basic and browser-session verification all construct the same context model.

## Persistent lifecycle model

`SecurityIdentityRepository` owns:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`.

`SecurityIdentityProvisioningRepository` owns idempotent separately configured identity bootstrap and exact metadata validation. Startup never silently reactivates revoked state.

`PersistentIdentityResolver` verifies that:

- actor exists and is active;
- device belongs to actor and is active;
- credential belongs to actor and is active, unexpired and unrevoked;
- session belongs to actor/device and is active, unexpired and unrevoked.

Missing or mismatched records fail closed as invalid credentials.

## Managed Basic verifier

`CredentialVerifierRepository` owns `security_basic_credential_verifiers`, binding one unique login name to one credential ID and one modular password hash.

`ManagedBasicAuthenticator`:

1. parses Basic credentials strictly;
2. applies bounded username/password lengths;
3. retrieves the persisted verifier;
4. verifies via thread-safe `crypt_r`;
5. compares the modular result in constant time;
6. creates the configured actor/device/session/credential context;
7. passes it through persistent lifecycle resolution.

Accepted hashes:

- yescrypt (`$y$`), preferred for managed passwords;
- SHA-512 crypt (`$6$`), transitional interoperable format.

Managed password generation and protected rotation remain open.

## Browser-session verifier

`BrowserSessionCredentialRepository` owns `security_browser_session_credentials`. Each row binds:

- non-secret token ID;
- actor ID;
- device ID;
- session ID;
- browser-session credential ID;
- issuing credential ID;
- modular session-secret hash;
- independent modular CSRF-secret hash;
- active, expiry, revocation, creation and update state.

Cookie value:

```text
<token-id>.<high-entropy-session-secret>
```

Only the token ID is stored directly. `BrowserSessionAuthenticator`:

1. finds one bounded `vdr_suite_session` cookie;
2. rejects malformed members and duplicate target cookies;
3. parses restricted token/secret alphabets;
4. retrieves the verifier by token ID;
5. verifies the session secret with `crypt_r`;
6. distinguishes invalid, expired, revoked and authenticated states;
7. constructs actor/device/session/credential context values;
8. resolves active actor grants through `SecurityPermissionGrantRepository`;
9. records whether grant resolution succeeded or was unavailable;
10. verifies `X-CSRF-Token` independently only for an active valid session.

## Persisted browser permission grants

`SecurityPermissionGrantRepository` owns the additive table
`security_actor_permission_grants`.

Each grant is keyed by:

```text
actor_id + permission + backend_id
```

Rows carry active, revocation, creation and update state. Browser authentication
loads only active, unrevoked grants for the authenticated actor.

The repository distinguishes two valid outcomes:

```text
Resolved with zero grants
  -> authentication remains valid;
  -> the actor has no browser permissions.

Unavailable
  -> authentication identity remains attributable;
  -> ordinary access fails closed with
     503 permission_grants_unavailable.
```

Browser sessions do not copy:

- legacy compatibility grants;
- managed-Basic configuration grants;
- rights from the credential that issued the browser session;
- implicit wildcard grants.

Legacy and managed Basic retain their existing explicitly configured grants.
No role model, protected grant-administration API or automatic default grant is
introduced by this increment.

## Browser-session issuance

`BrowserSessionIssuanceService` owns secret generation and transaction orchestration. It does not own HTTP response construction.

### Generated material

- token ID: 128 random bits, `bst_` plus lower-case hexadecimal;
- session ID: 128 random bits, `bss_` plus lower-case hexadecimal;
- credential ID: 128 random bits, `bsc_` plus lower-case hexadecimal;
- session secret: 256 random bits, unpadded Base64url;
- CSRF secret: independent 256 random bits, unpadded Base64url;
- session and CSRF salts: independent 16-character crypt salts.

Entropy comes from Linux `getrandom(2)` with full-read and `EINTR` handling. Failure to obtain complete entropy fails issuance.

Session and CSRF verifiers use SHA-512 crypt with `rounds=10000`. These secrets are machine-generated 256-bit values, not user passwords.

### Lifetime

- minimum: 5 minutes;
- default: 8 hours;
- maximum: 24 hours.

The same UTC expiry is assigned to the session identity, browser credential identity and browser verifier record.

### Transaction

The service generates and hashes material before holding the database write lock. It then acquires the transaction lease and starts `BEGIN IMMEDIATE`.

Inside that transaction it revalidates:

- active, unrevoked actor;
- active, unrevoked device owned by that actor;
- active, unexpired, unrevoked issuing credential owned by that actor.

It then inserts:

1. `security_sessions` row;
2. `security_credentials` row with type `browser-session` and issuing credential as rotation predecessor;
3. `security_browser_session_credentials` row with lookup data and one-way hashes.

All rows commit together. Validation failure, identifier conflict, repository failure or commit failure rolls back the transaction.

### Result ownership

`IssuedBrowserSession` exposes IDs, sensitive cookie value, sensitive CSRF token and expiry only to the HTTP service. It is move-only. Destruction and `clearSecrets()` overwrite the sensitive string buffers. The result must not be persisted or logged.

## Browser-session HTTP lifecycle

### Login exchange

`POST /api/security/browser-sessions` is handled before the general application gate.

- accepts legacy or managed Basic authentication;
- resolves persistent lifecycle before issuance;
- creates a default eight-hour browser session;
- returns `200` with `csrfToken`, `expiresAt` and `requestId` only;
- sets `Cache-Control: no-store` and `Pragma: no-cache`;
- emits the session secret only as `Set-Cookie`;
- cookie attributes: `Path=/`, `Max-Age=28800`, `HttpOnly`, `Secure`, `SameSite=Strict`;
- omits `Domain`;
- never returns the cookie value, session ID or credential ID in JSON.

This is a credential exchange from an already authenticated Basic context. It is not a JSON password-login mechanism.

### Logout

`POST /api/security/browser-sessions/logout` accepts only a valid browser cookie plus the matching `X-CSRF-Token`.

- Basic credentials do not replace the browser cookie;
- missing or wrong CSRF returns `403 csrf_validation_failed`;
- persistent lifecycle resolution occurs before mutation;
- `BrowserSessionLifecycleService` revokes verifier, canonical session and canonical browser credential in one `BEGIN IMMEDIATE` transaction;
- success returns `204` and an expired hardened cookie.

### Route ownership and mutation boundary

`BrowserSessionHttpGate` still handles exactly the two lifecycle POST paths.

`SecurityHttpGate` owns browser authentication for every ordinary application
request. A valid browser session can access ordinary read routes. A presented
browser cookie always takes precedence; malformed, duplicate, unknown, expired
or revoked browser credentials do not fall back to legacy or managed Basic.

Every browser-authenticated POST remains fail-closed with
`503 security_policy_not_migrated`. Persisted grants therefore do not yet enable
Remote, Timer, Recording, SearchTimer or administrative mutation. Route
permission mapping and applicable CSRF verification must be implemented
explicitly before dispatch.

### Installed HTTPS acceptance

The isolated lifecycle was accepted on the real yaVDR installation on 2026-07-28:

- anonymous issuance: `401 authentication_required`;
- authenticated issuance: `200` with one-time CSRF, expiry and request ID;
- hardened cookie: `Path=/`, `HttpOnly`, `Secure`, `SameSite=Strict`;
- logout without CSRF: `403 csrf_validation_failed`;
- valid logout: `204`;
- revoked-cookie replay: `401 credential_revoked`;
- verifier, session and browser credential revocation remained internally consistent;
- SQLite integrity and foreign-key checks passed;
- append-only accountability recorded the allow and deny chain.

The reverse-proxy route is a local runtime integration. This acceptance did not run yaVDR-Ansible or alter its repository.

## SQLite ownership

Direct SQLite calls remain limited to infrastructure, approved repository implementation units, the registered split repository family and registered SQLite/schema contract tests.

- public Security repository headers contain no `sqlite3_*` calls;
- issuance and lifecycle services use `Database` and repository abstractions only;
- `SecurityIdentityIssuanceRepository.cpp` owns issuance-specific session/credential SQL;
- `BrowserSessionCredentialRepository.cpp` owns browser-verifier SQL;
- `SecurityPermissionGrantRepository.cpp` owns actor permission-grant SQL;
- identity, browser and grant repositories own their lifecycle updates;
- `Database::filename()` owns database-path discovery for Global Search;
- SQLite progress-handler shutdown cancellation is owned by `core/sqlite`.

## Authorization and compatibility

Permission grants contain permission name and backend scope. The first migrated application mutation requires:

```text
remote.control@<backend-id>
```

### `legacy-basic`

- the exact legacy credential retains the temporary compatibility bypass for unmigrated application POST routes;
- migrated application routes are authorized and audited;
- optional managed identities do not inherit the bypass;
- the login exchange may use the authenticated legacy identity;
- browser logout is independent of the legacy bypass.

### `enforced`

- anonymous GET can reach existing read routes;
- presented invalid/expired/revoked credentials are rejected rather than downgraded;
- migrated Remote requires active identity, permission and backend scope;
- other application POST routes return `security_policy_not_migrated` before dispatch;
- the isolated browser lifecycle routes retain their own exact Basic or Cookie+CSRF requirements.

### Browser sessions

- a presented cookie has precedence over Basic;
- active actor grants are loaded from `security_actor_permission_grants`;
- a successfully resolved empty grant set is valid and grants no permission;
- unavailable grant persistence returns `503 permission_grants_unavailable`;
- browser sessions inherit no compatibility or managed-Basic grant;
- browser-authenticated business POSTs remain blocked until route CSRF migration.

## Security errors

Runtime errors include:

- `authentication_required`;
- `invalid_credentials`;
- `credential_expired`;
- `credential_revoked`;
- `session_expired`;
- `session_revoked`;
- `actor_revoked`;
- `device_revoked`;
- `permission_denied`;
- `permission_grants_unavailable`;
- `backend_scope_denied`;
- `invalid_backend_scope`;
- `accountability_unavailable`;
- `security_policy_not_migrated`;
- `security_runtime_unavailable`;
- `invalid_session_issuance_context`;
- `browser_session_issuance_failed`;
- `invalid_browser_session_context`;
- `browser_session_revocation_failed`;
- `csrf_validation_failed`.

Security and lifecycle responses use `Cache-Control: no-store` and never include submitted credentials.

## Accountability

`AccountabilityEventRepository` owns append-only pre-dispatch authorization records. Database triggers reject update and delete.

The general Remote path records actor, device, session, authentication state, permission, backend, operation, request, correlation, decision, reason and dispatch outcome.

The dedicated lifecycle gate records pre-dispatch allow/deny decisions for `session.issue.self`, `session.revoke.self`, authentication failures and CSRF failures. Complete issuance/revocation completion events, transactional coupling to lifecycle mutation, protected audit reads and retention remain open.

## Secret handling

Persisted security data is limited to lifecycle metadata and one-way verifiers.

The following are not persisted or reflected:

- complete Authorization header;
- decoded or plaintext password;
- generated managed password;
- complete Cookie header;
- complete browser cookie value;
- raw session secret;
- raw CSRF token;
- reversible credential.

The HTTP login response delivers the cookie through `Set-Cookie` and the CSRF value once through no-store JSON. Future frontend integration must retain CSRF only in memory and must not place either secret in URLs, logs or durable client storage.

## Test boundary

The browser verifier test covers parsing, verification, CSRF, expiry,
revocation, persisted grant loading, a successfully resolved empty grant set and
grant-store unavailability.

The permission-grant repository test covers additive/idempotent schema creation,
actor separation, backend scopes, revocation, reactivation, empty results and
unavailable persistence.

The issuance test covers entropy, identifiers, hashes, lifecycle rows, rollback,
lifetime bounds, revoked source credentials, persisted grant consumption and
secret wipe.

The HTTP service and dedicated gate tests cover:

- secure cookie attributes and no Domain;
- no cookie reflection in JSON;
- one-time CSRF response;
- invalid Basic login denial;
- exact route matching;
- missing/wrong CSRF denial;
- successful cookie+CSRF logout;
- atomic verifier/session/credential revocation;
- revoked post-logout authentication;
- ordinary-route cookie isolation;
- append-only lifecycle authorization decisions.

Passing tests and packaging do not replace installed acceptance for later increments or complete general cookie-authenticated application routing. The isolated HTTPS login/logout lifecycle itself is installed and real-runtime accepted.

## Remaining authentication boundary

Still open within Phase 62:

- separately approved installed-runtime acceptance of persisted browser grants;
- permission mapping and safe/mutating classification for every business POST;
- actual CSRF rejection before every applicable business mutation;
- frontend login/logout and in-memory CSRF handling;
- completion/outcome accountability and transactional outbox;
- idle timeout, refresh, cleanup, concurrent-session policy and recovery;
- protected managed/native/service credential lifecycle administration;
- persisted roles, assignments, grants and resource scopes;
- migration of all protected routes away from the compatibility bypass.

These capabilities must consume this repository-owned boundary rather than create a second authentication stack.

## Phase boundaries

This foundation may be consumed by future browser, native, TV, agent and public API clients. It does not implement those clients and does not advance Phase 63-67 runtime.
