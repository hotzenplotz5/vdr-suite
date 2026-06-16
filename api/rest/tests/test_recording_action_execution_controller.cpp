#include "RecordingActionExecutionController.h"

#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationRequestParser.h"

#include <cassert>
#include <memory>
#include <string>

namespace
{
class TestBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit TestBackendExecutorAdapter(
        std::string backendIdValue)
        : backendIdValue_(backendIdValue)
    {
    }

    std::string backendId() const override
    {
        return backendIdValue_;
    }

    std::string backendType() const override
    {
        return "test-backend";
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;

        result.success = !payload.dryRun;
        result.type = payload.type;
        result.backendId = payload.backendId;
        result.recordingId = payload.recordingId;

        if (payload.dryRun)
        {
            result.message = "dry-run backend execution skipped";
        }
        else
        {
            result.message = "backend execution completed";
        }

        return result;
    }

private:
    std::string backendIdValue_;
};
}

int main()
{
    RecordingActionExecutionService executionService;
    RecordingActionExecutionResultJsonSerializer jsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<TestBackendExecutorAdapter>("living-room"));

    RecordingActionExecutionController controller(
        executionService,
        jsonSerializer,
        registry);

    RecordingActionRequest request;
    request.backendId = "living-room";
    request.recordingId = "recording-1";
    request.type = RecordingActionType::Delete;
    request.dryRun = false;

    const ApiResponse response =
        controller.execute(request);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find(""success":true") != std::string::npos);
    assert(response.body.find(""type":"delete"") != std::string::npos);
    assert(response.body.find(""backendId":"living-room"") != std::string::npos);
    assert(response.body.find(""recordingId":"recording-1"") != std::string::npos);
    assert(response.body.find(""message":"backend execution completed"") != std::string::npos);

    RecordingActionExecutionController controllerWithoutParser(
        executionService,
        jsonSerializer,
        registry);

    const ApiResponse parserMissingResponse =
        controllerWithoutParser.executeBody("{}");

    assert(parserMissingResponse.statusCode == 500);
    assert(parserMissingResponse.contentType == "application/json");
    assert(parserMissingResponse.body.find("request parser unavailable") != std::string::npos);

    RecordingActionValidationRequestParser requestParser;

    RecordingActionExecutionController bodyController(
        executionService,
        jsonSerializer,
        registry,
        requestParser);

    const std::string body =
        "{"
        "\"backendId\":\"living-room\","
        "\"recordingId\":\"recording-2\","
        "\"type\":\"move\","
        "\"dryRun\":true,"
        "\"parameters\":{"
        "\"targetPath\":\"/video/archive\""
        "}"
        "}";

    const ApiResponse bodyResponse =
        bodyController.executeBody(body);

    assert(bodyResponse.statusCode == 200);
    assert(bodyResponse.contentType == "application/json");
    assert(bodyResponse.body.find(""success":false") != std::string::npos);
    assert(bodyResponse.body.find(""type":"move"") != std::string::npos);
    assert(bodyResponse.body.find(""backendId":"living-room"") != std::string::npos);
    assert(bodyResponse.body.find(""recordingId":"recording-2"") != std::string::npos);
    assert(bodyResponse.body.find(""message":"dry-run backend execution skipped"") != std::string::npos);
    assert(bodyResponse.body.find(""dry-run only"") != std::string::npos);

    return 0;
}
