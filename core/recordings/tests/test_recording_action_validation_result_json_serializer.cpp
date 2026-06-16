#include "RecordingActionValidationResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    RecordingActionValidationResult result;
    result.valid = true;
    result.dryRun = true;
    result.wouldCreateJob = false;
    result.backendId = "default";
    result.recordingId = "recording-001";
    result.requiredCapabilities.push_back("recordings.action.move");
    result.requiredPermissions.push_back("recordings.action.move");
    result.warnings.push_back("dry-run only");

    RecordingActionValidationResultJsonSerializer serializer;

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"valid\":true") != std::string::npos);
    assert(json.find("\"dryRun\":true") != std::string::npos);
    assert(json.find("\"wouldCreateJob\":false") != std::string::npos);
    assert(json.find("\"backendId\":\"default\"") != std::string::npos);
    assert(json.find("\"recordingId\":\"recording-001\"") != std::string::npos);
    assert(json.find("\"requiredCapabilities\":[\"recordings.action.move\"]") != std::string::npos);
    assert(json.find("\"requiredPermissions\":[\"recordings.action.move\"]") != std::string::npos);
    assert(json.find("\"warnings\":[\"dry-run only\"]") != std::string::npos);
    assert(json.find("\"errors\":[]") != std::string::npos);

    RecordingActionValidationResult failed;
    failed.valid = false;
    failed.dryRun = true;
    failed.wouldCreateJob = false;
    failed.backendId = "default";
    failed.recordingId = "";
    failed.errors.push_back("recordingId is required");

    const std::string failedJson =
        serializer.serialize(failed);

    assert(failedJson.find("\"valid\":false") != std::string::npos);
    assert(failedJson.find("\"errors\":[\"recordingId is required\"]") != std::string::npos);

    RecordingActionValidationResult escaped;
    escaped.valid = false;
    escaped.dryRun = true;
    escaped.backendId = "backend\\\\one";
    escaped.recordingId = "recording\\\"quoted";
    escaped.errors.push_back("path contains \\\"quote\\\"");

    const std::string escapedJson =
        serializer.serialize(escaped);

    assert(escapedJson.find(R"("backendId":"backend\\\\one")") != std::string::npos);
    assert(escapedJson.find(R"("recordingId":"recording\\\"quoted")") != std::string::npos);
    assert(escapedJson.find(R"("errors":["path contains \\\"quote\\\""])") != std::string::npos);

    std::cout
        << "test_recording_action_validation_result_json_serializer passed"
        << std::endl;

    return 0;
}
