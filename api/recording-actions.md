# Public API v1 — Recording Actions

> **Latest runtime/hardware acceptance (2026-10-21):** At installed
> source `13e63d277deb5c9e0230f757a8c5733f6e7b0cbb`, the Public-v1
> flat Recording collection uses the ready backend Recording cache and
> returned 1,037 items before testing. One explicitly approved Brisant
> Recording passed Human Account/CSRF, RENAME/ MOVE/ DELETE Validate and
> Dry-run, exactly one successful real execution for each action, and
> independent native/public-ID readbacks. Final operator result:
> `API_V1_RECORDING_HW_ACCEPTANCE=PASS`. The source is now deleted;
> never re-use its `rec_` identity. An initial DELETE-stage timeout stopped
> *before* a real DELETE POST, and a separate authorized DELETE-only attempt
> later succeeded. No blind mutation retry occurred. This is bounded host
> acceptance, not a durable crash/replay guarantee or blanket destructive
> permission. See [Real-Hardware Closeout](../docs/development/public-v1-recording-r2r3-hardware-acceptance-20261021.md).
>
> All older R1/R2 source-only and transitional notes below are historical
> and superseded for the completed bounded R2/R3 runtime acceptance.

> **R2/R3 implementation source checkpoint (2026-10-21, `6b388c2c`):**
> This newer source contract supersedes all older "R1 only / writes return
> 409" descriptions below. Verify exact-head GitHub CI and a separately
> approved disposable real-yaVDR fixture before calling R2/R3 accepted.
> `POST /api/v1/recordings/actions/validate` accepts the opaque public
> `rec_<32hex>` and action RENAME, DELETE or MOVE (MOVE requires an existing
> backend-scoped `targetFolderId: fld1_<32hex>`; native `targetPath` is
> rejected). RENAME requires `newName`. With a provider-safe executable
> preview the validation response includes an opaque strong `ETag`
> derived from exact backend/public recording/action/target and the
> provider's Recording/Timer revision counters. It does not expose native
> paths, service recording IDs or upstream payloads.
>
> Real `execute` requires `dryRun:false`, the identical action and
> target, and exactly one matching strong `If-Match`. Missing=428,
> malformed/weak=400, changed=412. DELETE additionally requires the
> literal JSON boolean `confirmDelete:true`. Only the already-registered
> RESTfulAPI SafeMutation adapter may preview and execute. The adapter
> independently rechecks both provider revision counters immediately
> before its native validate/execute stage; mismatch stops before the
> mutation endpoint. One execution attempt only, **no automatic retry**.
>
> After provider 2xx, a separate live `/recordings.json` readback must
> show confirmed absence for DELETE, or a unique matching relocated source
> for RENAME/MOVE. The persistent Suite `rec_` identity is then retired or
> rebound exclusively through the existing `PublicRecordingIdentityRepository`.
> Only verified readback returns HTTP 200 with public `recordingId`,
> `executed:true` and `readbackVerified:true`. Failures/ambiguous
> outcomes are sanitized `application/problem+json` responses (especially
> HTTP 409: **do not retry automatically**). No native path, backend
> response body or source-service ID is ever serialized by these public
> results. The private/legacy action endpoints and their DTOs are
> unchanged. Authorization is the existing exact backend-scoped recording
> grant and browser CSRF (no anonymous or legacy credential bypass).
>
> **Safety/acceptance debt:** Source and mock tests are not a real Recording
> hardware acceptance. The previous disposable yaVDR fixture was deleted.
> Never test real RENAME/DELETE/MOVE on an arbitrary productive Recording.
> Ambiguous native outcomes lack an actor-owned durable Operation/replay
> protocol; do not treat HTTP 409 as evidence that the mutation did not
> happen. Hardware acceptance with a newly authorized disposable fixture
> and durable idempotency policy review remain independent gates.


> **R2 initial safe-precondition slice, 2026-10-21:** For the public-v1
> `execute` route with `dryRun:false`, missing `If-Match` returns
> HTTP 428. Malformed/weak/wildcard/multiple entity tags are refused with
> HTTP 400. A syntactically valid strong tag currently still returns 409:
> cache-only Recording observations do **not** prove an authoritative
> write revision, and no productive mutation can be dispatched.
> A public MOVE request always returns 409 pending R3's public logical
> destination; `targetPath` (native or otherwise) is rejected as a
> Public-v1 body field. RENAME's `newName` accepts only one name
> component, not directory/path separators. Validate with
> `dryRun:false` also returns 409 until write preflights are connected.
> No change to any private/legacy action route.


> **R1 source checkpoint (2026-10-21):** Public-v1 `validate` and `execute` now
> require a backend-scoped `rec_<32hex>` identity from the Public Recording
> collection. The runtime rejects client-provided native ID/path/title fields
> and unknown input keys, reusing the existing
> `PublicRecordingPlaybackTargetResolver` and cache for server-side lookup.
> Both public entry points require the action's scoped permission and browser
> CSRF. Public-v1 `execute` only accepts `dryRun:true` in R1: it performs
> local validation without a backend dispatch and reports `executed:false`.
> Real public-v1 mutation returns HTTP 409 pending R2/R3 revisions and
> verified identity rebind. Private/legacy action routes remain unchanged.
> The historical real-VDR acceptance below predates this R1 request shape.
>
> **Transitional documentation:** Original sections immediately below describe
> the previous v1-as-Legacy-alias behavior and remain for historical context;
> this checkpoint takes precedence for the current v1 behavior.


