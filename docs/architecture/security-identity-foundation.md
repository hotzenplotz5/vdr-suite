# Security and Identity Foundation

Status: Phase 62 Slice 1; real-runtime-accepted lifecycle and managed Basic increments from Slice 2; staged browser-session credential/cookie/CSRF verifier foundation; incomplete Phase 62

## Current runtime boundary

The active Phase 62 security boundary is server-side and precedes API dispatch.

```text
HttpServerRequest
  -> SecurityHttpGate
       -> LegacyBasicAuthenticator (transitional adapter)
       -> optional ManagedBasicAuthenticator
            -> CredentialVerifierRepository
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
  -> existing controller/service/domain safety checks
```

Authentication answers who or what presented credentials. Persistent identity resolution answers whether that actor, device, credential, and session still exist and remain active. Authorization answers whether that actor may perform an action on a backend. Backend access policy independently answers whether the backend accepts writes. Capability checks independently answer whether the backend can perform the action. None of these decisions replaces another.

## Staged browser-session verifier boundary

The branch also contains an implemented and tested browser-session verifier foundation that is not yet connected to `TestHttpServer` or `SecurityHttpGate`:

```text
future Cookie request
  -> BrowserSessionAuthenticator
       -> strict vdr_suite_session parsing
       -> token-id lookup
       -> BrowserSessionCredentialRepository
            -> security_browser_session_credentials
       -> crypt_r verification of session secret
       -> independent X-CSRF-Token verification
  -> future PersistentIdentityResolver
  -> future SecurityHttpGate integration
```

This staged component changes no current request behavior. It does not issue a cookie, define login/logout HTTP routes, set cookie attributes, choose authentication precedence, enforce CSRF on real mutations, or write accountability events through the Gate.

## Identity model

`RequestSecurityContext` carries:

- authentication state;
- actor identity and type;
- optional device identity;
- optional session identity;
- optional credential identity;
- permission grants;
- request ID;
- optional correlation ID.

Actor types can represent users, services, agents, and system work. Runtime can authenticate the legacy local actor and one optional separately configured managed Basic actor. The staged browser-session verifier can construct the same context shape from a valid bound session record. This remains an incremental authentication foundation, not the final browser/native/public authentication contract.

## Persistent lifecycle model

