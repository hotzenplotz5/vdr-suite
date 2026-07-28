# Security and Identity Foundation

Status: Phase 62 Slice 1 plus real-runtime-accepted lifecycle/managed-Basic increments and CI-validated staged browser-session verifier/issuance foundations; Phase 62 remains incomplete

## Active runtime boundary

The active security boundary is server-owned and runs before API dispatch.

```text
HttpServerRequest
  -> SecurityHttpGate
       -> LegacyBasicAuthenticator (transitional)
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

Authentication identifies the presented credential. Persistent lifecycle resolution verifies that the actor, device, credential, and session still exist and remain active. Authorization evaluates permission and backend scope. Backend write policy, capability, operation validation, and domain execution remain independent cumulative decisions.

## Staged browser-session boundary

The branch additionally contains two linked and tested components that are not called by the active HTTP request path.

```text
future authenticated login request
  -> BrowserSessionIssuanceService
       -> getrandom CSPRNG
       -> identifier/secret/salt generation
       -> one-way session and CSRF hashes
       -> atomic identity and verifier persistence
       -> one-time move-only result

future Cookie request
  -> BrowserSessionAuthenticator
       -> strict vdr_suite_session parsing
       -> BrowserSessionCredentialRepository
       -> session-secret verification
       -> independent X-CSRF-Token verification
  -> future PersistentIdentityResolver
  -> future SecurityHttpGate integration
```

`TestHttpServer` and `SecurityHttpGate` do not invoke either component. No login/logout route, `Set-Cookie`, browser authentication precedence, real mutation CSRF denial, or browser accountability is active.

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

Actor types can represent users, services, agents, and system work. The active runtime can authenticate the legacy local actor and one optional managed Basic actor. The staged browser verifier constructs the same context shape; it does not create a second identity model.

## Persistent lifecycle model

`SecurityIdentityRepository` owns:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`.

`SecurityIdentityProvisioningRepository` owns idempotent separately configured identity bootstrap and exact metadata validation. Startup never silently reactivates revoked state.

`PersistentIdentityResolver` verifies on every active Basic-authenticated request that:

- actor exists and is active;
- device belongs to actor and is active;
- credential belongs to actor and is active, unexpired, and unrevoked;
- session belongs to actor/device and is active, unexpired, and unrevoked.

Missing or mismatched records fail closed as invalid credentials.

Future browser integration must pass the browser result through this same resolver. The browser verifier row does not replace canonical lifecycle state.

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

Managed password generation and protected rotation are still open.

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
- active, expiry, revocation, creation, and update state.

Conceptual cookie value:

```text
<token-id>.<high-entropy-session-secret>
```

Only the token ID is stored directly. `BrowserSessionAuthenticator`:

1. finds one bounded `vdr_suite_session` cookie;
2. rejects malformed members and duplicate target cookies;
3. parses restricted token/secret alphabets;
4. retrieves the verifier by token ID;
5. verifies the session secret with `crypt_r`;
6. distinguishes invalid, expired, revoked, and authenticated states;
7. constructs actor/device/session/credential context values;
8. verifies `X-CSRF-Token` independently only for an active valid session.

## Browser-session issuance

`BrowserSessionIssuanceService` owns secret generation and transaction orchestration. It does not own HTTP.

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

The same UTC expiry is assigned to the session identity, browser credential identity, and browser verifier record.

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

All rows commit together. Validation failure, identifier conflict, repository failure, or commit failure rolls back the transaction.

### Result ownership

`IssuedBrowserSession` exposes the IDs, sensitive cookie value, sensitive CSRF token, and expiry only to the future authenticated caller.

It is move-only. Copy construction and assignment are disabled. Destruction and `clearSecrets()` overwrite the sensitive string buffers. The result must not be persisted or logged.

## SQLite ownership

Direct SQLite calls remain limited to infrastructure, approved repository implementation units, the existing registered split repository family, and registered SQLite/schema contract tests.

