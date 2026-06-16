#include "RecordingActionValidationService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    RecordingActionValidationService service;

    RecordingActionRequest moveRequest;
    moveRequest.backendId = "default";
    moveRequest.recordingId = "recording-001";
    moveRequest.type = RecordingActionType::Move;
    moveRequest.dryRun = true;
    moveRequest.parameters["targetPath"] = "/srv/vdr/video/archive";

    RecordingActionValidationResult moveResult =
        service.validate(moveRequest);

    assert(moveResult.valid);
    assert(moveResult.dryRun);
    assert(!moveResult.wouldCreateJob);
    assert(moveResult.backendId == "default");
    assert(moveResult.recordingId == "recording-001");
    assert(moveResult.requiredCapabilities.size() == 1);
    assert(moveResult.requiredCapabilities.at(0) == "recordings.action.move");
    assert(moveResult.requiredPermissions.size() == 1);
    assert(moveResult.requiredPermissions.at(0) == "recordings.action.move");
    assert(moveResult.warnings.size() == 1);
    assert(moveResult.warnings.at(0) == "dry-run only");
    assert(moveResult.errors.empty());

    RecordingActionRequest executionRequest = moveRequest;
    executionRequest.dryRun = false;

    RecordingActionValidationResult executionResult =
        service.validate(executionRequest);

    assert(executionResult.valid);
    assert(!executionResult.dryRun);
    assert(executionResult.wouldCreateJob);
    assert(executionResult.warnings.empty());

    RecordingActionRequest missingBackend = moveRequest;
    missingBackend.backendId = "";

    RecordingActionValidationResult missingBackendResult =
        service.validate(missingBackend);

    assert(!missingBackendResult.valid);
    assert(missingBackendResult.errors.size() == 1);
    assert(missingBackendResult.errors.at(0) == "backendId is required");

    RecordingActionRequest missingRecording = moveRequest;
    missingRecording.recordingId = "";

    RecordingActionValidationResult missingRecordingResult =
        service.validate(missingRecording);

    assert(!missingRecordingResult.valid);
    assert(missingRecordingResult.errors.size() == 1);
    assert(missingRecordingResult.errors.at(0) == "recordingId is required");

    RecordingActionRequest unknownRequest = moveRequest;
    unknownRequest.type = RecordingActionType::Unknown;

    RecordingActionValidationResult unknownResult =
        service.validate(unknownRequest);

    assert(!unknownResult.valid);
    assert(unknownResult.errors.size() == 1);
    assert(unknownResult.errors.at(0) == "recording action type is required");
    assert(unknownResult.requiredCapabilities.empty());
    assert(unknownResult.requiredPermissions.empty());

    RecordingActionRequest missingMoveTarget = moveRequest;
    missingMoveTarget.parameters.erase("targetPath");

    RecordingActionValidationResult missingMoveTargetResult =
        service.validate(missingMoveTarget);

    assert(!missingMoveTargetResult.valid);
    assert(missingMoveTargetResult.errors.size() == 1);
    assert(
        missingMoveTargetResult.errors.at(0) ==
        "targetPath is required for move");

    RecordingActionRequest renameRequest;
    renameRequest.backendId = "default";
    renameRequest.recordingId = "recording-002";
    renameRequest.type = RecordingActionType::Rename;
    renameRequest.dryRun = true;
    renameRequest.parameters["newName"] = "Evening News";

    RecordingActionValidationResult renameResult =
        service.validate(renameRequest);

    assert(renameResult.valid);
    assert(renameResult.requiredCapabilities.at(0) == "recordings.action.rename");
    assert(renameResult.requiredPermissions.at(0) == "recordings.action.rename");

    RecordingActionRequest missingRenameName = renameRequest;
    missingRenameName.parameters.erase("newName");

    RecordingActionValidationResult missingRenameNameResult =
        service.validate(missingRenameName);

    assert(!missingRenameNameResult.valid);
    assert(missingRenameNameResult.errors.size() == 1);
    assert(
        missingRenameNameResult.errors.at(0) ==
        "newName is required for rename");

    RecordingActionRequest deleteRequest;
    deleteRequest.backendId = "default";
    deleteRequest.recordingId = "recording-003";
    deleteRequest.type = RecordingActionType::Delete;
    deleteRequest.dryRun = true;

    RecordingActionValidationResult deleteResult =
        service.validate(deleteRequest);

    assert(deleteResult.valid);
    assert(deleteResult.requiredCapabilities.at(0) == "recordings.action.delete");
    assert(deleteResult.requiredPermissions.at(0) == "recordings.action.delete");

    std::cout
        << "test_recording_action_validation_service passed"
        << std::endl;

    return 0;
}
