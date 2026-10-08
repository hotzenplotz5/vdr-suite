# MU.10E2 — Canonical Device and Device-Credential Revocation

Status: **implementation candidate**; exact-product-head automated acceptance and
real yaVDR runtime acceptance are not yet complete.

Base: merged MU.10E1 PR #442, `main` at
`94bda169f63049a1b53e5c6e6dd6e968d5d784b1`.

## Purpose and canonical authorities

Administrator-controlled revocation targets the existing
`security_devices` or `security_credentials` row. Credential targets
must be of type `device-app`, owned by the same canonical Service Actor
as the Device and joined through an existing
`security_device_credential_verifiers` binding. No second identity
registry, session system or alternative verifier authority is created.

A Device revoke causes **all credentials bound to that Device to fail
authentication** via the existing MU.10D canonical Device state check.
An individual Credential revoke invalidates that credential only.
The original Device and Credential metadata, grant history and append-only
accountability history remain available; no implicit reissue is permitted.

These operations do not revoke the Human Account that approved pairing,
and do not inherit its grants. They do not add or remove permission grants
from the Device Service Actor; already-revoked Devices cannot authenticate
regardless of grants. Reconnecting after revocation requires a separately
authorized future lifecycle or a fresh approved pairing, not silent renewal.

## Public-v1 API contract

```http
GET  /api/v1/devices/{deviceId}/lifecycle
POST /api/v1/devices/{deviceId}/lifecycle

GET  /api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle
POST /api/v1/devices/{deviceId}/credentials/{credentialId}/lifecycle
```

Both GET routes are administrator-only with the global
`devices.lifecycle.view@*` permission (also satisfied by the existing
`role.admin@*`). Responses contain the canonical Device ID, Actor ID,
optional Credential ID, `active`, `revoked` and a strong ETag; **no
credential secret**. GET supports `If-None-Match` and 304.

POST is a revoke-only operation. The Device route requires
`devices.revoke@*`, and the Credential route
`devices.credentials.revoke@*`; the existing global
`role.admin@*` satisfies these. Browser Session CSRF enforcement
applies. The request body must be exactly an empty JSON object:

```http
Content-Type: application/json
If-Match: "<strong ETag from GET>"

{}
```

The ETag is based on a canonical lifecycle revision for that exact
resource, either `device-lifecycle:{deviceId}:active|revoked` or
`device-credential-lifecycle:{credentialId}:active|revoked`.
A mismatched current active revision is rejected with 412. Repeating
a revoke on the **same already revoked resource** succeeds without
reissuance, including with the formerly active revision.

Operations are transactionally fenced with `BEGIN IMMEDIATE`,
use the canonical `SecurityIdentityRepository` and write
append-only accountability in the same transaction. A failed audit
insert rolls back the revoke. Failure modes are 401 (not
authenticated), 403 (no admin authorization / CSRF), 404 (target
not found/owned by another device), 428 (missing If-Match),
400/415/422 (malformed request), 412 (changed active revision),
503 (authority/storage/audit unavailable). Unsupported verbs receive
405 with an explicit GET/POST allowance.

## Client and tests

The existing Public-v1 reference client adds
`getDeviceLifecycle`, `revokeDevice`,
`getDeviceCredentialLifecycle` and `revokeDeviceCredential` using
caller-supplied Browser Session CSRF and `If-Match`. No Device
credential itself can authorize these admin operations.

Focused tests cover the immutable ownership/binding fence,
valid and stale revisions, Credential-only revoke, whole-Device
revoke, repeated desired-state revoke, authentication denial,
audit rollback, API ETag/304/428/412, CSRF and admin access,
and JavaScript request transport.

Out of scope for MU.10E2: Credential rotation, device listing/search,
Administrator UI, live VIDAA pairing and real yaVDR installation. Those
remain explicit successor slices. No TV prerequisite applies.
