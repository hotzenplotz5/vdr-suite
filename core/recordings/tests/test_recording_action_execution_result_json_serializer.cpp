#include "RecordingActionExecutionResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    RecordingActionExecutionResult result;
    result.success = true;
    result.type = RecordingActionType::Move;
    result.backendId = "default";
    result.recordingId = "recording-001";
    result.message = "accepted for execution";
    result.warnings.push_back("backend execution not implemented yet");

    RecordingActionExecutionResultJsonSerializer serializer;

    const std::string json = serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"type\":\"MOVE\"") != std::string::npos);
    assert(json.find("\"backendId\":\"default\"") != std::string::npos);
    assert(json.find("\"recordingId\":\"recording-001\"") != std::string::npos);
    assert(json.find("\"message\":\"accepted for execution\"") != std::string::npos);
    assert(json.find("\"warnings\":[\"backend execution not implemented yet\"]") != std::string::npos);
    assert(json.find("\"errors\":[]") != std::string::npos);

    RecordingActionExecutionResult failed;
    failed.success = false;
    failed.type = RecordingActionType::Delete;
    failed.backendId = "backend\\one";
    failed.recordingId = "recording\"quoted";
    failed.message = "validation failed";
    failed.errors.push_back("recordingId contains \"quote\"");

    const std::string failedJson = serializer.serialize(failed);

    assert(failedJson.find("\"success\":false") != std::string::npos);
    assert(failedJson.find("\"type\":\"DELETE\"") != std::string::npos);
    assert(failedJson.find(R"("backendId":"backend\\one")") != std::string::npos);
    assert(failedJson.find(R"("recordingId":"recording\"quoted")") != std::string::npos);
    assert(failedJson.find(R"("errors":["recordingId contains \"quote\""])") != std::string::npos);

    std::cout
        << "test_recording_action_execution_result_json_serializer passed"
        << std::endl;

    return 0;
}
