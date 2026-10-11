# Public-v1 TV and Device pairing — developer guide

## Navigation

- [API entry point](README.md)
- [OpenAPI 3.1 partial contract](openapi.json)
- [Account and permission-grant boundary](accounts-and-grants.md)
- [Administrative pairing approval and Device grants](device-administration.md)
- [MU.10A accepted pairing bootstrap](../docs/development/post-phase69-mu10a-device-pairing-bootstrap.md)
- [MU.10B approval contract](../docs/development/post-phase69-mu10b-admin-pairing-approval.md)
- [MU.10D Device authentication contract](../docs/development/post-phase69-mu10d-device-authentication.md)

## Security boundaries

Audited against `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11). A real TV client must distinguish:

```text
short-lived anonymous pairing request
 -> administrator reviews/approves
 -> original token-holder issues a credential exactly once
 -> Device authenticates under its own Service Actor
 -> administrator separately grants required device permissions
 -> protected Public-v1 resources can be used
```

**Pairing approval does not make the TV the approving administrator.** Successful issuance makes a dedicated Actor/Device/Credential identity with **zero automatic permission grants**. No TV needs to store a Human password, browser session cookie or CSRF token.

## 1. Start pairing (anonymous)

```http
POST /api/v1/device-pairings
Content-Type: application/json

{"displayName":"Hisense 43A6K","clientKind":"vidaa","appVersion":"phase1"}
```

The request shape is closed: `displayName` and `clientKind` required nonempty strings, `appVersion` optional, no unknown fields; body up to 2048 bytes, no query, `If-Match` or `Idempotency-Key`. Response **201 Created** includes `pairingRequestId`, `userCode`, `pairingToken`, `status=pending`, `expiresAt`, `pollIntervalSeconds`, `client` and `links.self`; `Location` points to the item.

**The userCode and pairingToken are one-time, short-lived bootstrap values.** They must not be logged, saved to screenshots, displayed outside the intended UI, or treated as durable authorization.

## 2. Poll only the bootstrap request

```http
GET /api/v1/device-pairings/{pairingRequestId}
X-VDR-Suite-Pairing-Token: <short-lived pairing token>
```

This token-scoped GET does **not** authenticate an Actor. It returns a secret-free `status` (`pending`, `approved`, `rejected`), `expiresAt`, `pollIntervalSeconds`, `client` and item `links`. It does not repeat `userCode` or `pairingToken`. No query parameters or ETag. A client may use the returned polling interval; the reference client does not automatically poll.

The **same path without the Pairing token** also supports a separately authorized administrator GET, with a different representation (decision metadata and ETag). That admin mode is **not** included in the bootstrap-only OpenAPI GET entry.

## 3. Administrator approval (separate permission)

An administrator uses server-owned read/decision resources on `/api/v1/device-pairings` and `/api/v1/device-pairings/{pairingRequestId}`. Global `device.pairing.view@*` and `device.pairing.decide@*` permissions and protected browser-session CSRF apply to the respective administrative operations. The decision resource uses a revision-based precondition; never substitute the TV's bootstrap token for an administrator identity.

The [administrative pairing and Device grants guide](device-administration.md) and OpenAPI now document the global administrator collection/decision contract, including `If-Match` and CSRF. The same item GET URL has two credential modes: token-scoped TV polling and global administrator read; keep their representations separate.

## 4. Issue the approved credential once

```http
POST /api/v1/device-pairings/{pairingRequestId}/credential
X-VDR-Suite-Pairing-Token: <same bootstrap token>
```

This POST has **no request body, query fields, If-Match or Idempotency-Key**. It succeeds with 201 only if the pairing is approved, not expired and unconsumed. Response:

```json
{"actorId":"actor-device","deviceId":"device-a","credentialId":"credential-a","credentialSecret":"<one-time-secret>"}
```

The `credentialSecret` is delivered **exactly once** and the server persists only one-way verifier material. A second request returns `409 pairing_state_conflict` rather than issuing another secret. If the response is lost after commit, there is **no way to recover the original secret**; a new administrative pairing/recovery process is required. Never invent a blind retry or log this response.

## 5. Authenticate as this Device

Use HTTPS with the accepted MU.10D header:

```http
GET /api/v1
Authorization: VDR-Suite-Device <credentialId>.<credentialSecret>
```

This is **not HTTP Bearer**, not Browser Basic and not the Pairing Token. The authenticated Device principal starts with no grants. Backend, channel, recording and playback access require **separately administered Device-targeted grants**. Human `accounts/{id}/grants` is not a shortcut for Device grants. Use the [Device grant administration contract](device-administration.md) for explicit scoped permissions. Revoke/rotation use their own Device administration authority; do not store credentials in URLs, query strings, logs or screenshots.

For the VIDAA client, a Device authentication or HTTP 404 in a media-track route must be diagnosed against the actual path, HTTP method, installed daemon SHA and grant requirements; neither successful pairing nor a working Web frontend proves Device route acceptance. A merged server route alone does not prove real Hisense runtime behavior.

## HTTP outcomes

| HTTP | Client meaning |
| --- | --- |
| `201` | Pairing created, or exactly one credential issued (depending on endpoint) |
| `200` | Token-scoped pairing state successfully read; not Device authentication |
| `400` / `415` / `422` | Invalid query/JSON/media type/metadata |
| `401` | Missing or wrong bootstrap token where required |
| `404` | Request missing/invalidated |
| `409 pairing_state_conflict` | Approval missing or credential already consumed |
| `410 pairing_expired` / `pairing_consumed` | Expired or terminal bootstrap request |
| `503` | Required verifier, entropy, persistence or audit service unavailable |

## Implementation and tests

- [Public pairing routes and response serializers](rest/src/PublicApiRuntime.cpp)
- [Pairing security classification](../core/security/include/SecurityHttpGate.h)
- [REST pairing tests](rest/tests/test_public_device_pairing.cpp)
- [Security pairing tests](../core/security/tests/test_public_device_pairing_security.cpp)
- [JavaScript pairing reference tests](../clients/reference-js/tests/test_public_v1_device_pairing_client.js)

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)
