#include "RecordingActionRequest.h"
#include "RecordingActionUtils.h"
#include "RecordingActionValidationResult.h"

#include <cassert>
#include <iostream>

int main()
{
    auto action = RecordingActionType::Shrink;
    auto text = toString(action);

    assert(text == "SHRINK");

    auto parsed = fromString(text);
    assert(parsed == RecordingActionType::Shrink);

    assert(toString(RecordingActionType::Move) == "MOVE");
    assert(toString(RecordingActionType::Delete) == "DELETE");
    assert(toString(RecordingActionType::MetadataRefresh) == "METADATA_REFRESH");

    assert(fromString("MOVE") == RecordingActionType::Move);
    assert(fromString("DELETE") == RecordingActionType::Delete);
    assert(fromString("METADATA_REFRESH") == RecordingActionType::MetadataRefresh);
    assert(fromString("DOES_NOT_EXIST") == RecordingActionType::Unknown);

    RecordingActionRequest request;
    request.backendId = "default";
    request.recordingId = "recording-001";
    request.type = RecordingActionType::Move;
    request.dryRun = true;
    request.parameters["targetPath"] = "/srv/vdr/video/archive";

    assert(request.backendId == "default");
    assert(request.recordingId == "recording-001");
    assert(request.type == RecordingActionType::Move);
    assert(request.dryRun);
    assert(request.parameters.at("targetPath") == "/srv/vdr/video/archive");

    RecordingActionValidationResult validation;
    validation.valid = true;
    validation.dryRun = true;
    validation.wouldCreateJob = false;
    validation.backendId = request.backendId;
    validation.recordingId = request.recordingId;
    validation.requiredCapabilities.push_back("recordings.action.move");
    validation.requiredPermissions.push_back("recordings.action.move");
    validation.warnings.push_back("dry-run only");

    assert(validation.valid);
    assert(validation.dryRun);
    assert(!validation.wouldCreateJob);
    assert(validation.backendId == "default");
    assert(validation.recordingId == "recording-001");
    assert(validation.requiredCapabilities.size() == 1);
    assert(validation.requiredCapabilities.at(0) == "recordings.action.move");
    assert(validation.requiredPermissions.size() == 1);
    assert(validation.requiredPermissions.at(0) == "recordings.action.move");
    assert(validation.warnings.size() == 1);
    assert(validation.errors.empty());

    RecordingActionValidationResult failedValidation;
    failedValidation.valid = false;
    failedValidation.dryRun = true;
    failedValidation.wouldCreateJob = false;
    failedValidation.backendId = "default";
    failedValidation.recordingId = "";
    failedValidation.errors.push_back("recordingId is required");

    assert(!failedValidation.valid);
    assert(failedValidation.dryRun);
    assert(!failedValidation.wouldCreateJob);
    assert(failedValidation.errors.size() == 1);
    assert(failedValidation.errors.at(0) == "recordingId is required");

    std::cout << "Recording action validation model OK" << std::endl;

    return 0;
}
