# Public-v1 Human Accounts and permission grants — developer guide

## Navigation

- [API entry point](README.md)
- [OpenAPI 3.1 partial contract](openapi.json)
- [Device pairing and Device-grant boundary](device-pairing.md)
- [Administrative Device grant API](device-administration.md)
- [MU.6B Account item and ETag acceptance](../docs/development/post-phase69-mu6b-public-account-item.md)
- [MU.7 accepted Account Grant administration](../docs/development/post-phase69-mu7-account-grant-administration.md)
- [Public-v1 compatibility contract](../docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)

## Audited scope

Source baseline `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` on 2026-10-11. This first account pass formally specifies **read-only** `GET /api/v1/accounts`, `GET /api/v1/accounts/{accountId}`, and `GET /api/v1/accounts/{accountId}/grants`. Their counterpart **mutation routes exist in source** and remain next-phase OpenAPI coverage, not absent features. The branch does not deploy or change runtime code.

## Human Account collection

```http
GET /api/v1/accounts?limit=50&sort=accountId&order=asc
```

Authenticated `accounts.view@*` is required. A backend-local `role.admin@backend-a` does not grant global Human Account administration. Keyset `accountId ASC`, optional `limit` 1–100 (default 50), opaque `ac1_` cursor (max 4096 characters), optional fixed `sort=accountId` and `order=asc`. Unsupported query fields fail. Collection items contain `accountId`, bound `actorId`, `displayName`, and `active`. The collection has `page`/`meta.partial=false`/`links` but no ETag or secret fields. It is **Human Account** administration, not a Device list.

## Human Account item

```http
GET /api/v1/accounts/{accountId}
If-None-Match: <opaque previous ETag>
```

Requires `accounts.view@*`. Item response exposes `accountId`, `actorId`, `displayName`, `active`, `links.self`. A **strong opaque ETag** derives from the Human Account lifecycle revision; matching conditional read returns 304 with an empty body. No password, credential, session, grant list or raw revision is exposed here.

## Permission Grant set

```http
GET /api/v1/accounts/{accountId}/grants
```

Requires the distinct global permission `accounts.grants.view@*`. Grants are sorted and constrained to the accepted public allowlist. The wire response includes:

```json
{
  "accountId": "account-a",
  "actorId": "actor-a",
  "items": [{"permission": "channels.view", "backendId": "backend-a"}],
  "supportedPermissions": ["channels.view"],
  "supportedPermissionOptions": [{"permission": "channels.view", "presentationKey": "channels.view", "category": "channels"}],
  "supportedScopeKinds": ["backend"],
  "links": {"self": "/api/v1/accounts/account-a/grants"}
}
```

The support arrays above are **illustrative**, not an exhaustive hardcoded permission allowlist. Use the server response as authority. The Grant-set ETag is independently derived from supported effective grants; it does **not** equal the Human Account ETag. `If-None-Match` works like other revisioned read resources.

The source also implements desired-state `POST /api/v1/accounts/{accountId}/grants` using `accounts.grants.modify@*`, `If-Match`, a closed `{permission,backendId,active}` body, and protection of the final usable administrator. That mutation is **not yet in this partial OpenAPI**; never infer its request from the GET schema. Account permission grants and Device permission grants are separate target scopes; the Device-specific GET/POST contract is documented in [Device administration](device-administration.md). Pairing by itself does not grant any Device permission.

## Error behavior

- `401`: missing authenticated actor; `403`: missing global Account/Grant authorization.
- `400`: malformed IDs, unsupported collection query/cursor, or invalid conditional header; `404`: Account missing.
- `304`: conditional item/grant-set read unchanged, with no body.
- `503`: security audit/grant persistence or account data authority unavailable.

Public runtime errors use `application/problem+json`, with `code` and request identity; security-gate denials may occur before runtime evaluation.

## Source and tests

- [Account collection/item and grant-set serializer](rest/src/PublicApiRuntime.cpp)
- [Global authorization gate](../core/security/include/SecurityHttpGate.h)
- [Account collection regression](rest/tests/test_public_account_collection.cpp)
- [Account Grant regression](rest/tests/test_public_account_grants.cpp)
- [Reference client Account tests](../clients/reference-js/tests/test_public_v1_account_client.js)
- [Reference client Account Grant tests](../clients/reference-js/tests/test_public_v1_account_grants_client.js)

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)
