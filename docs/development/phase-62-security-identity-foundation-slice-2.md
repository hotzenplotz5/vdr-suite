# Phase 62 Persistent Identity Lifecycle — Slice 2

Status: persistence/revocation and managed Basic increments are real-VDR accepted; browser-session credential/verifier foundation is implemented and CI validated but not yet wired into the HTTP runtime; Phase 62 remains open

## Purpose

Phase 62 Slice 1 established an explicit request identity, centralized authorization, the first protected mutation, and append-only pre-dispatch accountability. Slice 2 removes the next critical limitation: authenticated identity state is no longer only transient configuration assembled independently for every request.

The slice now contains three cumulative increments:

1. persistent lifecycle state for actors, devices, sessions, and credential metadata;
2. an optional separately provisioned managed Basic identity whose submitted password is verified against a persisted one-way password hash;
3. a persistent browser-session credential and verifier foundation with strict cookie parsing, one-way session/CSRF secret hashes, expiry, and revocation semantics.

The managed Basic mechanism remains an intermediate verifier. The browser-session foundation establishes storage and cryptographic verification contracts, but it does not yet issue cookies, expose login/logout routes, or participate in `SecurityHttpGate` request authentication.

## Current runtime boundary

```text
HttpServerRequest
  -> LegacyBasicAuthenticator (transitional browser compatibility)
  -> optional ManagedBasicAuthenticator
       -> strict Basic decoding
       -> CredentialVerifierRepository
            -> security_basic_credential_verifiers
            -> crypt_r verification of yescrypt or SHA-512 crypt hash
  -> PersistentIdentityResolver
       -> SecurityIdentityRepository
            -> security_actors
            -> security_devices
            -> security_sessions
            -> security_credentials
  -> SecurityHttpGate
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
```

A successful credential check establishes a transient request context. The persistent resolver then replaces all lifecycle assumptions with database state before authorization is evaluated.

## Staged browser-session boundary

The following foundation exists on the branch but is deliberately not yet connected to `TestHttpServer` or `SecurityHttpGate`:

```text
future Cookie request
  -> BrowserSessionAuthenticator
       -> strict Cookie parsing
       -> token-id lookup
       -> crypt_r verification of session secret
       -> BrowserSessionCredentialRepository
            -> security_browser_session_credentials
       -> X-CSRF-Token verification for mutation requests
  -> future PersistentIdentityResolver and SecurityHttpGate integration
```

This separation keeps issuance, cookie attributes, request authentication precedence, CSRF enforcement, logout, and accountability as explicit follow-up work rather than silently changing the existing browser runtime.

## Implemented scope

- persistent actor records with actor type, display name, active state, and revocation timestamp;
- persistent device records bound to an actor;
- persistent session records bound to an actor and device, with expiry and revocation state;
- persistent credential metadata records bound to an actor, with credential type, expiry, revocation, and rotation predecessor metadata;
- deterministic compatibility identity bootstrap using `INSERT OR IGNORE`;
- preservation of existing revocation state across daemon restarts and repeated bootstrap attempts;
- request-time actor, device, credential, and session lookup for authenticated requests;
- fail-closed rejection when a persisted identity is missing, belongs to another actor/device, is inactive, expired, or revoked;
- explicit `credential_expired` and `credential_revoked` authorization errors;
- configurable compatibility credential identifier through `VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID`;
- optional managed actor/device/session/credential provisioning without reactivating existing records;
- strict Basic Authorization parsing with bounded username/password sizes and malformed-input rejection;
- password verification through thread-safe `crypt_r`;
- accepted modular password-hash formats limited to yescrypt (`$y$`) and SHA-512 crypt (`$6$`);
- persisted verifier binding from login name to credential ID and one-way password hash;
- constant-time comparison of verifier results;
- rejection of partial managed configuration and unsupported hash formats at security-runtime startup;
- additive `security_browser_session_credentials` table linked to actor, device, session, browser credential, and issuing credential identities;
- separate token identifier plus one-way modular hashes for the session secret and CSRF secret;
- mandatory bounded browser-session expiry and persistent active/revoked state;
- strict bounded cookie parsing with duplicate-session-cookie rejection;
- session-cookie verification without storing or reflecting the submitted cookie value;
- `X-CSRF-Token` verification against the session-bound one-way CSRF hash;
- browser-session anonymous, valid, wrong-secret, unknown-token, malformed-token, duplicate-cookie, wrong-CSRF, expired, and revoked tests;
- repository, provisioning, verifier, resolver, authorization, configuration, HTTP-gate, and architecture tests.

