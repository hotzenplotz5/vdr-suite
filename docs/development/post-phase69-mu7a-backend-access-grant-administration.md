# MU.7A — Bounded Backend Access Grant Administration

Status: **IMPLEMENTED CANDIDATE — local and real yaVDR acceptance pending.**

MU.7A is the first runtime slice of the ADR-0067 backend-access / permission
grant administration line. It closes one already-proven product gap without
creating a second permission authority.

## Proven gap

MU.6D deliberately creates a Human Account with no automatic role or backend
grant. Real yaVDR acceptance proved that such an authenticated Account receives
HTTP 403 for:

```text
GET /api/v1/channels?backendId=default
```

ADR-0067 requires a product administration surface over the existing normalized
grant authority. The canonical persistence remains
`security_actor_permission_grants` through
`SecurityPermissionGrantRepository`.

## Bounded MU.7A allowlist

MU.7A administers exactly:

```text
channels.view@<concrete-backend>
```

The backend identifier must be explicit and concrete. Wildcard `*` is not an
MU.7A target. No other permission is accepted by
`HumanAccountGrantAdministrationService`.

This deliberate boundary proves backend-access administration end to end before
later MU.7 slices widen the product allowlist.

## Public-v1 resource

```text
GET  /api/v1/accounts/{accountId}/grants
POST /api/v1/accounts/{accountId}/grants
```

Read authority:

```text
accounts.grants.view@*
```

Mutation authority:

```text
accounts.grants.modify@*
```

Global `role.admin@*` may satisfy those administration permissions through the
existing `AuthorizationService` role expansion. A backend-scoped
`role.admin@default` does not satisfy the global administration scope.

The response is secret-free and contains only the Account ID, Actor ID and the
MU.7A-supported normalized grants.

## Revision and desired-state contract

The grant resource carries a deterministic opaque grant-set revision and strong
ETag derived from the sorted effective MU.7A grant set.

Mutation requires a strong `If-Match` for that grant-set resource and a closed
JSON body:

```json
{
  "permission": "channels.view",
  "backendId": "default",
  "active": true
}
```

Desired-state rules:

- matching revision + absent tuple + `active:true` -> ensure one active grant;
- matching revision + active tuple + `active:false` -> revoke it;
- stale revision + target already in requested final state -> success without
  duplicating or resurrecting state;
- stale revision + target not already in requested final state -> 412 revision
  conflict;
- unsupported permission or non-concrete backend scope -> fail closed.

An Idempotency-Key is intentionally not added: the ADR-0067 desired-state
contract provides the bounded retry behavior for this Class-A mutation.

## Security and accountability

The implementation reuses:

- `AuthorizationService`;
- `SecurityHttpGate`;
- `SecurityPermissionGrantRepository`;
- `HumanAccountRepository`;
- `SecurityIdentityRepository`;
- `AccountabilityEventRepository`.

Browser POST remains a protected mutation behind the established CSRF gate.
Grant mutations append secret-free accountability under
`security.human-account.grant-administration` with the administrator Actor,
requested permission/backend scope, request/correlation IDs and outcome.

No direct SQL grant mutation is introduced outside the canonical repository.

## Deliberate exclusions

MU.7A does **not** implement:

- `role.admin@*` or `role.read-only` target grant administration;
- final-usable-administrator role mutation (required when role.admin mutation
  is later admitted);
- a broad permission catalog;
- credential create/delete/rotation/revoke;
- session administration or generic session revoke;
- password changes;
- QR/device pairing;
- browser Admin UI;
- Phase 70.

Credential/session administration remains MU.8. Browser administration UI
remains MU.9.

## Required acceptance

Focused local acceptance must prove:

1. the canonical grant service ensures/revokes only the bounded target;
2. desired-state replay and stale revision conflict semantics;
3. Public-v1 ETag / If-Match behavior;
4. `accounts.grants.view@*` and `accounts.grants.modify@*` gate behavior,
   including browser CSRF;
5. architecture/document guards;
6. the productive daemon still links.

Real yaVDR acceptance must then prove through the public authority/API, without
direct SQL writes:

1. an existing Account without the backend grant gets 403 for
   `GET /api/v1/channels?backendId=default`;
2. an administrator grants `channels.view@default`;
3. the same Account can then read the default backend Channel collection;
4. a mismatched backend scope remains forbidden;
5. revoking the grant removes access again;
6. audit/revision/concurrency behavior remains consistent with ADR-0067;
7. no credential/session material is exposed;
8. the original system state is restored.

Until those gates pass, MU.7A remains an implementation candidate and is not an
accepted or completed MU.7 slice.
