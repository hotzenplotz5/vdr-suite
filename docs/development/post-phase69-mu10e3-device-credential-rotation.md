# MU.10E3 — Device credential rotation

Status: **Draft candidate; not yet accepted.** Depends on MU.10E2 merged at
`f2e363797300ab5e1b8bba0aaba8c14579c0c1f9`.

## Resource and trust boundary

`POST /api/v1/devices/{deviceId}/credentials/{credentialId}/rotate` is an
administrator-browser-only operation. It accepts exactly `{}`, a strong
`If-Match` for the **active** credential lifecycle ETag and the existing
browser CSRF token. No query parameters or `Idempotency-Key` are accepted.
The existing SecurityHttpGate must authorize
`devices.credentials.rotate` over the canonical administrator role before
the route can execute. Device credential auth cannot elevate to admin.

The rotation keeps the same canonical Service Actor and Device, and creates a
new `device-app` credential with `rotated_from_credential_id` pointing at
the previous credential. Grants remain attached to that same Actor and are
neither duplicated nor inherited from the approving Administrator.

The response contains the new `credentialId` and `credentialSecret`
**only on successful rotation**, with `Cache-Control: no-store` and
`Pragma: no-cache`. Normal read endpoints, logs, audit and URLs never
expose the secret. It is a 32-byte OS-random secret, hex encoded; only a
per-secret crypt SHA-512 rounds=10000 verifier is stored.

## Atomicity and replay behavior

The writer lease and `BEGIN IMMEDIATE` serialize rotation and revocation.
The service rechecks canonical Actor/Device/Credential/verifier ownership and
active lifecycle inside the transaction. The existing
`SecurityIdentityRepository::rotateCredentialInActiveTransaction` creates
the replacement and revokes its predecessor. Verifier insert, append-only
accountability and commit are part of the same transaction.

A second POST with the same prior credential is **not** a replay success:
the prior credential is revoked. The service returns a conflict and issues no
secret. A browser must not automatically retry a rotation request after
network uncertainty.

If the response is lost **after** commit, the previous credential has already
been invalidated. The server never recovers, persists or replays the plaintext
secret. Treat the new secret as lost. Recovery requires an explicit new
administrator-controlled pairing/re-provisioning workflow for a replacement
device identity and explicit reapplication of grants. Do not silently
re-enable the predecessor or mint another credential on a replay.

Rotating an expired or revoked credential or one on a revoked Device fails
closed. If hashing, storage, audit append or commit fails, the transaction
must roll back. No productive yaVDR changes or VIDAA acceptance are part of
MU.10E3. TV acceptance remains MU.10F.

## Test status

The focused make target is `test-security-device-credential-rotation`.
Security service, Public-v1 API and gate, and reference-JS client tests are
registered there. CI and local compilation of the **final exact head**
must be recorded before Ready-for-review; this Draft must not be merged
without separate authorization.