- public Security repository headers contain no `sqlite3_*` calls;
- `BrowserSessionIssuanceService` uses `Database` and repository abstractions only;
- `SecurityIdentityIssuanceRepository.cpp` owns issuance-specific session/credential SQL;
- `BrowserSessionCredentialRepository.cpp` owns browser-verifier SQL;
- `Database::filename()` owns database-path discovery for Global Search;
- SQLite progress-handler shutdown cancellation is owned by `core/sqlite`.

## Authorization and compatibility

Permission grants contain permission name and backend scope. The first migrated mutation requires:

```text
remote.control@<backend-id>
```

### `legacy-basic`

- every request still authenticates;
- the exact legacy credential retains the compatibility bypass for unmigrated POST routes;
- migrated routes are still authorized and audited;
- optional managed identities do not inherit the bypass;
- staged browser components change no request behavior.

### `enforced`

- anonymous GET can reach existing read routes;
- presented invalid/expired/revoked credentials are rejected rather than downgraded;
- migrated Remote requires active identity, permission, and backend scope;
- other POST routes return `security_policy_not_migrated` before dispatch;
- browser-cookie behavior is not yet active.

## Security errors

Current runtime errors include:

- `authentication_required`;
- `invalid_credentials`;
- `credential_expired`;
- `credential_revoked`;
- `session_expired`;
- `session_revoked`;
- `actor_revoked`;
- `device_revoked`;
- `permission_denied`;
- `backend_scope_denied`;
- `invalid_backend_scope`;
- `accountability_unavailable`;
- `security_policy_not_migrated`;
- `security_runtime_unavailable`.

Errors use `Cache-Control: no-store` and never include submitted credentials. Login/logout/cookie/CSRF-specific runtime errors are not implemented.

## Accountability

`AccountabilityEventRepository` owns append-only pre-dispatch authorization records. Database triggers reject update and delete.

The active Remote path records actor, device, session, authentication state, permission, backend, operation, request, correlation, decision, reason, and dispatch outcome.

The staged issuer/authenticator do not yet append issuance, authentication, CSRF, logout, expiry, or revocation events. Those events belong to future HTTP/Gate integration and are required before runtime acceptance.

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

The issuance service temporarily returns the cookie value and CSRF token in a move-only object so a future HTTP layer can deliver them once.

## Test boundary

The browser verifier test covers parsing, verification, CSRF, expiry, and revocation.

The issuance test covers:

- deterministic IDs and UTC expiry;
- generated secret format;
- verifier persistence;
- lifecycle rows;
- successful verifier/CSRF consumption;
- forced token collision after identity inserts and complete rollback;
- lifetime bounds;
- entropy failure;
- revoked issuing credential;
- move-only result and explicit secret wipe.

The architecture contract requires CSPRNG, transaction, rollback, repository ownership, secret wipe, build linkage, and absence of HTTP/Gate wiring.

Passing these tests does not validate HTTP login, response headers, browser authentication precedence, real CSRF rejection, logout, accountability, or installed yaVDR behavior.

## Remaining authentication boundary

Still open within Phase 62:

- HTTP login request parsing and response contract;
- logout and atomic session/credential revocation;
- secure `Set-Cookie` attributes and HTTP/HTTPS deployment behavior;
- browser authentication precedence;
- `PersistentIdentityResolver` and `SecurityHttpGate` integration;
- actual CSRF rejection before mutation dispatch;
- issuance/login/logout/accountability events;
- idle timeout, refresh, cleanup, concurrent-session policy, and recovery;
- protected managed/native/service credential lifecycle administration;
- persisted roles, assignments, grants, and resource scopes;
- migration of all protected routes away from the compatibility bypass.

These capabilities must consume this repository-owned boundary rather than create a second authentication stack.

## Phase boundaries

This foundation may be consumed by future browser, native, TV, agent, and public API clients. It does not implement those clients and does not advance Phase 63-67 runtime.
