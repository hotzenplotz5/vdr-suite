# Public-v1 Human Account mutations — developer contract

## Navigation

- [API developer entry](README.md)
- [OpenAPI source-verified subset](openapi.json)
- [Account and grant read guide](accounts-and-grants.md)
- [Accepted MU.6C Account lifecycle](../docs/development/post-phase69-mu6c-public-account-lifecycle-mutation.md)
- [Accepted MU.6D Account CREATE with idempotency](../docs/development/post-phase69-mu6d-account-create-idempotency.md)
- [Accepted MU.7 grant desired state](../docs/development/post-phase69-mu7-account-grant-administration.md)
- [Account Credential and Browser Session administration](account-security-administration.md)

## Scope

Reconciled against `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11). This documents **implemented Public-v1 Account mutations**, not a live-server mutation test. All calls are authenticated global protected administration calls; Browser Sessions require central CSRF and an effective `role.read-only@*` blocks mutation. A backend-local administrator grant does not replace global permissions.

## Account CREATE

```http
POST /api/v1/accounts
Content-Type: application/json
Idempotency-Key: <new stable logical request key>

{"loginName":"viewer","displayName":"Living Room","password":"<initial-password>"}
```

Exactly the three string fields are accepted, body at most 4096 bytes, no query or `If-Match`. Requires `accounts.create@*`. `Idempotency-Key` must be caller-owned nonempty visible ASCII, at most 160 bytes, without whitespace. Successful CREATE **and exact replay** return HTTP 201, strong Account ETag, `Location: /api/v1/accounts/{accountId}`, and secret-free Account representation. There are **no automatic grants/roles**.

The server's durable idempotency scope uses authenticated actor and key. The request fingerprint binds the non-secret `loginName` and `displayName`, not the password. A replay with the same key and non-secret fields returns the **original** Account without changing the password. Changed non-secret fields yield `409 idempotency_conflict`; a login-name collision yields `409 operation_conflict`. A client must never generate a new key for a blind retry after uncertain response.

## Account lifecycle mutation

```http
POST /api/v1/accounts/{accountId}
Content-Type: application/json
If-Match: <current strong Account ETag>

{"displayName":"Living Room Admin"}
```

Or supply exactly one `{"active":true}` / `{"active":false}` member. Required global permissions: `accounts.modify@*` for display name, `accounts.activate@*` for activate, `accounts.deactivate@*` for deactivate. The ETag belongs to the **Account item**, not Account grant set. Success is 200 with the new Account ETag and secret-free resource. `412 revision_conflict` fences stale writes; deactivating the final usable administrator produces `409 operation_conflict` and must never be bypassed. Deactivation also revokes the Account's active Browser Sessions.

## Grant desired-state mutation

```http
GET /api/v1/accounts/{accountId}/grants

POST /api/v1/accounts/{accountId}/grants
Content-Type: application/json
If-Match: <strong Grant-set ETag from the GET>

{"permission":"channels.view","backendId":"backend-one","active":true}
```

Read authority is global `accounts.grants.view@*`; mutation authority is `accounts.grants.modify@*`. Only the **supported server allowlist** is accepted; the `backendId` is the target grant scope, not the administrator's own grant. The Boolean `active` expresses desired final state. Successful POST returns 200 and the full revised Grant set with a new ETag. Stale ETag returns 412 **when a change is still needed**, but a desired-state no-op can return success. Removing the final usable global admin role produces 409. This **does not** administer Device grants; use [Device administration](device-administration.md).

## Result and failure semantics

- `400` invalid JSON, invalid key or ETag; `401` unauthenticated; `403` missing global permission, read-only/CSRF denial.
- `409` login/idempotency/final-administrator conflict; `412` stale Account or Grant-set ETag.
- `415` invalid Content-Type, `422` closed body/field/tuple failure; `428` missing required `If-Match` for item/grant mutations; `503` authoritative security/storage failure.
- Do not put passwords, idempotency keys, strong ETags or CSRF tokens in logs or URLs.

## Implementations/tests

- [Public Account POST handlers](rest/src/PublicApiRuntime.cpp)
- [Security gate and CSRF](../core/security/include/SecurityHttpGate.h)
- [Account lifecycle and create regression](rest/tests/test_public_account_collection.cpp)
- [Account Grant regression](rest/tests/test_public_account_grants.cpp)
- [Reference client](../clients/reference-js/public-v1-client.js)

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)
