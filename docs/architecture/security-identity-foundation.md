# Security and Identity Foundation

Status: Phase 62 Slice 1 and persistent lifecycle foundation from Slice 2; incomplete Phase 62

## Boundary

The Phase 62 security boundary is server-side and precedes API dispatch.

```text
HttpServerRequest
  -> SecurityHttpGate
       -> LegacyBasicAuthenticator (transitional adapter)
       -> RequestSecurityContext
       -> PersistentIdentityResolver
            -> SecurityIdentityRepository
       -> AuthorizationService
       -> AccountabilityEventRepository
  -> ApiRouter
  -> existing controller/service/domain safety checks
```

Authentication answers who or what presented credentials. Persistent identity resolution answers whether that actor, device, credential, and session still exist and remain active. Authorization answers whether that actor may perform an action on a backend. Backend access policy independently answers whether the backend itself accepts writes. Capability checks independently answer whether the backend can perform the action. None of these decisions replaces another.

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

Actor types can represent users, services, agents, and system work. The current authenticator still emits only the configured legacy local web actor. That actor is transitional and is not the final user or native-client authentication architecture.

## Persistent lifecycle model

`SecurityIdentityRepository` owns additive SQLite persistence for:

- `security_actors`;
- `security_devices`;
- `security_sessions`;
- `security_credentials`.

An authenticated compatibility request is first represented transiently by `LegacyBasicAuthenticator`. `PersistentIdentityResolver` then reads repository state and verifies:

- the actor exists and remains active;
- the device belongs to the actor and remains active;
- the credential belongs to the actor and is neither expired nor revoked;
- the session belongs to the actor and device and is neither expired nor revoked.

Missing or mismatched records fail closed as invalid credentials. Lifecycle state is resolved for each authenticated request rather than trusted from the submitted request.

The compatibility identity is bootstrapped with `INSERT OR IGNORE`. This prevents a daemon restart from silently reactivating an existing revoked record.

## Authorization model

A permission grant contains:

- permission name;
- backend scope, either an exact backend ID or `*`.

The first protected permission is:

```text
remote.control@<backend-id>
```

Authorization is fail-closed for:

- anonymous or invalid authentication;
- expired or revoked credential/session state;
- inactive or revoked actor or device;
- missing permission;
- wrong backend scope;
- missing backend context.

The remote mutation is evaluated before `ApiRouter::handleClientPost`. The existing `RemoteActionService` continues to enforce request validity, operation ID, action allowlist, backend existence, backend read-only state, capability, and executor availability.

## Compatibility modes

### `legacy-basic`

This is the default migration mode.

- It preserves the pre-Phase-62 behavior that every HTTP request requires the configured Basic credential.
- The credential maps to an explicit actor/device/session/credential context.
- The mapped lifecycle records are stored server-side and resolved on every authenticated request.
- The default grant remains `*@*` only to prevent an unannounced local browser outage.
- Authorization decisions for the migrated remote route are still evaluated and audited.

This mode is a named compatibility mechanism, not a final security claim.

### `enforced`

- Anonymous GET requests can reach existing read routes.
- The migrated remote mutation requires authentication, active persisted identity state, permission, and backend scope.
- Other POST routes return `security_policy_not_migrated` before router dispatch.
- This deliberately prevents an accidental unauthenticated mutation while route-by-route Phase 62 migration is incomplete.
- No embedded default credential or grant is active unless it is explicitly configured for the enforced rollout.

## Security errors

Phase 62 security errors use:

```json
{
  "error": {
    "code": "permission_denied",
    "message": "The actor lacks the required permission",
    "requestId": "req-..."
  }
}
```

Current codes introduced by the Phase 62 boundary:

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

Security errors are non-cacheable. They never include the submitted credential. The unversioned API still contains older ad-hoc error shapes outside this migrated boundary; convergence remains Phase 62 work and stable public error compatibility remains Phase 67.

## Accountability

`AccountabilityEventRepository` owns the SQLite boundary. It creates an append-only `accountability_events` table and rejects update/delete statements with database triggers.

For the first protected mutation it stores the pre-dispatch authorization decision with:

- event ID, schema version, class, type, severity, and timestamp;
- actor, actor type, device, session, and authentication state;
- permission, backend, operation ID, action;
- request and correlation IDs;
- allow/deny decision, reason code, and dispatch authorization outcome.

An allowed remote mutation is not dispatched when the required decision row cannot be appended. Enforced-mode denial of an unmigrated POST is also recorded before the request is rejected.

This is not yet ADR-0049 completion. Authentication lifecycle events, mutation completion events, transactional outbox semantics, protected audit reads, retention, export, integrity chaining, and the full event catalogue remain open.

## Storage and credential handling

The security database path is resolved in this order:

1. `VDR_SUITE_SECURITY_DATABASE_PATH`;
2. `VDR_SUITE_DATABASE_PATH`;
3. `/tmp/vdr-suite-test.db`.

The HTTP server owns a dedicated SQLite connection and explicit repository, resolver, and gate lifetimes. There is no global security singleton. The connection may point to the same database file as the rest of the daemon while repository ownership remains explicit.

The persistence layer stores a credential identifier and lifecycle metadata, not the complete Authorization header, decoded password, reversible secret, or token material. Authorization header values are not placed in identity tables, domain objects, audit rows, error responses, request IDs, correlation IDs, or operation IDs.

## Remaining authentication boundary

The repository and resolver provide durable lifecycle state but do not implement production credential issuance or verification. Still open within Phase 62 are:

- per-user credential issuance and secure password hashing or token verification;
- browser cookie and CSRF contracts;
- native-client enrollment, token rotation, refresh, logout, and recovery;
- administrative lifecycle APIs protected by explicit permissions;
- persisted roles, grants, and backend scopes.

These capabilities must consume the current repository-owned boundary rather than create a second authentication stack.

## Phase boundaries

This foundation may be consumed by future browser, native, TV, agent, and public API clients. It does not implement those clients and does not advance Phase 63-67 runtime.
