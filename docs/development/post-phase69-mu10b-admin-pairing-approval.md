# MU.10B — Administrator Pairing Approval

Status: **IMPLEMENTATION CANDIDATE — PR #437 / HOSTED CI #9790 GREEN / REAL YAVDR ACCEPTANCE PENDING**

Authority:
- [ADR-0065 Human Account, Profile and Device Identity Boundary](../adr/ADR-0065-human-account-profile-device-identity-boundary.md)
- [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)
- [MU.10 Device/App Pairing Architecture and Gap Audit](post-phase69-mu10-device-app-pairing-audit.md)
- [MU.10A Device Pairing Bootstrap](post-phase69-mu10a-device-pairing-bootstrap.md)

## Scope

MU.10B adds explicit administrator approval or rejection to the existing
short-lived MU.10A Pairing Request. It changes only Pairing Request lifecycle
state.

It does **not** create an Actor, Device, Credential, Session or Permission
Grant. Durable Device identity and credential issuance remain MU.10C.

## Administrative read contract

Authenticated administration reuses the existing Public-v1 Pairing Request
resource:

```http
GET /api/v1/device-pairings
GET /api/v1/device-pairings/{pairingRequestId}
```

The collection returns pending, non-expired Pairing Requests with bounded
keyset pagination. The item read returns the secret-free administrative
representation and a strong ETag derived from the persisted Pairing Request
revision.

Administration requires the explicit global permission:

```text
device.pairing.view@*
```

The existing `AuthorizationService` lets `role.admin@*` satisfy that
permission. A backend-scoped administrator grant does not become global
Pairing administration authority.

The Pairing bootstrap token is never accepted as administrator authority.

## Decision contract

```http
POST /api/v1/device-pairings/{pairingRequestId}
If-Match: "<strong Pairing Request ETag>"
Content-Type: application/json

{"decision":"approve"}
```

or:

```json
{"decision":"reject"}
```

The mutation requires:

```text
device.pairing.decide@*
```

`role.admin@*` may satisfy the permission through the existing
`AuthorizationService` role expansion. Because this is registered as a
protected mutation permission, an effective `role.read-only@*` continues to
fail the mutation through the normal mutation-policy path.

Browser-session mutations retain the central SecurityHttpGate CSRF check.

## Revision and lifecycle rules

Approval/rejection requires the caller's observed strong ETag via
`If-Match`.

The authoritative transaction fails closed when the Pairing Request is:

- unknown or invalidated;
- expired;
- already approved or rejected;
- no longer at the observed revision.

A successful decision:

1. requires current state `pending`;
2. records `approved` or `rejected`;
3. records the authenticated deciding User Actor;
4. records the decision timestamp;
5. advances the persisted Pairing Request revision;
6. appends secret-free Accountability evidence in the same transaction.

Repeated or conflicting decisions do not create a second decision or silently
overwrite a newer revision.

## Client poll after decision

The existing MU.10A token-scoped request remains:

```http
GET /api/v1/device-pairings/{pairingRequestId}
X-VDR-Suite-Pairing-Token: <short-lived pairing token>
```

After a valid administrator decision it may expose only the bounded Pairing
Request state:

```text
pending | approved | rejected
```

It does not expose the deciding Actor, administrator session data, permissions,
credentials or bootstrap verifier material.

`approved` means only that an administrator accepted this bootstrap request.
It is **not** a Device identity, authentication credential or permission grant.

## Accountability

The decision path records the authenticated Actor and the explicit
`device.pairing.decide` permission, Pairing Request/revision, decision,
outcome and conflict/denial reason without recording user codes, pairing
tokens, password material, browser cookies or CSRF secrets.

## Explicitly outside MU.10B

- durable Device creation;
- Actor-to-Device binding;
- Device Credential issuance or rotation;
- Device-authenticated Public-v1;
- automatic permission/backend grants;
- Device/Credential revoke;
- Profiles;
- VIDAA persistent sign-in.

Those remain successor work, beginning with MU.10C.

## Acceptance gate

Before merge:

- branch must remain based on current `main` with no lost main history;
- focused MU.10A/MU.10B security/API tests pass;
- architecture/docs guards pass;
- all hosted PR CI jobs pass;
- real yaVDR canonical stage/deploy identity is verified before restart;
- real HTTP acceptance proves pending list/item, ETag/If-Match,
  approve/reject, expired/already-decided conflicts and token-poll status;
- SQLite/accountability evidence proves revision + deciding Actor/timestamp
  without any Device/Credential/Session/Grant creation.

VIDAA changes are not required merely to complete server-side MU.10B. A client
acceptance update is justified only after the server contract is live and
stable.
