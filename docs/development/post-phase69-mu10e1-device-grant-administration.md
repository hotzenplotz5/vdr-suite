# MU.10E1 — Device Grant Administration (Public-v1)

Status: **implementation candidate; exact-head CI and real yaVDR acceptance not yet complete**.

Base: MU.10D1 merged on `main` as PR #441, commit `a7383e2450a33fe56dc0a360a62fd9e204a29bcb`.
Implementation PR: #442 (`work/mu10e1-device-grant-administration`).

## Canonical authority and trust boundary

Only canonical `security_actors`, `security_devices`,
`security_credentials`, `security_device_credential_verifiers`
and `security_actor_permission_grants` are used. The successful MU.10C
pairing issuance creates a dedicated Service Actor with **zero grants**.
Device grants belong to that Service Actor, never the approving Human
Account. This slice does not provision Actors or Credentials, open
Sessions, or change Device authentication.

The target must be an existing active non-revoked Device owned by an
active non-revoked Service Actor with a canonical `device-app` Credential
verifier binding. Unknown, revoked or non-device targets cannot receive
grants.

For MU.10E1 the explicitly allowed grants are only:

- `channels.view`;
- `timers.view`;
- `media.live.play`;
- `media.recording.play`.

The backend scope is an existing bounded product backend identifier or
`*`. The allowlist delegates its tuple validation to the **existing**
`HumanAccountGrantAdministrationService::supportedGrant`; Device
administration may not grant `role.admin`, `role.read-only`,
`accounts.*`, `devices.grants.modify`, `remote.control` or any other
unauthorized permission. No administrator rights are inherited from the
Human Account that approved pairing.

## Public-v1 contract

```http
GET  /api/v1/devices/{deviceId}/grants
POST /api/v1/devices/{deviceId}/grants
```

GET is administrator-only (`devices.grants.view@*`, also satisfied by
the existing `role.admin@*`). It returns an ETag-fenced secret-free
normalized grant set, the target Device and Service Actor IDs, and the
supported Device permission vocabulary. `If-None-Match` supports
`304 Not Modified`.

POST is administrator-only (`devices.grants.modify@*`, likewise
satisfied by `role.admin@*`) and uses existing authenticated Browser
Session CSRF enforcement. It requires `Content-Type: application/json`
and the **strong** `If-Match` ETag from a previous GET:

```json
{"permission":"channels.view","backendId":"default","active":true}
```

The JSON body is closed. `active:true` means ensure; `active:false`
means revoke. The original MU.7 desired-state semantics are reused:
stale revisions fail with `412` unless the requested final state is
already present, where the operation completes successfully. Grant-set
revisions use the existing SHA-256 normalized effective grant
computation; they do not rewrite canonical Account or Device revisions.
Writes use a `BEGIN IMMEDIATE` transaction, existing grant repository,
and append-only accountability event in the same transaction. Audit
failures prevent successful mutation commits.

Policy is fail-closed: anonymous/invalid credentials `401`, missing
global administrative authority or Browser CSRF `403`, missing target
`404`, malformed input or precondition `400`/`415`/`422`/`428`,
real stale grant-set mutation `412`, unavailable authorities `503`.
Resource permissions themselves never authorize device grant mutation.

## Reference client

`getDeviceGrants({deviceId, ...})` and
`setDeviceGrant({deviceId, permission, backendId, active, ifMatch, ...})`
are added to the established Public-v1 JavaScript reference client,
transporting the existing `If-Match` and caller-supplied Browser CSRF.
No separate credential storage or authentication stack is created.

## Acceptance boundaries

Focused regressions: canonical-only Device validation; zero starting
grants; allowlist denial of administrative permissions; desired-state
ensure/revoke and stale ETag handling; append-only accountability;
Browser Admin authorization and CSRF; Public-v1 GET/ETag/304 and POST
validation; reference-client request transport; subsequent Device
credential authentication resolves the newly granted effective scope
and is blocked after Device revocation.

MU.10E1 does **not** provide Device listing, Credential rotation,
Device/Credential revocation endpoints or Administrator UI. Those are
explicit successor slices MU.10E2+ before the real VIDAA MU.10F
acceptance. No TV or real yaVDR installation is required for MU.10E1
repository tests. Merge and live deployment remain separately gated.
