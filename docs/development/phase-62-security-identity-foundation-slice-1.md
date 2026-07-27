# Phase 62 Security Identity Foundation — Slice 1

Status: implemented and real-runtime validated first Phase 62 slice; Phase 62 remains open

## Why this slice comes first

The repository already had backend safety checks and mature mutation services, but HTTP authentication collapsed every caller into one boolean result. Adding more endpoint-specific checks on top of that boolean would have produced a second ad-hoc authorization architecture.

Phase 62 Slice 1 therefore establishes the smallest reusable boundary that can be exercised by a real mutation:

1. represent authentication state, actor, device, session, request and correlation identity;
2. evaluate permission and backend scope centrally;
3. preserve the existing local browser credential through an explicitly named compatibility adapter;
4. protect one high-impact mutation before router dispatch;
5. persist the authorization decision in an append-only repository before an allowed dispatch can occur.

The remote-control route was selected because it is actively used by the web frontend and already has a request `backendId`, required `operationId`, action allowlist, backend read-only guard, capability guard and executor boundary. The slice adds actor authorization without duplicating those domain checks.

## Implemented scope

- `ActorIdentity` and actor types: anonymous, user, service, agent and system.
- `DeviceIdentity` and `SessionIdentity`.
- explicit authentication states: anonymous, authenticated, invalid, expired and revoked.
- `RequestSecurityContext` with request and correlation IDs.
- `PermissionGrant` with exact or wildcard backend scope.
- central `AuthorizationService`.
- `LegacyBasicAuthenticator` as a transitional adapter, not a production authentication claim.
- two rollout modes:
  - `legacy-basic` (default): preserves the existing requirement for the configured Basic credential and grants the compatibility actor its configured permissions;
  - `enforced`: anonymous GETs are allowed, the migrated remote mutation requires authentication plus `remote.control` for its backend, and all other POST routes fail closed until explicitly migrated.
- stable security errors containing `error.code`, `error.message` and `error.requestId`, with `Cache-Control: no-store`.
- `X-Request-ID` generation/propagation and optional `X-Correlation-ID` propagation.
- append-only `accountability_events` SQLite table, indexes and update/delete rejection triggers.
- pre-dispatch `authorization.allowed` and `authorization.denied` evidence for `POST /api/vdr/remote/actions`.
- actor, actor type, device, session, authentication state, permission, backend, operation, request, correlation, action, decision, reason and dispatch authorization outcome in each decision event.
- fail-closed 503 response when required accountability persistence is unavailable.
- automated architecture, unit, repository and HTTP-gate tests.

## Explicitly not included in Slice 1

- persistent user, device, session, credential, role or grant repositories;
- password hashing, cookie sessions, CSRF, native token issuance, refresh, MFA, enrollment, recovery, rotation, logout or revocation commands;
- migration of recording, timer, channel, SearchTimer, cache or administration mutations;
- a universal revision or `If-Match` contract;
- a universal `Idempotency-Key` repository or replay contract;
- mutation completion/outcome events or a transactional outbox;
- protected audit query/export/retention APIs;
- an Android or Android TV application;
- production remote-site or agent operation from Phase 63;
- timer orchestration from Phase 64;
- streaming/media sessions from Phase 65;
- legacy OSD runtime from Phase 66;
- stable `/api/v1` or SDK release from Phase 67.

The subsequent [Slice 2](phase-62-security-identity-foundation-slice-2.md) adds a persistent lifecycle foundation, but it does not retroactively change the bounded Slice 1 scope.

## Compatibility and rollout configuration

