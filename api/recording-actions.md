# Public API v1 — Recording Actions

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
