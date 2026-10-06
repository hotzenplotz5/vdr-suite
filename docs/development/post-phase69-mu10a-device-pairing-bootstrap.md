# MU.10A — Device Pairing Bootstrap

Status: **IMPLEMENTATION CANDIDATE — HOSTED ACCEPTANCE PENDING**

Authority:
[MU.10 Device/App Pairing Architecture and Gap Audit](post-phase69-mu10-device-app-pairing-audit.md).

MU.10A is the smallest productive Device/app pairing slice. It creates no
durable Device identity and does not authenticate ordinary Public-v1 requests.

## Public-v1 resources

### Create Pairing Request

```http
POST /api/v1/device-pairings
Content-Type: application/json
```

Authentication state: **anonymous bootstrap**.

Request semantics:

```json
{
  "displayName": "Hisense 43A6K",
  "clientKind": "vidaa",
  "appVersion": "phase1"
}
```

`displayName` and `clientKind` are required bounded presentation metadata.
`appVersion` is optional. Unknown fields are rejected.

Success is **201 Created** with `Location` pointing to the Pairing Request
item. The response contains the server-generated `pairingRequestId`, a short
human `userCode`, a high-entropy short-lived `pairingToken`,
`status = pending`, absolute `expiresAt`, `pollIntervalSeconds = 3`,
the non-secret client metadata and an item self link.

There is no ETag/If-Match contract and no Idempotency-Key contract in MU.10A.
A client that loses the one-time create response starts a new short-lived
Pairing Request rather than guessing/recovering bootstrap secrets.

### Poll Pairing Request

```http
GET /api/v1/device-pairings/{pairingRequestId}
X-VDR-Suite-Pairing-Token: <short-lived opaque pairing token>
```

Authentication state: **not authenticated as an Actor**. The polling token
authorizes only this one bootstrap resource. It is not accepted as
authentication for Backends, Channels, TimerAssignments, Operations, Accounts
or any other Public-v1 resource.

Success is **200 OK** with non-secret Pairing Request metadata, `pending`
state, expiry and polling interval. The poll response never repeats
`userCode` or `pairingToken`.

## Permissions and escalation boundary

MU.10A requires no normal Account permission because it is the anonymous
bootstrap entry point. That does **not** create authorization. MU.10A creates
zero grants and does not create or activate an Actor, Device, Credential or
Session.

Therefore:

```text
Pairing Request
  != Device Identity
  != Authentication
  != Permission Grant
```

Ordinary Public-v1 resources keep their existing authenticated Actor and
server-side grant requirements.

## Errors

- `400 invalid_request`: malformed request/path/query or unsupported
  conditional/idempotency input;
- `401 unauthorized`: missing or incorrect Pairing polling token;
- `404 not_found`: unknown or invalidated Pairing Request;
- `405 method_not_allowed`: collection is POST-only; item is GET-only;
- `410 pairing_expired`: Pairing Request lifetime ended;
- `415 invalid_request`: create is not `application/json`;
- `422 validation_error`: closed request shape or presentation metadata is
  invalid;
- `503 service_unavailable`: entropy, hashing, storage or runtime authority is
  unavailable.

Public-v1 request/correlation evidence, `Cache-Control: no-store` and Problem
Details conventions are retained.

## Bootstrap persistence and secret handling

The existing VDR-Suite security database receives one additive table,
`security_device_pairing_requests`. It stores only Pairing Request identity,
one-way `user_code_hash`, one-way `pairing_token_hash`, bounded presentation
metadata, state, expiry and lifecycle timestamps.

Clear `userCode` and `pairingToken` exist only in the create response and
short-lived process memory. Entropy comes from `getrandom()`; verifier storage
reuses the repository's SHA-512-crypt-compatible one-way pattern.

Creation records append-only accountability evidence as
`device_pairing.requested`. Pairing Request persistence and its creation audit
event commit atomically; failure is fail-closed.

## ETag, revision and idempotency

MU.10A has no ETag/If-Match semantics and no durable mutation replay contract.
The resource is deliberately short-lived bootstrap state. Approval will be a
separate administrative mutation and must select its own revision/precondition
contract from the live authority when MU.10B is implemented.

## Credential lifecycle

There is **no Device Credential in MU.10A**. Credential issuance, persistence,
rotation, authentication and revocation are intentionally not faked here.

Those concerns remain successor work:

```text
MU.10A  Pairing Request + polling
  -> MU.10B  administrative approval
  -> MU.10C  Device identity / durable Credential issuance
  -> MU.10D  Device-authenticated Public-v1 access
  -> MU.10E  Device/Credential revoke integration
```

The exact successor slicing remains subject to fresh repository truth.

## VIDAA first-consumer status

The shared Public-v1 reference client exposes:

- `createDevicePairing(...)`;
- `getDevicePairing(...)`.

It performs no automatic polling, retry, Browser Session fallback or Basic
fallback. A VIDAA client may own its timer/UI behavior explicitly.

No code in `hotzenplotz5/vdr-suite-vidaa` is changed by MU.10A.

Once this candidate is available on the real yaVDR runtime, a real TV test is
already meaningful for:

```text
VIDAA starts Pairing
  -> real POST /api/v1/device-pairings
  -> TV displays real userCode
  -> TV performs token-scoped GET
  -> status remains pending
  -> expiry/error behavior is visible
```

A test of administrator approval, persistent authentication after TV restart or
server-side revoke is **not** meaningful yet because MU.10B-D do not exist.

## Explicitly outside MU.10A

- administrator pending-request list and approval;
- Actor/Device binding;
- Device Credential or Session issuance;
- Device-authenticated Public-v1 requests;
- automatic administrator/backend/content grants;
- Credential rotation/revoke or Device revoke;
- browser administration UI;
- VIDAA application code;
- Phase 70.

## Candidate acceptance

Focused repository acceptance is:

```text
make test-security-device-pairing-request
python3 tools/check_phase69_public_api_inventory.py
```

No daemon build, installation, restart or runtime mutation is required for this
candidate. No PR or merge is authorized by this document.