The public-v1 Recording Action entry points reuse the already deployed VDR-Suite recording action controllers and execution service. They do not create a second mutation stack or change the Web frontend's private routes.

## HTTP surface

- `POST /api/v1/recordings/actions/validate` — validate the action request without changing a Recording.
- `POST /api/v1/recordings/actions/execute` — execute through the existing Recording Action execution controller. The execution controller validates the request before backend dispatch. Set `dryRun: true` for non-mutating execution.

Both operations use the same request DTO and JSON responses as the existing `/api/vdr/recordings/actions/validate` and `/api/vdr/recordings/actions/execute` routes. The public-v1 entry points currently share these responses; they are **not yet** the separately audited, stable Public-v1 Problem Details/OpenAPI contract. Do not infer a general v1 mutation compatibility guarantee from this initial integration.

## Request

```json
{
  "backendId": "default",
  "recordingId": "REPLACE-WITH-AN-EXISTING-ID",
  "action": "RENAME",
  "newName": "Example title",
  "dryRun": true
}
```

Supported action-specific parameters remain governed by the existing Recording Action parser/executor: `newName` for RENAME, `targetPath` for MOVE, and the recording identity/path data needed by the chosen backend. `DELETE` requires deliberate user confirmation at the client. Do not assume the public Recording collection ID alone is accepted as a backend-native action identity.

## Permissions

The existing SecurityHttpGate applies to these routes:

- Validation: authenticated caller; no backend mutation.
- Execution: backend-scoped `recordings.rename`, `recordings.move`, or `recordings.delete` according to `action`; unsupported action values are refused by the existing gate.
- Browser-authenticated execution: existing CSRF token verification and mutation accountability remain in force.
- Dry-run does not bypass the execution-route permission check.

Live destructive tests must only use a dedicated disposable test recording. The normal Web frontend continues to use its established private API wrappers unchanged.

## Implementation boundaries

- Routing: `api/rest/src/ApiRouter.cpp`
- Authorization: `core/security/include/SecurityHttpGate.h`
- Existing execution stack: `RecordingActionExecutionController` and `RecordingActionExecutionService`
- Source contract: [Recording Action Execution API](../docs/architecture/recording-action-execution-api.md)


## Real yaVDR acceptance — 2026-10-21

A single explicitly authorized disposable VDR recording (`Report Mainz`,
2026-10-20 21:45, recording directory
`2026-10-20.21.45.1-0.rec`) was exercised through the **public-v1**
validate and execute endpoints on the real yaVDR deployment.

Observed results:

| Step | Result | Evidence |
| --- | --- | --- |
| Human Account login and browser CSRF | PASS | Session issued by `POST /api/security/browser-sessions`; authenticated v1 requests accepted. |
| RENAME | PASS | `status=renamed`, RESTfulAPI readback confirmed `/srv/vdr/video/VDR-SUITE-TEST-API-V1-RENAMED/2026-10-20.21.45.1-0.rec`. |
| MOVE | PASS | `status=moved`, RESTfulAPI readback confirmed the recording under `/srv/vdr/video/VDR-SUITE-TEST-API-V1-MOVED/VDR-SUITE-TEST-API-V1-RENAMED/2026-10-20.21.45.1-0.rec` and absent at the source. |
| DELETE (safe trash) | PASS | `status=trashed`, RESTfulAPI readback confirmed the moved recording absent from the active recording list. |

Final operator output: `API_V1_RECORDING_ACCEPTANCE=PASS`. **The fixture
was deleted by the completed test; do not rerun this acceptance against the
same recording.** No additional recordings were approved as fixtures.

Important runtime observations:

- The RESTfulAPI `number`/recording ID was not stable across operations or
  refreshes (observed 282, 1036 and 587). Discover the current ID again from
  the **exact** `file_name` before each action. Preserve the backend-native
  absolute path as the action identity. A public-v1 Recording collection ID
  must not be substituted for it.
- An initially successful MOVE preview/validation was followed by an
  upstream HTTP 409 on `/recordings/move.json`; **no move was applied** in
  that attempt. Read-only follow-up showed source present, target absent,
  executable preview, ready validation and stable revision values. A later
  explicitly initiated MOVE attempt succeeded, followed by DELETE. The root
  cause of the one 409 was **not established**; this result does not justify
  bypassing safety gates or asserting that a code fix was deployed.
- The suite deliberately returns `success:false` and `dry-run only` for
  successful backend preview dry-runs. Validate the upstream preview's
  `executable:true`, HTTP 200 and absence of errors instead of treating
  the suite's `success:false` as a rejected dry-run.
- Browser session cookies carry the `Secure` attribute. Python's standard
  cookie jar will not replay them on loopback HTTP; the authorized acceptance
  client used the issued cookie explicitly on requests confined to
  `127.0.0.1:18080`. HTTPS clients should preserve normal Secure-cookie
  behavior.
- All three public-v1 action routes share the existing VDR-Suite action
  controllers and RESTfulAPI backend adapter with the Web-private action
  routes. This acceptance proves **these three actions on this deployment**,
  not a fully audited public-v1 error/compatibility contract.