`SecurityIdentityRepository` owns additive SQLite persistence for:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`.

`SecurityIdentityProvisioningRepository` owns idempotent creation and binding validation for separately provisioned identities. It uses `INSERT OR IGNORE` and verifies the resulting metadata, so startup neither overwrites an existing identity nor silently reactivates lifecycle state.

`PersistentIdentityResolver` verifies on each currently authenticated Basic request that:

- the actor exists and remains active;
- the device belongs to the actor and remains active;
- the credential belongs to the actor and is neither expired nor revoked;
- the session belongs to the actor and device and is neither expired nor revoked.

Missing or mismatched records fail closed as invalid credentials. Lifecycle state is never trusted solely from request input.

Future browser-cookie integration must pass the browser context through the same resolver rather than treating the browser verifier row as a replacement for actor/device/session/credential lifecycle state.

## Managed Basic credential verifier model

`CredentialVerifierRepository` owns the `security_basic_credential_verifiers` table. Each row binds:

- one unique login name;
- one credential ID;
- one modular one-way password hash.

`ManagedBasicAuthenticator`:

1. parses the Basic scheme and Base64 payload strictly;
2. applies bounded username and password lengths;
3. rejects malformed input and unsupported hash formats;
4. retrieves the verifier by login name;
5. verifies the submitted password using thread-safe `crypt_r`;
6. compares the modular crypt result in constant time;
7. emits the configured managed actor/device/session/credential context;
8. passes that context through persistent lifecycle resolution.

Accepted verifier formats are:

- yescrypt (`$y$`), preferred for managed deployments;
- SHA-512 crypt (`$6$`), accepted as a transitional interoperable format.

The server does not generate managed passwords or hashes in this increment. It also does not silently replace an existing login/hash binding at daemon restart; protected credential rotation remains later Slice 2 work.

## Browser-session credential and verifier model

`BrowserSessionCredentialRepository` owns the additive `security_browser_session_credentials` table. Each row binds:

- a non-secret token identifier used for bounded lookup;
- actor ID;
- device ID;
- session ID;
- browser-session credential ID;
- the credential ID from which the browser session was issued;
- a modular one-way session-secret hash;
- a separate modular one-way CSRF-secret hash;
- active, expiry, revocation, creation, and update state.

The conceptual cookie value is:

```text
<token-id>.<high-entropy-session-secret>
```

Only the token ID is stored directly. The complete cookie value and raw secret are not persisted. `BrowserSessionAuthenticator`:

1. finds the configured `vdr_suite_session` cookie in a bounded Cookie header;
2. rejects malformed cookie members and duplicate target cookies;
3. parses one restricted token ID and one restricted high-entropy secret;
4. retrieves the bound verifier row by token ID;
5. verifies the session secret using thread-safe `crypt_r` and constant-time comparison;
6. distinguishes invalid, expired, and revoked states;
7. constructs actor/device/session/credential context values and configured grants;
8. can independently verify `X-CSRF-Token` against the session-bound CSRF hash only while the browser-session row is active, unexpired, and unrevoked.

The same yescrypt and SHA-512 crypt verifier formats are accepted for the staged session and CSRF hashes. Protected issuance must generate cryptographically strong values server-side, create identity lifecycle and browser verifier rows atomically, and return the CSRF value only to the authenticated browser.

## Authorization model

A permission grant contains:

- permission name;
- backend scope, either an exact backend ID or `*`.

The first protected permission is:

```text
remote.control@<backend-id>
```

Current runtime authorization is fail-closed for:

- anonymous or invalid authentication where authentication is required;
- expired or revoked credential/session state;
- inactive or revoked actor or device;
- missing permission;
- wrong backend scope;
- missing backend context.

The remote mutation is evaluated before `ApiRouter::handleClientPost`. The existing `RemoteActionService` continues to enforce request validity, operation ID, action allowlist, backend existence, backend read-only state, capability, and executor availability.

For future browser-cookie mutations, CSRF verification is a separate mandatory decision before actor authorization and router dispatch. The staged verifier proves the primitive only; the Gate does not enforce it yet.

## Compatibility modes

### `legacy-basic`

This is the current default migration mode.

- Every request still requires runtime authentication.
- The configured legacy credential maps to the legacy actor/device/session/credential context.
- The legacy credential alone retains the old compatibility bypass for not-yet-migrated POST routes.
- The default legacy grant remains `*@*` only to prevent an unannounced local browser outage.
- Authorization decisions for migrated routes are still evaluated and audited.
- An optional managed Basic identity may authenticate alongside the legacy browser.
- Managed identities do not inherit the compatibility bypass.
- The staged browser-session verifier is not called and changes no compatibility behavior.

A managed identity using an unmigrated POST route receives `security_policy_not_migrated` before router dispatch. This lets Phase 62 test separate users safely while preserving the existing browser until route migration is complete.

### `enforced`

- Anonymous GET requests can reach existing read routes.
- A request that presents invalid, expired, or revoked current runtime credentials is rejected rather than downgraded to anonymous.
- The migrated remote mutation requires authentication, active persisted identity state, permission, and backend scope.
- Other POST routes return `security_policy_not_migrated` before router dispatch.
- No embedded legacy credential or grant is active unless explicitly configured for the enforced rollout.
- Browser-cookie behavior is not yet part of this mode.

## Managed configuration

No managed Basic identity exists by default. Configuration is enabled only when both are present:

- `VDR_SUITE_MANAGED_BASIC_USERNAME`;
- `VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH`.

Optional identity configuration:

- `VDR_SUITE_MANAGED_BASIC_ACTOR_ID`;
- `VDR_SUITE_MANAGED_BASIC_ACTOR_DISPLAY_NAME`;
- `VDR_SUITE_MANAGED_BASIC_DEVICE_ID`;
- `VDR_SUITE_MANAGED_BASIC_SESSION_ID`;
- `VDR_SUITE_MANAGED_BASIC_CREDENTIAL_ID`;
- `VDR_SUITE_MANAGED_BASIC_PERMISSIONS`.

Managed permissions default to empty. Partial configuration or an unsupported hash format prevents the security runtime from becoming ready.

The staged browser-session classes currently default to cookie name `vdr_suite_session` and CSRF header name `X-CSRF-Token`. These are internal branch contracts until the HTTP issuer, cookie deployment policy, and Gate integration are implemented. They are not a stable Phase 67 public API.

## Security errors

Phase 62 runtime security errors use a nested machine-readable error with request ID and `Cache-Control: no-store`.

Current runtime codes include:

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

Security errors never include the submitted credential. Cookie/login/logout/CSRF-specific runtime error codes are not yet implemented. Stable public error compatibility remains Phase 67.

## Accountability

`AccountabilityEventRepository` creates an append-only `accountability_events` table and rejects update/delete statements with database triggers.

For the first protected mutation it stores the pre-dispatch decision with actor, device, session, authentication state, permission, backend, operation, request, correlation, reason, and dispatch-authorization outcome.

An allowed remote mutation is not dispatched when the required decision row cannot be appended. Denials for invalid managed credentials and unmigrated managed POST requests are also recorded before rejection.

The staged browser verifier does not yet append authentication, CSRF, issuance, logout, expiry, or revocation events. Those must be added with HTTP/Gate integration; no staged unit test is treated as runtime accountability evidence.

This is not yet ADR-0049 completion. Authentication lifecycle events, mutation completion events, transactional outbox semantics, protected audit reads, retention, export, integrity chaining, and the full event catalogue remain open.

## Storage and secret handling

The security database path is resolved in this order:

1. `VDR_SUITE_SECURITY_DATABASE_PATH`;
2. `VDR_SUITE_DATABASE_PATH`;
3. `/tmp/vdr-suite-test.db`.

The HTTP server owns a dedicated SQLite connection and explicit repository, authenticator, resolver, and gate lifetimes. There is no global security singleton.

Stored credential information is limited to identity/lifecycle metadata and dedicated one-way verifier records:

- managed Basic login-to-credential and password hash;
- staged browser token ID, identity bindings, and separate session/CSRF hashes.

The complete Authorization header, decoded password, plaintext password, generated password, complete cookie value, submitted Cookie header, raw session secret, raw CSRF value, and reversible secret are not persisted or reflected in errors/accountability. Architecture guards forbid raw browser secret storage fields in the browser-session repository.

## Test boundary

The staged browser-session test covers:

- schema creation and identity bindings;
- duplicate and unsupported-hash rejection;
- proof that raw session/CSRF values are not stored in verifier columns;
- anonymous and successful cookie authentication;
- correct, missing, and wrong CSRF secrets;
- wrong secret, unknown token, malformed token, and duplicate target cookie;
- expiry and persistent revocation.

Passing this test validates the repository and verifier foundation. It does not validate browser login, HTTP response headers, Gate authentication precedence, real CSRF rejection, logout, audit events, or installed yaVDR behavior.

## Remaining authentication boundary

Still open within Phase 62 are:

- secure random generation and atomic browser-session/credential issuance;
- HTTP login and logout routes;
- documented `Set-Cookie` attributes and deployment behavior for HTTP versus HTTPS;
- browser-cookie authentication precedence and `PersistentIdentityResolver`/`SecurityHttpGate` integration;
- actual CSRF rejection before browser-authenticated mutation dispatch;
- refresh, idle timeout, absolute expiry, cleanup, concurrent-session policy, and atomic logout/revocation;
- secure managed password-hash generation and protected credential issuance/change/recovery;
- service/native credential enrollment, rotation, refresh, recovery, and device trust;
- protected lifecycle administration;
- persisted roles, grants, and backend/resource scopes;
- migration of all protected routes away from the legacy bypass.

These capabilities must consume the repository-owned boundary rather than create a second authentication stack.

## Phase boundaries

This foundation may be consumed by future browser, native, TV, agent, and public API clients. It does not implement those clients and does not advance Phase 63-67 runtime.
