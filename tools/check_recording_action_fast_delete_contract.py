#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
actions = (ROOT / "web/frontend/recordings2-actions.js").read_text(encoding="utf-8")
daemon = (ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp").read_text(encoding="utf-8")

delete_start = actions.index("    function createDeleteEditor(recording) {")
delete_end = actions.index("\n    function createPanel(recording) {", delete_start)
delete_block = actions[delete_start:delete_end]

required_delete = (
    "global.confirm(",
    "execute(",
    "'DELETE'",
    "{finishOnBackendSuccess: true}",
)
for token in required_delete:
    if token not in delete_block:
        raise SystemExit(f"missing fast delete contract token: {token}")

for forbidden in (
    "Papierkorb prüfen",
    "validate(recording, 'DELETE'",
    "dryRun: true",
    "deleteReadback(recording)",
):
    if forbidden in delete_block:
        raise SystemExit(f"slow/multi-step delete contract returned: {forbidden}")

if delete_block.count("global.confirm(") != 1:
    raise SystemExit("recording delete must require exactly one browser confirmation")

callback_start = daemon.index(
    "recordingActionExecutionController_->setAfterSuccessfulExecutionCallback("
)
callback_end = daemon.index(
    "\n\n    recordingActionRequestPreviewService_ =",
    callback_start,
)
callback = daemon[callback_start:callback_end]

for token in (
    "recordingCacheRefreshQueue_.request(backendId, 8);",
    "externalVdrChangeHint_.store(true);",
):
    if token not in callback:
        raise SystemExit(f"missing asynchronous recording refresh handoff: {token}")

for forbidden in (
    "buildRecordings()",
    "snapshotCacheService_->updateRecordingsForBackend",
    "vdrRecordingCacheRepository_->replaceRecordingsForBackend",
    "vdrRecordingCacheRepository_->markRefreshFinished",
):
    if forbidden in callback:
        raise SystemExit(f"synchronous recording rebuild returned to action response path: {forbidden}")

print("recording delete fast single-confirm contract ok")