| Environment variable | Meaning |
|---|---|
| `VDR_SUITE_SECURITY_MODE` | `legacy-basic` by default; set to `enforced` only after required mutation routes and grants are prepared |
| `VDR_SUITE_BASIC_AUTH` | Existing complete Basic Authorization header value; retained only for compatibility |
| `VDR_SUITE_LEGACY_BASIC_ACTOR_ID` | Actor ID assigned to the compatibility credential |
| `VDR_SUITE_LEGACY_BASIC_DEVICE_ID` | Device ID assigned to the compatibility client |
| `VDR_SUITE_LEGACY_BASIC_SESSION_ID` | Session ID assigned to the stateless compatibility request |
| `VDR_SUITE_LEGACY_BASIC_PERMISSIONS` | Comma-separated `permission@backend` grants; backend may be `*`; compatibility default is `*@*`, enforced default is no grants |
| `VDR_SUITE_SECURITY_DATABASE_PATH` | Optional accountability database path; defaults to `VDR_SUITE_DATABASE_PATH`, then `/tmp/vdr-suite-test.db` |

Examples:

- `remote.control@default`
- `remote.control@living-room,remote.control@holiday-home`
- `*@*` only for the transitional local compatibility behavior

The complete credential is compared but is never returned in an error, copied into the request context or stored in accountability rows.

## Request decision order for the migrated route

1. establish or validate request and correlation IDs;
2. authenticate through the configured compatibility adapter;
3. classify the route;
4. extract the backend and operation context needed for policy;
5. authorize `remote.control` for that backend;
6. append the allow/deny decision;
7. stop with a stable 400/401/403/503 response when required;
8. only after a durable allow decision, dispatch to the existing router;
9. retain `RemoteActionService` validation, backend read-only, capability and executor checks.

## Acceptance criteria and evidence

| Criterion | Evidence |
|---|---|
| Existing local browser authentication remains compatible by default | `test_security_http_gate.cpp`, existing frontend/full suite and real yaVDR browser actions |
| Anonymous compatibility access is rejected | 401 `authentication_required` automated and real-runtime negative |
| Invalid credential is rejected without reflection | 401 `invalid_credentials`, secret absence assertion and real-runtime negative |
| Enforced anonymous GET is possible | anonymous GET positive case |
| Authenticated remote mutation with permission and backend scope is allowed | exact-scope positive and real-browser actions |
| Missing permission is rejected | 403 `permission_denied` |
| Wrong backend scope is rejected | 403 `backend_scope_denied` |
| Missing backend context is rejected before dispatch | 400 `invalid_backend_scope` |
| Expired/revoked session and revoked actor/device are denied centrally | authorization unit negatives |
| Unmigrated POST route is fail-closed in enforced mode | 503 `security_policy_not_migrated` |
| Required pre-dispatch audit failure prevents mutation dispatch | 503 `accountability_unavailable` |
| Accountability rows cannot be updated or deleted through normal SQL | SQLite trigger tests |
| Request/correlation IDs are propagated | response-header assertions and real anonymous request ID propagation |
| No credential is stored in error or audit values | negative string assertions and inspected runtime rows |
| Architecture boundary remains wired | `tools/check_security_identity_architecture.py` |

## Real-VDR acceptance — 2026-07-27

The branch was installed from a separate worktree on the yaVDR host and used `/var/lib/vdr-suite/vdr-suite.db`.

Observed evidence:

- anonymous Remote POST returned `401 authentication_required`, `Cache-Control: no-store` and the supplied `X-Request-ID`;
- its audit row recorded `authorization.denied`, anonymous authentication and `dispatch_denied`;
- an invalid Basic credential returned `401 invalid_credentials` without reflecting the submitted value;
- its audit row recorded `authentication_state=invalid` and `dispatch_denied`;
- two browser Remote actions were attributed to `legacy-local-web`, `legacy-browser` and `legacy-basic-session`;
- both were authorized as `remote.control@default` with independent request and operation IDs and `dispatch_authorized`;
- neither negative request dispatched a VDR key action.

Canonical checks:

```text
make test-security
make test-architecture
make test-docs
make test
```

Slice 1 is therefore validated on the real runtime. Phase 62 remains open because production identity issuance, remaining route migration, common mutation contracts and complete accountability are separate later slices.
