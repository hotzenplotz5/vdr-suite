# Public-v1 Human Credential and Browser Session administration

## Navigation

- [API developer entry](README.md)
- [OpenAPI 3.1 partial source contract](openapi.json)
- [Human Account and permission grants](accounts-and-grants.md)
- [Human Account mutations](account-mutations.md)
- [MU.8A secret-free metadata](../docs/development/post-phase69-mu8a-credential-session-metadata.md)
- [MU.8B Session revoke](../docs/development/post-phase69-mu8b-session-revoke.md)
- [MU.8C Human-password Credential revoke](../docs/development/post-phase69-mu8c-credential-revoke.md)

## Status and authority

Source-verified against `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11). MU.8B and MU.8C implementations were merged; the earlier MU.8A status still contains an acceptance-debt note. This page documents **current code and focused tests**, not a new deployed yaVDR acceptance. It uses the existing canonical Credential, Session, issuer-Credential and BrowserSessionLifecycleService authority, not a second security store.

All reads/mutations are **global Human Account administration**. A device-app credential and an administrative Human-password credential are not interchangeable. No browser cookie, bearer secret, password verifier hash or CSRF secret appears in the public response.

## 1. Secret-free collection reads

```http
GET /api/v1/accounts/{accountId}/credentials
GET /api/v1/accounts/{accountId}/sessions
```

Require `accounts.credentials.view@*` and `accounts.sessions.view@*`, respectively. Results contain `accountId`, `actorId`, `items` and `links.self`. **Neither collection has pagination, collection ETag, or conditional 304 semantics.** They do not expose a credential/token that could authenticate a caller.

Credential collection `items` contain `credentialId`, `credentialType`, `active`, `expired`, `revoked`, `expiresAt` (nullable), `createdAt`. Browser-session internal credential entries are **excluded**.

Session collection `items` contain `sessionId`, `deviceId`, `issuedFromCredentialId`, `active`, `expired`, `revoked`, `expiresAt` (nullable), `lastSeenAt` (nullable), `createdAt`. The projection respects revocation of the issuing Credential. Collection responses do not expose the per-item resource ETag.

## 2. Revisioned item reads

```http
GET /api/v1/accounts/{accountId}/credentials/{credentialId}
GET /api/v1/accounts/{accountId}/sessions/{sessionId}
If-None-Match: <previous matching ITEM ETag>
```

Credential reads require `accounts.credentials.view@*`; Session reads require `accounts.sessions.view@*`. The item returns `accountId`, `actorId`, the corresponding metadata and `links.self`, with a **strong opaque lifecycle ETag** in the response header. Matching `If-None-Match` returns **304 with no body**. The Account ETag must not substitute for a Credential/Session ETag. Session `lastSeenAt` activity is excluded from its lifecycle revision to avoid spurious conflicts.

## 3. Revoke one Session

```http
POST /api/v1/accounts/{accountId}/sessions/{sessionId}
Content-Type: application/json
If-Match: <strong Session-item ETag>

{}
```

Requires global `accounts.sessions.revoke@*`, central Browser Session CSRF where applicable and an empty `{}` request (max 1024 bytes). Success is HTTP **200** with terminal Session metadata and a new strong ETag. Revoke completes synchronously in the canonical BrowserSessionLifecycleService and fences the bound Browser Session/credential state. It does not revoke all Account Sessions.

A stale ETag on a **still-active** Session returns `412 revision_conflict`; an already-terminal Session may return success even with an earlier ETag. The route has no bulk revoke, cookie issuance or password reset.

## 4. Revoke one Human-password Credential

```http
POST /api/v1/accounts/{accountId}/credentials/{credentialId}
Content-Type: application/json
If-Match: <strong Credential-item ETag>

{}
```

Requires global `accounts.credentials.revoke@*`, central Browser Session CSRF where relevant and a closed `{}` body (max 1024 bytes). Only a **human-password** Credential may be revoked here. Revoking a device-app, browser-session or other unsupported type returns `422 validation_error`; use the [Device lifecycle API](device-lifecycle.md) for Device Credential revocation.

The synchronous transaction revokes the canonical Credential **and every active Browser Session issued from it**. The last usable administrator is protected at **Credential granularity**: an admin may revoke one of several independently usable credentials, but may not revoke the one that would leave no usable administrator. That case returns `409 operation_conflict`.

Same desired-state replay semantics apply as for Session revoke: already terminal may succeed; stale still-active revision gives `412 revision_conflict`. An operation never reactivates a revoked Credential or restores a Session.

## Error contract and evidence

| HTTP | Meaning |
| --- | --- |
| 200 | Metadata read, or synchronous revoke completed/already terminal |
| 304 | Item If-None-Match unchanged (bodyless); never a collection response |
| 400 | Invalid path/query, malformed JSON or wrong lifecycle precondition |
| 401 / 403 | Missing identity / missing global authorization or CSRF |
| 404 | Human Account, Credential or Session not found under Account |
| 409 | Final usable administrator Credential cannot be revoked |
| 412 | Stale lifecycle If-Match while the resource is still active |
| 415 / 422 / 428 | Wrong media type / nonempty body or wrong Credential type / missing If-Match |
| 503 | Canonical lifecycle, issuer-session, audit or persistence unavailable |

OpenAPI describes `application/problem+json` failures from the Public-v1 runtime. The Security HTTP gate can reject first. Do not conflate HTTP 200 for a successful read with authorization for a subsequent write.

## Implementations and tests

- [Public REST serializers and item/collection handlers](rest/src/PublicApiRuntime.cpp)
- [Global permissions and Browser CSRF gate](../core/security/include/SecurityHttpGate.h)
- [Collection metadata test](rest/tests/test_public_account_security_metadata.cpp)
- [Credential revoke test](rest/tests/test_public_account_credential_revoke.cpp)
- [Session revoke test](rest/tests/test_public_account_session_revoke.cpp)
- [Account Credential revoke Security test](../core/security/tests/test_public_account_credential_revoke_security.cpp)
- [Account Session revoke Security test](../core/security/tests/test_public_account_session_revoke_security.cpp)
- [Public-v1 reference client](../clients/reference-js/public-v1-client.js)

No runtime, installed daemon or VDR native behavior is changed by this documentation-only slice.

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)
