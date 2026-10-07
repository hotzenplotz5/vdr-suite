# MU.10A — Device Pairing Bootstrap

Status: **COMPLETED — PR #436 MERGED / REAL YAVDR + REAL VIDAA ACCEPTANCE PASS / POST-MERGE CI #9779 GREEN**

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
- `429 rate_limited`: the active anonymous Pairing Request capacity is
  exhausted; active `pending` rows are capped at 256;
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

Anonymous bootstrap storage is bounded: at most 256 non-invalidated,
non-expired `pending` requests may be active at once. CREATE prunes requests
that have been expired for at least one hour before checking that capacity.
The one-hour retention preserves the established `410 pairing_expired`
diagnostic window while preventing unbounded expired-row accumulation.

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

Focused real-yaVDR acceptance passed on candidate
`75e9f65790d7a2cc5bbcf8cfbeb22c1738ad0e39`, based directly on
`main` `5199bfadff4f2219f623fa5ce841d5b6139a026c`.

The temporary detached worktree ran:

```text
git diff --check origin/main...HEAD
make --output-sync=target test-security-device-pairing-request
make --output-sync=target test-test-http-server
make --output-sync=target test-api-router
python3 tools/check_phase69_public_api_inventory.py
python3 tools/check_mu10a_device_pairing_bootstrap.py
python3 tools/check_multiuser_admin_ux_guidance.py
python3 tools/check_phase_consistency.py
```

All focused gates returned zero and the final marker was
`MU10A_FOCUSED_ACCEPTANCE=PASS`.

Safety evidence:

```text
DAEMON_BUILD=NO
INSTALLATION=NO
RESTART=NO
RUNTIME_MUTATION=NO
VIDAA_REPO_CHANGED=NO
PR_CREATED=NO
MERGE_PERFORMED=NO
```

The shell returned to the normal yaVDR prompt after the test. Later failed
`sudo su` authentication attempts occurred after acceptance had completed and
are unrelated to MU.10A.

At this candidate checkpoint no PR had yet been created; the later integration closeout below supersedes that transient repository-state note.
No PR or merge authorization is implied by this document.


## Real runtime deployment acceptance

The same MU.10A implementation was subsequently staged through the canonical
`stage-install-runtime -> deploy-install-runtime` path and restarted on the
real yaVDR host.

Observed runtime evidence:

```text
RUNTIME_DEPLOYMENT_MATCH=YES
live daemon sha256=88b1ca53f81c3b8b314bcd50042b9afa5e1772a52d5d4e0dd0f1f957e9b8a3d2
old daemon pid=29570
new daemon pid=49547
POST /api/v1/device-pairings -> 201
token-scoped GET -> 200
wrong pairing token -> 401
anonymous GET /api/v1/backends -> 401
pairing row -> pending
user_code_hash length -> 119
pairing_token_hash length -> 119
both verifier prefixes -> $6$rounds=10000$
```

A first acceptance wrapper reported FAIL only because its diagnostic SQL used
`substr(...,1,14)` and then compared that deliberately truncated value with
the 16-character `$6$rounds=10000$` prefix. The corrected direct SQLite
verification read the full verifier strings and passed. This was a test-wrapper
bug, not a runtime or persistence defect.

No PR or merge was performed.


## Real VIDAA first-consumer acceptance closeout

The physical Hisense VIDAA client completed the bounded MU.10A journey through
the same-origin acceptance host without AppInfo reinstall.

Observed evidence:

```text
MU10A_VIDAA_APP_LOAD=PASS
MU10A_VIDAA_PAIRING_ACTION=PASS
MU10A_VIDAA_REAL_CODE_VISIBLE=PASS
MU10A_VIDAA_PAIRING_STATUS_VISIBLE=PASS
MU10A_VIDAA_LAYOUT_READABLE=PASS
MU10A_VIDAA_NO_REINSTALL=PASS
POLL_COUNT=10
UNIQUE_POLLED_REQUESTS=1
MU10A_VIDAA_PENDING_POLL=PASS
PAIRING_TOKEN_SCOPE_REQUEST_PATH=CONSISTENT
RESULT=MU10A_VIDAA_PAIRING_ACCEPTANCE_PASS
```

The final five observed GETs were spaced at the server-advertised three-second
cadence and all targeted the same Pairing Request resource with HTTP 200.

MU.10A is therefore accepted as the anonymous short-lived bootstrap slice.
It still creates no Actor, Device, Credential, Session or Grant. The selected
successor is MU.10B administrative approval; durable Device identity and
credential issuance remain MU.10C.


## Merge and hosted CI closeout

MU.10A subsequently merged to `main` as PR #436 with merge commit
`10c37c60d2fb957dd6089d7503b57373c37ba624`. Post-merge main CI run #9779
(run ID `37570884570`) completed successfully on that exact commit with all six
repository jobs green. This final integration state supersedes the earlier
candidate-time notes above that correctly recorded no PR or merge at those
earlier acceptance checkpoints.
