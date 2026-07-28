# Phase 62 Persistent Identity Lifecycle — Slice 2

Status: persistence and revocation foundation implemented and real-VDR accepted; Phase 62 remains open

## Purpose

Phase 62 Slice 1 established an explicit request identity, centralized authorization, the first protected mutation, and append-only pre-dispatch accountability. Slice 2 removes the next critical limitation: authenticated identity state is no longer only transient configuration assembled independently for every request.

This slice adds repository-owned persistence and request-time resolution for the compatibility actor, device, session, and credential identity. It does not claim that the transitional Basic credential is production authentication.

## Runtime boundary

```text
HttpServerRequest
  -> LegacyBasicAuthenticator
       -> transient RequestSecurityContext
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

The authenticator verifies the presented compatibility credential. The resolver then replaces transient lifecycle assumptions with persisted server-side state before authorization is evaluated.

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
- repository, resolver, authorization, configuration, HTTP-gate, and architecture tests.

## Storage contract

The security database connection creates these tables when absent:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`.

The tables are additive and use the existing security database path resolution:

1. `VDR_SUITE_SECURITY_DATABASE_PATH`;
2. `VDR_SUITE_DATABASE_PATH`;
3. `/tmp/vdr-suite-test.db`.

The compatibility bootstrap inserts only stable identity metadata. It does not persist the Basic Authorization value, a decoded password, a reversible secret, or request headers.

## Compatibility behavior

The default local browser remains mapped to:

```text
actor:      legacy-local-web
device:     legacy-browser
session:    legacy-basic-session
credential: legacy-basic-credential
```

The IDs remain configurable. Existing deployments receive the four records automatically on startup while the compatibility credential is configured. `INSERT OR IGNORE` is intentional: restarting the daemon must not reactivate a record that was already expired, disabled, or revoked in SQLite.

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

## Real-VDR acceptance

The Slice 2 head was installed on the yaVDR system and used the production Suite database at `/var/lib/vdr-suite/vdr-suite.db`.

Observed on 2026-07-28:

- daemon startup created one active compatibility actor, device, session, and credential with the expected ownership bindings;
- the stored records contained identity and lifecycle metadata only; no Basic Authorization value, password, token, or reversible secret was present;
- setting `legacy-basic-credential.active=0` and assigning `revoked_at` immediately blocked browser authentication without restarting the daemon;
- accountability recorded the persisted actor as `legacy-local-web`, authentication state `revoked`, reason `credential_revoked`, decision `denied`, and outcome `dispatch_denied`;
- several denial rows in the same second represented parallel browser document, asset, and API requests while the sole browser credential was revoked; they were independent access denials, not repeated VDR action dispatches;
- restoring `active=1` and clearing expiry/revocation values restored browser access without restarting the daemon;
- the following Remote request was attributed to `legacy-local-web`, authorized with `remote.control@default`, and recorded as `permission_granted` / `dispatch_authorized`.

This proves automatic bootstrap, durable revocation, request-time lifecycle resolution, fail-before-dispatch behavior, and recovery on the real runtime. The test also confirms that revoking the sole compatibility credential blocks the complete legacy browser, not only the Remote route; future lifecycle tests should use a separate test identity once production issuance exists.

## Explicitly not included

- user enrollment or user-management HTTP routes;
- password hashing or password verification against the new repository;
- cookie session issuance, CSRF protection, refresh tokens, native bearer tokens, MFA, recovery, or device enrollment;
- credential secret storage;
- administrative revoke/reactivate APIs;
- persisted roles, permissions, grant assignments, or backend scopes;
- migration of additional mutation routes;
- revision, `If-Match`, idempotency-key, operation-lifecycle, or transactional-outbox work;
- Phase 63-67 runtime or client implementation.

Those omissions keep this slice coherent: it establishes the durable lifecycle boundary that later authentication and administration flows can use without prematurely publishing an insecure management API.

## Acceptance evidence

| Criterion | Evidence |
|---|---|
| Compatibility identity is created without storing its secret | repository schema and bootstrap test; architecture secret guard; real-VDR table inspection |
| Actor/device/session/credential bindings are persisted | `test_security_identity_repository.cpp`; real-VDR bootstrap rows |
| Persisted display name replaces transient request value | resolver assertion |
| Expired session is rejected | repository expiry plus resolver negative |
| Revoked credential is rejected before HTTP dispatch | HTTP-gate integration negative and real-VDR `credential_revoked` audit evidence |
| Missing repository records fail closed | resolver behavior and repository optional lookups |
| Repeated startup does not overwrite lifecycle state | `INSERT OR IGNORE` bootstrap contract |
| Existing browser and protected Remote path remain supported | compatibility tests plus real-VDR recovery and subsequent `remote.control@default` allow event |
| Persisted lifecycle state is re-read without daemon restart | real-VDR revoke and restore exercise |
| Architecture remains server-owned | `tools/check_security_identity_architecture.py` |

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Slice 2 is implementation-, CI-, and real-runtime accepted. Phase 62 remains open because secure credential issuance and verification, production browser/native session lifecycle, protected administration, persisted roles/grants, complete route migration, mutation contracts, and complete accountability are still outstanding.