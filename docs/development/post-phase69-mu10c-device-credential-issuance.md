# MU.10C — Durable Device Identity and Credential Issuance Scope

Status: **ARCHITECTURE / FAILURE-MODE AUDIT — IMPLEMENTATION NOT YET ACCEPTED**

Base: merged MU.10B PR #437, main `eb25c7814e58b437cf418e6f7f95bd7760118b4c`.

## Existing authorities (do not duplicate)

- `SecurityIdentityRepository`: canonical `security_actors`, `security_devices`, `security_credentials`, `security_sessions`, revocation and credential-rotation ancestry.
- `SecurityIdentityProvisioningRepository::ensureTechnicalIdentity`: existing Actor + Device + Credential technical-principal provisioning, including `ActorType::Service`.
- `DevicePairingRequestRepository`: canonical short-lived bootstrap lifecycle. Currently `pending | approved | rejected` and revisioned administrator decisions; no consumption yet.
- `DevicePairingRequestService`: entropy, request-scoped pairing-token verifier, HTTP-independent lifecycle and accountability orchestration.
- `SecurityPermissionGrantRepository` / `AuthorizationService`: sole permission authority, never invoked to create grants by pairing.
- `PersistentIdentityResolver`: canonical Actor/Device/Credential and (where present) Session enforcement.

No second Identity/Device database, no Human Account binding, no approving administrator impersonation, no new Actor enum until a concrete `ActorType::Service` deficiency is proved.

## Selected implementation boundary: MU.10C1

Materialize one administrator-approved, unexpired, unconsumed Pairing Request to a **new dedicated non-human Service Actor**, bound Device and a non-Basic Device Credential, with only one-way server-side verifier material and **zero grants**.

All persistence — consumption fence, Actor, Device, Credential, verifier/binding and secret-free accountability — must be in one immediate SQLite transaction using existing authorities. A successful response contains a fresh opaque high-entropy Device secret at most once. Only the original pairing-token holder may call the bounded activation/issuance operation. Pending, rejected, expired, invalidated and consumed requests fail closed.

The bootstrap token must not authenticate other Public-v1 routes or become a Device Credential. Do not silently add Device authentication here: that belongs to MU.10D. Do not create Browser Session cookies, CSRF tokens or Human Account bindings.

## Pre-implementation failure model

1. **Concurrent activation / repeated polling:** SQLite `BEGIN IMMEDIATE` and a conditional approved + live + revision/consumption update serialize the winner. Exactly one transaction can create the identity and mark the request consumed; all later activation attempts return a deterministic terminal state with no secret.
2. **Failure before commit:** roll back every Actor/Device/Credential/verifier/consumption/accountability write. Original approved pairing may be retried while still live.
3. **Commit succeeds but HTTP response is lost:** a durable one-way verifier cannot reconstruct the secret. Never return a second generated credential for that consumed pairing. The user must initiate a fresh pairing after an explicit recovery path revokes/orphans no usable pre-existing identity. Define and test that recovery path before accepting the slice.
4. **Process crash / storage failure:** an incomplete transaction leaves no partially issued identity. A committed-but-unacknowledged credential cannot be reissued in plaintext.
5. **Expired approval:** approval does not extend the Pairing Request expiry. Consumption checks expiry inside the winning transaction.
6. **Accountability write failure:** fails the transaction, not just logging; no plaintext userCode, pairingToken or Device secret in events or HTTP diagnostics.

Important: choosing a `consumed` state, dedicated consumption timestamp, binding identifiers and a recovery/revocation mechanism requires schema- and migration-level validation before implementation. Ordinary token-scoped GET polling should remain backward-compatible, secret-free and must not itself accidentally consume the request.

## Session boundary / optional MU.10C2

No durable Device Session is justified at issuance alone. MU.10D may create or resolve a Device Session as part of actual Credential authentication if the canonical lifecycle requires it. Introduce MU.10C2 only if focused implementation proves a separately useful activation step; do not preemptively expand the scope.

## Implemented C1 server contract (candidate)

The implementation selects a narrow one-time endpoint, independent of normal
device authentication (reserved for MU.10D):

```http
POST /api/v1/device-pairings/{pairingRequestId}/credential
X-VDR-Suite-Pairing-Token: <short-lived pairing token>
```

The request has no JSON body and does not use a Human Account password,
Browser Session cookie, CSRF secret, or durable Device credential. The token
authorizes **only** this Pairing Request. Authorization re-checks the hash
inside the winning SQLite transaction after the initial HTTP gate.

A first successful response returns HTTP 201 with server-owned `actorId`,
`deviceId`, `credentialId`, and the one-time `credentialSecret`.
`Cache-Control: no-store` is required. Subsequent issuance attempts receive
a terminal HTTP 409 without any secret. Token-scoped GET polling after a
successful consumption returns HTTP 410 `pairing_consumed`, also secret-free.

The dedicated `ActorType::Service` Actor, Device, canonical `device-app`
Credential and `security_device_credential_verifiers` one-way binding are
persisted in one transaction with the Pairing Request revision/consumption
fence and append-only accountability. No permission-grant or Session write is
performed by the issuer.

If SQLite or accountability persistence fails, the transaction rolls back.
If the HTTP response is lost after commit, the secret cannot be recovered;
the bootstrap request stays consumed. A new pairing must be initiated;
an administrative cleanup policy for unclaimed credentials must be addressed
before final runtime acceptance, rather than silently creating a second
credential for the consumed request.

This is a candidate implementation, not real yaVDR or real VIDAA acceptance.
MU.10D remains responsible for actual device-credential authentication.

## Required acceptance

- Exactly-once issuance and canonical Actor/Device/Credential binding.
- One-way verifier only, secret emitted once; no stored recoverable credential.
- Zero grants, approving admin is not Device Actor, `ActorType::Service` reuse.
- Denied/expired/pending/replay/parallel requests fail closed.
- Transaction failure injection and commit-before-response-loss recovery are explicitly tested.
- Secret-free accountability, no Session creation unless proved necessary.
- MU.10D authenticated Public-v1, MU.10E grants/revoke/rotation administration and MU.10F real VIDAA acceptance remain separate.
- Real yaVDR preflight/postcheck and canonical stage/deploy gates before runtime acceptance; no large SQLite backup without proven headroom.

Phase 70 remains not started. No TV is needed for the C1 repository implementation.
