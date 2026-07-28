# Phase 62 Persistent Identity Lifecycle — Slice 2

Status: persistence/revocation foundation real-VDR accepted; first managed credential verifier implemented; Phase 62 remains open

## Purpose

Phase 62 Slice 1 established an explicit request identity, centralized authorization, the first protected mutation, and append-only pre-dispatch accountability. Slice 2 removes the next critical limitation: authenticated identity state is no longer only transient configuration assembled independently for every request.

The slice now contains two cumulative increments:

1. persistent lifecycle state for actors, devices, sessions, and credential metadata;
2. an optional separately provisioned managed Basic identity whose submitted password is verified against a persisted one-way password hash.

The managed Basic mechanism is an intermediate verifier for safe multi-identity testing. It is not the final browser cookie/session, native token, enrollment, recovery, or public authentication contract.

## Runtime boundary

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
- accepted password-hash formats limited to yescrypt (`$y$`) and SHA-512 crypt (`$6$`), with yescrypt preferred for real deployments;
- persisted verifier binding from login name to credential ID and one-way password hash;
- constant-time comparison of the verifier result;
- rejection of partial managed configuration and unsupported hash formats at security-runtime startup;
- repository, provisioning, verifier, resolver, authorization, configuration, HTTP-gate, and architecture tests.

## Storage contract

The security database connection creates these tables when absent:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`;
- `security_basic_credential_verifiers`.

The first four tables contain identity and lifecycle metadata. The verifier table contains only:

- credential ID;
- unique login name;
- one-way modular password hash;
- creation and update timestamps.

The runtime does not persist the submitted Basic Authorization value, decoded password, plaintext password, reversible secret, request header, or generated password. Changing an already provisioned login/hash pair is intentionally not performed silently by daemon restart; later protected lifecycle administration owns rotation.

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

The configured hash is a modular crypt value, not a plaintext password. A yescrypt hash is preferred. SHA-512 crypt is accepted as a transitional interoperable verifier format.

## Route-safety rule

The legacy compatibility bypass belongs only to the configured legacy actor and legacy credential.

A managed identity may:

- access authenticated GET routes in `legacy-basic` mode;
- use `POST /api/vdr/remote/actions` only with `remote.control` for the requested backend;
- use migrated routes added later only with their explicit permission.

A managed identity may not use the legacy bypass for an unmigrated POST route. Such a request returns `503 security_policy_not_migrated` before router dispatch, even while the existing legacy browser remains compatible.

In `enforced` mode anonymous GET remains possible, but a request that presents invalid, expired, or revoked credentials is rejected with 401 instead of being treated as anonymous.

## Revocation semantics

The request path resolves persisted state on every authenticated request. The following states deny dispatch before the API router:

- inactive or revoked actor: `actor_revoked`;
- inactive or revoked device: `device_revoked`;
- inactive or revoked credential: `credential_revoked`;
- expired credential: `credential_expired`;
- inactive or revoked session: `session_revoked`;
- expired session: `session_expired`;
- missing or mismatched persisted binding: `invalid_credentials`.

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

This proves durable lifecycle enforcement and recovery. The new managed verifier increment still requires controlled real-VDR provisioning and positive/negative tests with a separate identity before that increment receives runtime acceptance.

## Explicitly not included

- user enrollment or user-management HTTP routes;
- server-side password-hash generation or password-change workflow;
- browser cookie session issuance and CSRF protection;
- refresh tokens, native bearer tokens, MFA, recovery, or device enrollment;
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
| Managed identity provisioning preserves existing records and rejects metadata conflicts | `test_managed_basic_authenticator.cpp` |
| Managed verifier stores a one-way hash binding, not a submitted password/header | verifier repository contract and architecture guards |
| Correct managed password authenticates the configured actor/device/session/credential | managed authenticator positive test |
| Wrong, malformed, unsupported, or unknown managed credentials fail closed | managed authenticator and HTTP-gate negative tests |
| Managed identity cannot use the legacy unmigrated-POST bypass | compatibility-mode HTTP-gate negative test |
| Invalid credentials on an enforced-mode GET are rejected rather than downgraded to anonymous | enforced-mode HTTP-gate negative test |
| Expired/revoked persisted state is rejected | repository/resolver/HTTP-gate negatives and real-VDR revocation evidence |
| Existing legacy browser and migrated Remote path remain supported | compatibility tests and real-VDR recovery evidence |
| Architecture remains server-owned | `tools/check_security_identity_architecture.py` |

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Phase 62 remains open. Slice 2 still requires production browser session/cookie and CSRF behavior, native/service credential issuance, logout/rotation/recovery, and protected lifecycle administration before it is complete.
