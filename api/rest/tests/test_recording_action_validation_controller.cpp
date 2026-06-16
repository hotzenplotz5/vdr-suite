#include "RecordingActionValidationController.h"

#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionValidationService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    RecordingActionValidationService validationService;
    RecordingActionValidationResultJsonSerializer jsonSerializer;
    RecordingActionValidationRequestParser requestParser;

    RecordingActionValidationController controller(
        validationService,
        jsonSerializer);

    RecordingActionRequest request;
    request.backendId = "default";
    request.recordingId = "recording-001";
    request.type = RecordingActionType::Move;
    request.dryRun = true;
    request.parameters["targetPath"] = "/srv/vdr/video/archive";

    ApiResponse response =
        controller.validate(request);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"valid\":true") != std::string::npos);
    assert(response.body.find("\"dryRun\":true") != std::string::npos);
    assert(response.body.find("\"wouldCreateJob\":false") != std::string::npos);
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"recordingId\":\"recording-001\"") != std::string::npos);
    assert(response.body.find("\"requiredCapabilities\":[\"recordings.action.move\"]") != std::string::npos);
    assert(response.body.find("\"requiredPermissions\":[\"recordings.action.move\"]") != std::string::npos);
    assert(response.body.find("\"warnings\":[\"dry-run only\"]") != std::string::npos);
    assert(response.body.find("\"errors\":[]") != std::string::npos);

    RecordingActionRequest invalidRequest;
    invalidRequest.backendId = "default";
    invalidRequest.recordingId = "";
    invalidRequest.type = RecordingActionType::Delete;
    invalidRequest.dryRun = true;

    ApiResponse invalidResponse =
        controller.validate(invalidRequest);

    assert(invalidResponse.statusCode == 200);
    assert(invalidResponse.contentType == "application/json");
    assert(invalidResponse.body.find("\"valid\":false") != std::string::npos);
    assert(invalidResponse.body.find("\"errors\":[\"recordingId is required\"]") != std::string::npos);

    RecordingActionValidationController bodyController(
        validationService,
        jsonSerializer,
        requestParser);

    ApiResponse bodyResponse =
        bodyController.validateBody(
            "{"
            "\"backendId\":\"default\","
            "\"recordingId\":\"recording-005\","
            "\"action\":\"MOVE\","
            "\"dryRun\":true,"
            "\"targetPath\":\"/srv/vdr/video/archive\""
            "}");

    assert(bodyResponse.statusCode == 200);
    assert(bodyResponse.contentType == "application/json");
    assert(bodyResponse.body.find("\"valid\":true") != std::string::npos);
    assert(bodyResponse.body.find("\"recordingId\":\"recording-005\"") != std::string::npos);
    assert(bodyResponse.body.find("\"requiredCapabilities\":[\"recordings.action.move\"]") != std::string::npos);

    ApiResponse missingParserResponse =
        controller.validateBody("{}");

    assert(missingParserResponse.statusCode == 500);
    assert(missingParserResponse.contentType == "application/json");
    assert(missingParserResponse.body.find("request parser unavailable") != std::string::npos);

    std::cout
        << "test_recording_action_validation_controller passed"
        << std::endl;

    return 0;
}