## Storage contract

The security database components can create these additive tables when their owning repositories are initialized:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`;
- `security_basic_credential_verifiers`;
- `security_browser_session_credentials`.

The first four tables contain identity and lifecycle metadata. The Basic verifier table contains credential ID, unique login name, one-way modular password hash, and timestamps.

The browser-session credential table contains:

- a non-secret token identifier used for bounded lookup;
- session, actor, device, browser credential, and issuing credential bindings;
- a one-way modular hash of the high-entropy session secret;
- a separate one-way modular hash of the CSRF secret;
- active, expiry, revocation, and timestamp fields.

It does not contain the complete cookie value, raw session secret, raw CSRF token, submitted Cookie header, Authorization header, password, or reversible secret.

The existing security database path resolution remains:

1. `VDR_SUITE_SECURITY_DATABASE_PATH`;
2. `VDR_SUITE_DATABASE_PATH`;
3. `/tmp/vdr-suite-test.db`.

## Compatibility identity

The default local browser remains mapped to:

```text
actor:      legacy-local-web
device:     legacy-browser
session:    legacy-basic-session
credential: legacy-basic-credential
```

`INSERT OR IGNORE` remains intentional: restarting the daemon must not reactivate a record already expired, disabled, or revoked in SQLite.

## Optional managed Basic identity

No managed identity exists by default. It is enabled only when both of these are configured:

- `VDR_SUITE_MANAGED_BASIC_USERNAME`;
- `VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH`.

Optional identity and permission configuration:

- `VDR_SUITE_MANAGED_BASIC_ACTOR_ID`;
- `VDR_SUITE_MANAGED_BASIC_ACTOR_DISPLAY_NAME`;
- `VDR_SUITE_MANAGED_BASIC_DEVICE_ID`;
- `VDR_SUITE_MANAGED_BASIC_SESSION_ID`;
- `VDR_SUITE_MANAGED_BASIC_CREDENTIAL_ID`;
- `VDR_SUITE_MANAGED_BASIC_PERMISSIONS`.

Managed permissions default to empty. They use the existing comma-separated `permission@backend` syntax.

The configured hash is a modular crypt value, not a plaintext password. Yescrypt and SHA-512 crypt are accepted by the managed verifier.

## Browser-session credential/verifier foundation

A future browser cookie has the conceptual value:

```text
<token-id>.<high-entropy-session-secret>
```

Only `<token-id>` is stored directly. The secret is verified against `session_secret_hash`. Mutation requests authenticated by this mechanism will additionally require an independent high-entropy value in `X-CSRF-Token`, verified against `csrf_secret_hash`.

Current parser and verifier rules:

- cookie name defaults to `vdr_suite_session`;
- cookie header length is bounded;
- the target cookie must appear at most once;
- token ID and secret use a restricted cookie-safe alphabet;
- unknown, malformed, duplicate, or wrong-secret cookies fail as invalid;
- a valid secret bound to an inactive/revoked record returns revoked state;
- a valid secret bound to an expired record returns expired state;
- CSRF succeeds only when the session cookie is valid, active, unexpired, unrevoked, and the independent header secret verifies.

The foundation intentionally does not define the final external authentication API. Protected issuance must generate cryptographically strong random values server-side, create the `security_sessions` and `security_credentials` lifecycle rows atomically with the verifier row, set documented cookie attributes, and return the CSRF value only to the authenticated browser.

## Route-safety rule

The legacy compatibility bypass belongs only to the configured legacy actor and legacy credential.

A managed Basic identity may:

- access authenticated GET routes in `legacy-basic` mode;
- use `POST /api/vdr/remote/actions` only with `remote.control` for the requested backend;
- use migrated routes added later only with their explicit permission.

A managed identity may not use the legacy bypass for an unmigrated POST route. Such a request returns `503 security_policy_not_migrated` before router dispatch, even while the existing legacy browser remains compatible.

The staged browser-session verifier does not change this runtime behavior until explicit HTTP integration and CSRF enforcement are implemented and tested.

In `enforced` mode anonymous GET remains possible, but a request that presents invalid, expired, or revoked credentials is rejected with 401 instead of being treated as anonymous.

## Revocation semantics

The current request path resolves persisted identity state on every authenticated Basic request. The following states deny dispatch before the API router:

- inactive or revoked actor: `actor_revoked`;
- inactive or revoked device: `device_revoked`;
- inactive or revoked credential: `credential_revoked`;
- expired credential: `credential_expired`;
- inactive or revoked session: `session_revoked`;
- expired session: `session_expired`;
- missing or mismatched persisted binding: `invalid_credentials`.

The browser-session verifier independently enforces its own active, expiry, and revocation fields before future gate integration. The integration increment must also pass the resulting actor/device/session/credential through `PersistentIdentityResolver`, preserving cumulative lifecycle enforcement.

Backend permission, backend scope, backend read-only state, capability, operation validation, and domain execution remain separate cumulative decisions.

## Real-VDR acceptance of the persistence/revocation foundation

The earlier Slice 2 lifecycle head was installed on the yaVDR system and used the production Suite database at `/var/lib/vdr-suite/vdr-suite.db`.

Observed on 2026-07-28:

- daemon startup created one active compatibility actor, device, session, and credential with the expected ownership bindings;
- the stored lifecycle records contained no Basic Authorization value, password, token, or reversible secret;
- setting `legacy-basic-credential.active=0` and assigning `revoked_at` immediately blocked browser authentication without restarting the daemon;
- accountability recorded `actor_id=legacy-local-web`, authentication state `revoked`, reason `credential_revoked`, decision `denied`, and outcome `dispatch_denied`;
- parallel denial rows represented browser document, asset, and API requests, not repeated VDR action dispatches;
- restoring `active=1` and clearing expiry/revocation restored browser access without restarting the daemon;
- the following Remote request was authorized with `remote.control@default` and recorded as `permission_granted` / `dispatch_authorized`.

This proves durable lifecycle enforcement and recovery.

## Real-VDR acceptance of the managed verifier increment

The managed verifier increment was installed on the same yaVDR runtime and provisioned as a separate identity without replacing or revoking the legacy browser credential.

Observed on 2026-07-28:

- startup provisioned active actor `user-phase62-admin`, device `device-phase62-admin`, session `session-phase62-admin`, and credential `credential-phase62-admin` with correct ownership bindings;
- `security_basic_credential_verifiers` contained login `phase62-admin`, credential binding `credential-phase62-admin`, SHA-512 crypt marker `$6$`, and a 106-character one-way hash; no plaintext password or Authorization header was stored;
- correct credentials returned `200` for `GET /api/backends` and propagated request ID `req-1b802f959d39-1`;
- a wrong password returned `401 invalid_credentials`, `Cache-Control: no-store`, and request ID `req-1b9482d06e6b-2` without reflecting the submitted password;
- accountability attributed that invalid attempt to anonymous/invalid state and recorded `dispatch_denied`;
- correct managed credentials sent to `POST /api/vdr/timers/actions/create` returned `503 security_policy_not_migrated` with request ID `req-1bb054a1d030-4` before router dispatch;
- accountability attributed the blocked Timer mutation to `user-phase62-admin` / `device-phase62-admin` / `session-phase62-admin`, permission `unmapped.mutation`, reason `security_policy_not_migrated`, decision `denied`, and outcome `dispatch_denied`;
- the same managed identity successfully invoked the migrated `POST /api/vdr/remote/actions` operation `phase62-managed-1785212278` with action `up` and received `200` / `Remote action executed`;
- accountability attributed that request to the managed actor/device/session, permission `remote.control`, backend `default`, reason `permission_granted`, decision `allowed`, and outcome `dispatch_authorized`.

This proves real-runtime password verification, independent managed identity attribution, secret-safe failure behavior, separation from the legacy compatibility bypass, backend-scoped authorization, and successful dispatch through the migrated Remote path.

## CI evidence

GitHub Actions VDR-Suite CI run 6247 completed successfully on managed-verifier code head `8c8cd524b8b3a2463d7c9195ac745c06083c0d2d`. Documentation head `d6bc1e0c8f6904afbf41412f07d6adc25549264b` was subsequently validated by CI run 6249.

For the browser-session foundation, CI run 6267 reached a successful fast regression on head `9be3467f6a5148972fdcfbed1b9d26e458c59d8b`, including compilation and execution of `test_browser_session_authenticator.cpp`, before the workflow was superseded and cancelled by subsequent documentation commits. The final complete workflow conclusion is taken only from the latest documentation head.

Browser-session tests cover:

- schema and identity bindings;
- duplicate registration rejection;
- unsupported hash rejection;
- proof that raw session and CSRF values are not stored in verifier columns;
- anonymous request behavior;
- valid cookie authentication and configured grants;
- correct, missing, and wrong CSRF values;
- wrong secret, unknown token, malformed token, and duplicate target cookie;
- expiry and persistent revocation behavior.

## Explicitly not included

- user enrollment or general user-management HTTP routes;
- server-side managed-password hash generation or password-change workflow;
- browser-session HTTP issuance and login response;
- `Set-Cookie` policy and secure-cookie deployment configuration;
- browser-cookie authentication in `SecurityHttpGate`;
- enforced CSRF rejection on actual mutation routes;
- browser logout route and atomic session/credential revocation;
- refresh, idle timeout, expiry cleanup, concurrent-session policy, recovery, or device enrollment;
- native bearer tokens or service credentials;
- protected administrative revoke/reactivate or credential-rotation APIs;
- persisted roles, role assignments, permission grants, or backend scopes;
- migration of additional mutation routes;
- revision, `If-Match`, idempotency-key, operation-lifecycle, or transactional-outbox work;
- Phase 63-67 runtime or client implementation.

## Acceptance evidence

| Criterion | Evidence |
|---|---|
| Compatibility identity is created without storing its credential secret | repository schema/bootstrap tests, architecture guards, real-VDR table inspection |
| Actor/device/session/credential bindings are persisted | identity repository and real-VDR bootstrap evidence |
| Managed identity provisioning preserves existing records and rejects metadata conflicts | `test_managed_basic_authenticator.cpp`; real-VDR managed identity rows |
| Managed verifier stores a one-way hash binding, not a submitted password/header | verifier repository contract, architecture guards, and real-VDR verifier-table inspection |
| Correct managed password authenticates the configured actor/device/session/credential | managed authenticator positive test and real-VDR `GET /api/backends` 200 |
| Wrong, malformed, unsupported, or unknown managed credentials fail closed | managed authenticator/HTTP-gate negatives and real-VDR 401 `invalid_credentials` |
| Managed identity cannot use the legacy unmigrated-POST bypass | compatibility-mode HTTP-gate negative and real-VDR Timer 503 `security_policy_not_migrated` |
| Managed identity can use its backend-scoped migrated permission | real-VDR `remote.control@default` allow and operation `phase62-managed-1785212278` |
| Browser-session verifier stores only token ID and one-way session/CSRF hashes | repository schema, architecture guards, and plaintext-negative test |
| Valid browser cookie resolves actor/device/session/credential context | `test_browser_session_authenticator.cpp` positive path |
| Wrong, unknown, malformed, duplicate, expired, or revoked browser sessions fail closed | browser-session negative matrix |
| CSRF is independently bound to a valid active browser session | correct/missing/wrong CSRF tests |
| Browser-session runtime is not overstated | no TestHttpServer/Gate wiring; explicit staged-boundary documentation |
| Invalid credentials on an enforced-mode GET are rejected rather than downgraded to anonymous | enforced-mode HTTP-gate negative test |
| Existing legacy browser and migrated Remote path remain supported | compatibility tests and real-VDR legacy plus managed Remote evidence |
| Architecture remains server-owned | `tools/check_security_identity_architecture.py` |

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Phase 62 remains open. The next strict Slice 2 increment is atomic browser-session issuance plus HTTP login/logout, documented `Set-Cookie` attributes, authentication precedence, `PersistentIdentityResolver`/`SecurityHttpGate` integration, and actual CSRF enforcement before router dispatch.
