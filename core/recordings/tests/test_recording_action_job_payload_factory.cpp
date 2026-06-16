#include "RecordingActionJobPayloadFactory.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionRequest request;
    request.backendId = "living-room";
    request.recordingId = "recording-1";
    request.type = RecordingActionType::Move;
    request.dryRun = false;
    request.parameters["targetPath"] = "/video/archive";

    RecordingActionValidationResult validation;
    validation.valid = true;
    validation.dryRun = false;
    validation.wouldCreateJob = true;
    validation.backendId = "living-room";
    validation.recordingId = "recording-1";
    validation.requiredCapabilities.push_back("recordings.action.move");
    validation.requiredPermissions.push_back("recordings.action.move");
    validation.warnings.push_back("validated for execution");

    RecordingActionJobPayloadFactory factory;

    const RecordingActionJobPayload payload =
        factory.create(request, validation);

    assert(payload.backendId == "living-room");
    assert(payload.recordingId == "recording-1");
    assert(payload.type == RecordingActionType::Move);
    assert(payload.jobType == "recording.move");
    assert(!payload.dryRun);
    assert(payload.parameters.at("targetPath") == "/video/archive");
    assert(payload.requiredCapabilities.size() == 1);
    assert(payload.requiredCapabilities.at(0) == "recordings.action.move");
    assert(payload.requiredPermissions.size() == 1);
    assert(payload.requiredPermissions.at(0) == "recordings.action.move");
    assert(payload.warnings.size() == 1);
    assert(payload.warnings.at(0) == "validated for execution");

    RecordingActionRequest unknownRequest;
    unknownRequest.backendId = "remote-house";
    unknownRequest.recordingId = "recording-2";
    unknownRequest.type = RecordingActionType::Unknown;
    unknownRequest.dryRun = true;

    RecordingActionValidationResult unknownValidation;
    unknownValidation.valid = false;
    unknownValidation.warnings.push_back("dry-run only");

    const RecordingActionJobPayload unknownPayload =
        factory.create(unknownRequest, unknownValidation);

    assert(unknownPayload.backendId == "remote-house");
    assert(unknownPayload.recordingId == "recording-2");
    assert(unknownPayload.type == RecordingActionType::Unknown);
    assert(unknownPayload.jobType == "recording.unknown");
    assert(unknownPayload.dryRun);
    assert(unknownPayload.warnings.size() == 1);
    assert(unknownPayload.warnings.at(0) == "dry-run only");

    return 0;
}
