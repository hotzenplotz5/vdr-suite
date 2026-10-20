# Public API v1 — real yaVDR Recording Action acceptance

## Scope

One **dedicated, disposable** VDR test recording exercises the new public-v1 routes via the unchanged existing Recording Action controllers. This is not Timer API v1 acceptance. Do not use production recordings.

Branch: `work/public-v1-recording-actions`. Before running, record exact branch HEAD and deployed server version; the source branch existing on GitHub is not proof that yaVDR runs it.

## Fixture

Create a short, separate, identifiable VDR recording whose recording title contains `VDR-SUITE-TEST`. Read it from the live recording collection and capture its **actual** `recordingId`, `backendNativeId` (absolute native `.rec` directory) and `backendId`. Confirm that it is no longer recording. Never synthesize or modify recording identity, never choose a real recording by positional index, and never run DELETE without affirming the exact final test-fixture identity.

Use a unique rename title such as `VDR-SUITE-TEST-API-V1-RENAMED`, and a test-only target folder `VDR-SUITE-TEST-API-V1-MOVED`. No filesystem-level moves or deletes.

## HTTP requests (existing DTO)

Each of the three actions uses the same sequence:

1. `POST /api/v1/recordings/actions/validate` with `dryRun: true`; require `valid: true`.
2. `POST /api/v1/recordings/actions/execute` with `dryRun: true`; inspect the established dry-run response, **not** a fabricated `success: true` requirement (backend dispatch is deliberately skipped).
3. Only if explicitly authorized for this dedicated fixture: `POST /api/v1/recordings/actions/execute` with `dryRun: false`. Apply existing backend-scoped permission and Browser CSRF requirements.
4. Re-read the live Recording collection, confirm result and refresh **both** IDs before proceeding. If ambiguous or unsuccessful, STOP and do not issue the next mutation or automatically retry the POST.

### RENAME

```json
{"backendId":"default","recordingId":"<CURRENT_ID>","backendNativeId":"<CURRENT_ABSOLUTE_REC_PATH>","action":"RENAME","newName":"VDR-SUITE-TEST-API-V1-RENAMED","dryRun":true}
```

Require the old display name to be replaced by the new name in the follow-up read. Capture the new recording and native ID.

### MOVE

```json
{"backendId":"default","recordingId":"<REFRESHED_ID>","backendNativeId":"<REFRESHED_ABSOLUTE_REC_PATH>","action":"MOVE","targetPath":"VDR-SUITE-TEST-API-V1-MOVED","dryRun":true}
```

Require source location absent and the same fixture present in the test target folder, with fresh identifiers. `targetPath` is a target **folder**, not a complete filename; the existing backend request builder preserves the recording leaf. Check the actual backend semantics before executing.

### DELETE

```json
{"backendId":"default","recordingId":"<REFRESHED_ID>","backendNativeId":"<REFRESHED_ABSOLUTE_REC_PATH>","action":"DELETE","dryRun":true}
```

Require exact fixture confirmation before real DELETE; after the action, the fixture must no longer be present in live recordings. Do not treat a successful HTTP response without readback as a PASS.

## Evidence and pass criteria

Record exact deployed commit, HTTP status + redacted JSON for each validate/dry-run/execute, initial/current/final recording identities and three readback outcomes. Do not log credentials or cookies.

```text
API_V1_RECORDING_RENAME=PASS|FAIL|NOT_RUN
API_V1_RECORDING_MOVE=PASS|FAIL|NOT_RUN
API_V1_RECORDING_DELETE=PASS|FAIL|NOT_RUN
API_V1_RECORDING_HW_ACCEPTANCE=PASS|FAIL|NOT_RUN
```

An actual PASS requires three live action executions and their independent readbacks. CI green, preview success or historical pre-v1 real-VDR evidence is not an API-v1 hardware pass. On failure, retain the existing fixture state for diagnosis and **do not restart the sequence**.
