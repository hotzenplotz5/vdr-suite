# MU.10 — Device/App Pairing Architecture and Gap Audit

Status: **LIVE AUDIT COMPLETE — MU.10A IMPLEMENTATION CANDIDATE / REAL YAVDR RUNTIME PASS**

Audited against `main` `5199bfadff4f2219f623fa5ce841d5b6139a026c`.
There were no open pull requests. Exact-head normal CI run `37500705794`
completed successfully. The latest scheduled Full Regression failure belongs to
older SHA `436d677a...`, not this baseline.

PR #433 (post-MU.9 administration usability) and PR #434 (device-list UX) are
merged although the volatile status documents still call the usability slice
`CANDIDATE`. That is predecessor documentation drift, not Pairing authority.

## Existing authority

### Device persistence

`SecurityIdentityRepository` already owns canonical `security_devices`.
A Device is bound to exactly one Actor and has display, active and revocation
state. `PersistentIdentityResolver` re-resolves it on authenticated requests
and fails closed on missing ownership or revocation.

This is the final Device identity authority. MU.10 must not add another Device
database. Missing are Pairing lifecycle, public Device administration and
accepted Device ownership/policy semantics beyond the generic Actor binding.

### Credentials

`security_credentials` is the canonical Credential lifecycle authority. It
already owns type, active/expiry/revocation state and rotation ancestry.

Secret material is deliberately specialized: Human/Managed Basic verifiers are
one-way verifiers, Browser Session secrets are one-way hashes, and Backend
Agent enrollment proves a technical Actor/Device/Credential lifecycle. The
Agent's private protocol and Basic transport are not a client API.

MU.10 may add a verifier/binding table for a Device/App credential type only if
`security_credentials` remains the sole Credential lifecycle authority and no
reusable plaintext secret is persisted.

### Sessions

`security_sessions` already binds Session to Actor/Device and the persistent
resolver enforces ownership, expiry and revocation. Browser Session cookies,
issuer binding, CSRF, idle timeout and retention remain browser-specific and
must not become generic TV authentication.

### Human Accounts

Explicit Human Accounts and Account-to-Actor binding are productive. Pairing
must not turn a generic Actor into a Human Account and must not require a TV to
store a Human Account password.

### Permission grants

`security_actor_permission_grants`,
`SecurityPermissionGrantRepository` and `AuthorizationService` remain the
single authorization authority.

ADR-0067's current public/UI Grant administration is Human-Account-scoped. It
cannot silently be treated as Device grant administration. Any later
Device-targeted grant surface must delegate to the same grant repository,
server-owned permission allowlist and desired-state/revision semantics.

Pairing creates **zero grants**.

### Credential revocation

Canonical Credential revocation is present and fail-closed. MU.8C's public
Account Credential revoke intentionally accepts only `human-password`.
Device/App Credentials therefore need their own explicit revoke/rotation
contract; MU.8C must not be widened accidentally.

### Session revocation

Canonical Session revocation exists. MU.8B is specifically Account/Browser
Session administration. A Device/App Session needs explicit ownership and
administration semantics.

### Accountability

`accountability_events` is append-only. Security bootstrap mutations and
protected requests already retain Actor/Device/Session/Credential and decision
evidence. MU.10 create, approval, activation/rotation and revoke must retain
bounded audit evidence and fail closed if required audit persistence fails.

### Public-v1 authentication boundary

The general `SecurityHttpGate` currently authenticates Browser Session cookie
and optional Managed Basic before persistent identity resolution and
authorization. There is no Device/App credential authenticator.

That is the direct first-party TV blocker.

The live VIDAA Phase-1 branch independently confirms it: its stable transport
covers Public-v1 discovery, Backends, Channels, TimerAssignments and
Operations, while the installed runtime remains fixture-backed because
supported TV/app authentication and pairing do not yet exist.

## Reusable pieces

MU.10 directly reuses:

- canonical Actor/Device/Credential/Session persistence and revocation;
- existing one-way verifier patterns;
- `PersistentIdentityResolver` fail-closed lifecycle enforcement;
- the generic permission-grant repository and `AuthorizationService`;
- Human Account administration for humans;
- append-only accountability;
- Public-v1 problem/request/correlation/capability conventions;
- established ETag/If-Match/idempotency patterns where a concrete mutable
  resource needs them.

## Missing pieces

A real Device/App pairing stack still needs:

- short-lived Pairing Request persistence;
- human/QR bootstrap material with expiry;
- client polling proof scoped to one Pairing Request;
- authenticated Administrator pending-request read/approval;
- final Device principal/ownership policy;
- Device/App Credential verifier/binding and issuance;
- Device/App authentication in Public-v1;
- Device/App Session binding if that lifecycle uses one;
- Device-targeted grant administration over the existing grant authority;
- Credential rotation;
- Device/Credential/Session revoke integration;
- stable Device metadata for later Administrator recognition;
- real VIDAA end-to-end acceptance.

The earlier P1 audit had already named Device policy/ownership, short-lived
Pairing bootstrap and stable approval/read models as missing. Human Account and
Account-to-Actor gaps are now closed; those Device/Pairing gaps remain.

## Complete target lifecycle

