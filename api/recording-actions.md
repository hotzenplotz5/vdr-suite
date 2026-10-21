# Public API v1 — Recording Actions

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

