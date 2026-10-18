# Public-v1 Device revocation and credential rotation — developer contract

## Navigation

- [API developer entry](README.md)
- [OpenAPI 3.1 selected contract](openapi.json)
- [TV pairing and one-time initial credential](device-pairing.md)
- [Device administrator approval/grants](device-administration.md)
- [MU.10E2 Device/Credential revocation source audit](../docs/development/post-phase69-mu10e2-device-revocation.md)
- [MU.10E3 Credential rotation source audit](../docs/development/post-phase69-mu10e3-device-credential-rotation.md)

## Trust and runtime boundaries

Cross-checked against `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11), including PublicApiRuntime, SecurityHttpGate and regression tests. Historical MU.10E2/MU.10E3 design pages have candidate-status headers even though their route implementations are present on current main. This guide is **source-level verification**, **not proof of real yaVDR/VIDAA acceptance** for a particular build.

A Device belongs to its canonical non-Human Service Actor; the Human Account that approved pairing does not become its owner. Grant history stays attached to the Device's Actor when credentials are revoked or rotated; it never grants administrator authority to the Device.

## Read lifecycle and opaque ETags

```http
GET /api/v1/devices/{deviceId}/lifecycle
GET /api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle
```

Both require authenticated `devices.lifecycle.view@*`. Successful 200 responses have:

```json
{"deviceId":"device-a","actorId":"actor-device-a","credentialId":"","active":true,"revoked":false}
```

For the Credential-level resource, `credentialId` is that actual credential ID instead of an empty string. Strong opaque lifecycle ETags arrive as headers, not raw lifecycle revision fields; matching `If-None-Match` returns 304 with an empty body. Neither route returns credentials or verifier secrets.

## Revoke one Credential or the entire Device

```http
POST /api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle
Content-Type: application/json
If-Match: <strong Credential lifecycle ETag>

{}
```

Requires global `devices.credentials.revoke@*`.

```http
POST /api/v1/devices/{deviceId}/lifecycle
Content-Type: application/json
If-Match: <strong Device lifecycle ETag>

{}
```

Requires global `devices.revoke@*`. Both are protected administrator mutations (Browser Session CSRF when applicable) with exactly an empty `{}` body, max 4096 bytes, no query or Idempotency-Key. Successful 200 returns `active:false`, `revoked:true` with new ETag. An already-revoked target may be revoked again without reissuing authority. A real stale active-state revision yields 412; missing `If-Match` yields 428.

**Credential revoke** affects only that credential. **Device revoke** makes every credential bound to the Device fail subsequent authentication. Revocation does not delete grant history, reactivate identities or automatically re-pair. The original administrator Human Account is unaffected.

## Rotate one ACTIVE Device credential

```http
POST /api/v1/devices/{deviceId}/credentials/{credentialId}/rotate
Content-Type: application/json
If-Match: <strong ACTIVE Credential lifecycle ETag>

{}
```

**Administrator Browser Session required**, plus `devices.credentials.rotate@*` and Browser Session CSRF. A Device Credential cannot rotate itself merely because it authenticates Public-v1. The ETag must be for the predecessor Credential in its **active** lifecycle state; no query/Idempotency-Key, max 4096 bytes, closed `{}`.

Successful 200 returns **exactly once**:

```json
{"deviceId":"device-a","actorId":"actor-device-a","credentialId":"credential-new","credentialSecret":"<one-time-secret>"}
```

The predecessor is revoked atomically, the replacement has a fresh opaque secret, and grants stay on the same Service Actor. The new secret is not recoverable via GET, database lookup, logs, audit or an idempotent retry. Server response includes `Cache-Control: no-store`, `Pragma: no-cache`.

**A second POST with the old Credential is not a replay.** It returns `412 revision_conflict`, without secret. If the response after commit is lost, the old secret is already invalid and the new secret unrecoverable. An administrator must initiate explicit recovery/re-pairing; never silently regenerate a successor or re-enable the predecessor.

## Errors and client implementation

- `400`: invalid precondition/query/Idempotency-Key; `401`: unauthenticated; `403`: insufficient global admin grant, Browser Session or CSRF; `404`: incorrect Device/Credential binding.
- `412`: conflicting lifecycle revision, including post-rotation replay; `415`: Content-Type failure; `422`: invalid closed body; `428`: missing If-Match; `503`: authoritative verifier/storage/audit failure.
- [Device lifecycle REST tests](rest/tests/test_public_device_lifecycle.cpp)
- [Device rotation REST tests](rest/tests/test_public_device_credential_rotation.cpp)
- [Device rotation security gate regression](../core/security/tests/test_public_device_rotation_security.cpp)
- [Reference JS client](../clients/reference-js/public-v1-client.js)

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)
