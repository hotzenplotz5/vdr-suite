# MU.6C Public Account Lifecycle Mutation

Status: **COMPLETED — real yaVDR acceptance passed; PR #410 merged.**

Parent workstream: [Post-Phase-69 Multiuser Productization Workstream](post-phase69-multiuser-workstream.md)

Architecture authority: [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)

Foundation:
- [MU.6A Human Account Lifecycle Authority Foundation](post-phase69-mu6-account-lifecycle-foundation.md)
- [MU.6B Public Account Item + Revision/ETag](post-phase69-mu6b-public-account-item.md)

## Purpose

MU.6C exposes the already established Human Account lifecycle authority through the stable Public-v1 Account item without creating a second mutation authority.

The candidate adds exactly three lifecycle operations:

- change the Human Account display name;
- activate a Human Account;
- deactivate a Human Account.

Account CREATE remains MU.6D. Grant administration remains MU.7. Credential/session administration remains MU.8. Admin UI remains MU.9.

## Public contract

The existing Account item remains the resource:

```text
GET  /api/v1/accounts/{accountId}
POST /api/v1/accounts/{accountId}
```

POST accepts exactly one closed mutation field per request:

```json
{"displayName":"Living Room Admin"}
```

or:

```json
{"active":true}
```

or:

```json
{"active":false}
```

The operation-to-authority mapping is fixed:

| Request | Required permission | Scope |
| --- | --- | --- |
| `displayName` | `accounts.modify` | `*` |
| `active:true` | `accounts.activate` | `*` |
| `active:false` | `accounts.deactivate` | `*` |

A backend-scoped administrator such as `role.admin@default` does not satisfy these global Account permissions. `role.admin@*` may satisfy them through the accepted ADR-0067 role vocabulary. `role.read-only@*` blocks them because these are protected mutations.

## Concurrency and response semantics

Every lifecycle mutation requires exactly one canonical strong `If-Match` entity tag obtained from the Account item read.

The Account ETag still represents the persisted Account revision. MU.6C does not expose the raw revision number in the response body.

Successful mutation returns the same secret-free Account item representation with the new strong ETag.

Relevant failures remain explicit:

- `400` for malformed request/precondition syntax;
- `401` for missing authentication;
- `403` for missing global authority, wrong scope, browser CSRF failure or read-only fencing;
- `404` for an unknown Account;
- `409` when deactivation would remove the final usable administrator;
- `412` when the supplied Account revision is stale;
- `415` for unsupported content type;
- `422` for a syntactically valid but invalid/ambiguous Account mutation;
- `428` when `If-Match` is missing;
- `503` when the authoritative lifecycle service is unavailable.

## Existing authority reused

MU.6C delegates to `HumanAccountAdministrationService` from MU.6A.

Therefore the public route does not reimplement lifecycle persistence. It preserves the existing transactional invariants:

- Account revision fencing;
- Human Account and actor display-name synchronization;
- Account/actor active-state synchronization;
- browser-session revocation on deactivation;
- final usable administrator protection;
- accountability events for lifecycle actions and failure boundaries.

The production HTTP dispatcher owns this binding on the same security database and session lifecycle repositories already used for browser authentication. No second Account authority is introduced in the daemon.

## Browser and client safety

Browser-authenticated mutations remain protected by the central `SecurityHttpGate` CSRF boundary.

The Public-v1 reference JavaScript client adds:

- `updateAccountDisplayName(...)`;
- `activateAccount(...)`;
- `deactivateAccount(...)`.

All three use the Account item path, require caller-supplied `ifMatch`, preserve caller headers such as the browser CSRF token and return the new ETag.

## Local backend-access grant is not a main patch

The earlier real yaVDR playback repair that added `role.admin@default` is intentionally separate from MU.6C and does not require a bootstrap or code patch on `main`.

The first-admin bootstrap deliberately creates global `role.admin@*`. Global administration and concrete backend media access are separate authorities. A backend-scoped grant such as `role.admin@default` is runtime access configuration for that backend; it must not be inferred from the global first-admin role.

## Acceptance

Required focused checks:

```bash
make -j2 test-security-public-account-collection
make -j2 test-security-human-account-administration
make test-security-architecture
```

The production HTTP composition must also link successfully through the daemon build target before the candidate is accepted.

Acceptance must prove at least:

- display-name mutation with a fresh Account ETag;
- activate/deactivate mutation with a fresh Account ETag;
- stale `If-Match` rejection;
- browser CSRF enforcement;
- direct global permissions and `role.admin@*`;
- denial of backend-scoped admin for global Account mutation;
- read-only fencing;
- final usable administrator protection;
- session revocation on deactivation.

Acceptance passed on real yaVDR. The production daemon linked successfully and the real HTTP run proved Human Account login, display-name mutation, stale `If-Match` rejection, browser CSRF rejection, missing-`If-Match` rejection, final-usable-administrator protection, state restoration and logout. The supported system had one usable administrator and no safe second Account, so a destructive deactivate/reactivate round-trip was intentionally not run against that sole administrator; the lifecycle and Public-v1 tests cover the state-changing activate/deactivate path.

PR #410 merged MU.6C into `main`. MU.6D Account CREATE is now the active bounded candidate.

Phase 70 remains not started.
