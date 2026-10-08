# MU.10D1 — Device Credential Authentication (Public-v1)

Status: **implementation candidate; automated final-head and real yaVDR acceptance pending**.

Base: merged MU.10C1 PR #440, main bbc2d5734c74e37a57b87b609afe57b9b2e8f4bb.
Working branch: work/mu10d-device-authentication.

## Existing authorities

MU.10C1 issues a fresh opaque high-entropy credential secret exactly once
after approved pairing. The canonical `security_actors`,
`security_devices`, and `security_credentials` rows remain the identity
authority; `security_device_credential_verifiers` stores a one-way salted
SHA-512-crypt verifier bound to a canonical credential and device.

MU.10D1 authenticates **these existing device-app credentials** on the
existing Public-v1 HTTP security gate. It does not reissue a credential,
reopen pairing, create a Browser Session, create a Device Session or grant
any permissions.

## Wire contract

Requests presenting a MU.10C1 device credential send:

```http
GET /api/v1
Authorization: VDR-Suite-Device <credentialId>.<credentialSecret>
```

The credential identifier and secret are the opaque values returned on the
successful one-time issuance response. Clients should use HTTPS and keep
the credential secret out of URLs, logs, telemetry and screenshots; do not
reuse the short-lived pairing token here. Device authenticated requests
should not attach a Browser Session cookie. The scheme applies only to
`/api/v1` and descendants; legacy unversioned API routes reject it.

Each request performs verifier lookup, `crypt_r` hash verification and
canonical binding check (credential type `device-app`, matching Service
Actor/Device/Credential). `PersistentIdentityResolver` then checks the
current canonical active/revoked/expired state, without issuing a Session.
The current active grants are resolved through
`SecurityPermissionGrantRepository`, with no implicit device permissions.

`GET /api/v1` reports `authentication.authenticated = true` for a
valid device credential. A missing credential leaves the anonymous
contract root public. Wrong or expired credentials receive HTTP 401;
a valid device identity without the required resource-specific grant
receives HTTP 403. If grant persistence cannot be resolved, device
authentication fails closed with HTTP 503. Credential secret material
is never returned by the authentication or error response.

## MU.10D1 acceptance and boundaries

The security regression must prove valid device authentication, zero
implicit grants, protected-route denial, tampered/wrong credential
failure, bootstrap-token non-substitution, malformed headers, expired
credentials, revoked credential/device/actor, mismatched verifier binding,
grant-persistence failure and rejection on legacy HTTP routes.

The reference JavaScript Public-v1 client already supports caller-supplied
headers and `credentials: 'omit'`; no second client authentication
transport is required. The device client can set the above Authorization
header when calling `getApiRoot()` (the verified exported reference-client method).
Use a fresh device-scoped client instance rather than mixing browser or
pairing traffic with durable device credentials.

MU.10E governs administrative permission grants, credential rotation and
revocation UI. MU.10F covers actual Hisense VIDAA pairing and TV client
acceptance. There is **no TV prerequisite** for MU.10D1 repository tests.
Do not deploy or restart the productive yaVDR daemon as part of this slice
without the repository's real-runtime preflight/staged-deployment gates.