```text
fresh TV/app
  -> create short-lived Pairing Request
  -> receive short-lived userCode + pairingToken
  -> show userCode / optional QR bootstrap
  -> poll only its own Pairing Request
  -> authenticated Administrator sees pending request
  -> Administrator approves identity only
  -> existing security authority creates/binds Actor + Device + Credential
  -> zero role/backend/content grants are created
  -> TV obtains/activates the long-lived Device credential
  -> Device authenticator yields Actor + Device + Credential
     (+ Device Session if accepted by the successor contract)
  -> PersistentIdentityResolver revalidates lifecycle
  -> AuthorizationService applies explicit grants
  -> restart: stored Device credential authenticates again
  -> rotation atomically replaces Credential and revokes predecessor
  -> Device/Credential/Session revoke fences later access
```

The human `userCode`, QR payload and `pairingToken` are bootstrap material
only. No durable Device credential may be embedded in the human code or QR.

## Final Device principal boundary

ADR-0065 says Device trust is not User identity or permission. Therefore
approval must not bind a TV to the approving Administrator merely because that
Administrator approved it.

The safe target is a dedicated non-Human device/application principal in the
existing Actor + Device authority, initially with zero grants. Existing
`ActorType::Service` is already a non-Human technical principal; the
credential-issuance successor must prove whether it is sufficient before
inventing a new Actor type.

This also prevents Pairing from inheriting the approver's Administrator role.

## Long-lived Device authentication

The target credential is an opaque high-entropy non-Basic credential. Only a
one-way verifier is persisted. Successful verification resolves the canonical
Actor, Device and Credential and then passes through
`PersistentIdentityResolver` and `AuthorizationService`.

Browser cookies/CSRF remain browser-only. Human Account passwords are never
stored in the TV.

The exact authorization scheme and verifier record belong to the
credential-issuance slice, not MU.10A.

## Rotation and revocation

Rotation must reuse the existing atomic Credential rotation ancestry. Revoke
must be independently enforceable at canonical Device, Credential and, when
present, Device Session layers. A high-level Device revoke may terminate all
three in one transaction, but enforcement remains the existing persistent
resolver.

Expired, denied or consumed Pairing Requests never authenticate to ordinary
Public-v1 resources.

## Administrator recognition

The later Device read model must be secret-free and include at least stable
Device ID, display name, client/application kind, paired timestamp, last-seen
where available, active/revoked state and active Credential/Session metadata.

Client-supplied labels/version strings are presentation hints. The
server-generated Device ID and security bindings are authoritative.

## Selected first productive slice — MU.10A

MU.10A owns only:

```http
POST /api/v1/device-pairings
GET  /api/v1/device-pairings/{pairingRequestId}
```

POST is intentionally unauthenticated because a fresh client has no Device
credential. It accepts bounded client presentation metadata, creates no Actor,
Device, Credential, Session or Grant, and returns a server-generated request
ID, short human `userCode`, high-entropy short-lived `pairingToken`,
`pending` status, absolute expiry, minimum poll interval and self link.

Only one-way hashes of bootstrap secrets are persisted. Pending requests are
bounded and expired bootstrap rows are cleanup candidates.

GET requires the matching short-lived `pairingToken` and exposes only that
single request's non-secret metadata/status. The bootstrap token confers no
Actor identity and cannot authorize any other Public-v1 resource.

MU.10A deliberately has no Administrator approval, final identity/credential,
Device authenticator, grant, durable credential, rotation/revoke mutation or
Browser CSRF requirement.

There is no ETag/If-Match contract in MU.10A because it exposes no competing
Administrator mutation yet. There is no Idempotency-Key replay contract:
losing the create response means creating a new short-lived request rather
than storing/replaying plaintext bootstrap secrets.

## Justified successor decomposition

- **MU.10A** — short-lived Pairing Request + client polling.
- **MU.10B** — authenticated Administrator pending-request read + approval.
- **MU.10C** — final Device principal + Device Credential/Session issuance.
- **MU.10D** — Device-credential authentication in Public-v1.
- **MU.10E** — Device grant/revoke/rotation administration over existing
  authorities.
- **MU.10F** — real VIDAA end-to-end acceptance.

Every successor must re-read live `main` and may be split further if needed.
Phase 70 remains not started.


## MU.10A implementation candidate

The bounded implementation is documented in
[MU.10A Device Pairing Bootstrap](post-phase69-mu10a-device-pairing-bootstrap.md).

MU.10A owns only short-lived Pairing Request creation and token-scoped polling.
It does not approve a Device, create Actor/Device/Credential/Session identity,
grant permissions or authenticate ordinary Public-v1 resources.


## MU.10A focused real-yaVDR acceptance

Candidate `75e9f65790d7a2cc5bbcf8cfbeb22c1738ad0e39` passed the
focused detached-worktree acceptance against `main`
`5199bfadff4f2219f623fa5ce841d5b6139a026c`.

The accepted checks covered the Device Pairing service/persistence contract,
Public-v1 Pairing routes, SecurityHttpGate isolation, TestHttpServer/API router
composition, reference-client seam, public route inventory, predecessor
administration-usability guard and numbered-phase consistency. The run ended
with `MU10A_FOCUSED_ACCEPTANCE=PASS`.

No daemon build, installation, restart, runtime mutation, VIDAA-repository
change, PR or merge occurred. Hosted PR CI remains pending until a PR is
explicitly authorized.


## Real runtime checkpoint

MU.10A is now running on the real yaVDR daemon through the canonical sealed
runtime deployment path. Deployment identity matched, the daemon restarted with
a new PID, Pairing creation/polling and wrong-token rejection passed, ordinary
anonymous Public-v1 Backend access remained fenced, and SQLite contained only
the expected one-way bootstrap verifier hashes.

The temporary diagnostic false negative caused by reading only 14 characters
of a 16-character verifier prefix has been classified and corrected; it does
not invalidate the runtime evidence.
