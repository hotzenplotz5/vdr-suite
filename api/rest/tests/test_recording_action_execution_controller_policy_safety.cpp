#include "RecordingActionExecutionController.h"

#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationRequestParser.h"

#include <cassert>
#include <memory>
#include <string>

namespace
{
class NoOpBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit NoOpBackendExecutorAdapter(
        std::string backendId)
        : backendId_(std::move(backendId))
    {
    }

    std::string backendId() const override
    {
        return backendId_;
    }

    std::string backendType() const override
    {
        return "noop";
    }

    RecordingActionCapabilitySet capabilities() const override
    {
        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        return RecordingActionExecutionResult::ok(
            payload.type,
            payload.recordingId,
            payload.backendId,
            "noop");
    }

private:
    std::string backendId_;
};
}

int main()
{
    RecordingActionExecutionService executionService;
    RecordingActionExecutionResultJsonSerializer jsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry executorRegistry;
    RecordingActionValidationRequestParser requestParser;
    BackendRegistry backendRegistry;

    executorRegistry.registerAdapter(
        std::make_shared<NoOpBackendExecutorAdapter>("living-room"));
    executorRegistry.registerAdapter(
        std::make_shared<NoOpBackendExecutorAdapter>("remote-house"));

    BackendNode livingRoom;
    livingRoom.backendId = "living-room";
    livingRoom.backendType = "restfulapi";
    livingRoom.enabled = true;
    livingRoom.online = true;
    backendRegistry.addBackend(livingRoom);

    BackendNode remoteHouse;
    remoteHouse.backendId = "remote-house";
    remoteHouse.backendType = "restfulapi-readonly";
    remoteHouse.enabled = true;
    remoteHouse.online = true;
    backendRegistry.addBackend(remoteHouse);

    RecordingActionExecutionController controller(
        executionService,
        jsonSerializer,
        executorRegistry,
        backendRegistry,
        requestParser);

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-1";
        request.type = RecordingActionType::Move;
        request.dryRun = false;

        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = false;

        const ApiResponse response =
            controller.safety(request, context);

        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(response.body.find("\"canExecute\":true") != std::string::npos);
        assert(response.body.find("\"reasons\":[]") != std::string::npos);
    }

    {
        RecordingActionRequest request;
        request.backendId = "remote-house";
        request.recordingId = "recording-2";
        request.type = RecordingActionType::Move;
        request.dryRun = true;

        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const ApiResponse response =
            controller.safety(request, context);

        assert(response.statusCode == 200);
        assert(response.body.find("\"canExecute\":false") != std::string::npos);
        assert(response.body.find("\"readOnlyBlocked\":true") != std::string::npos);
        assert(response.body.find("\"reasons\":[\"backend_read_only\",\"permission_denied\"]") != std::string::npos);
        assert(response.body.find("missing permission: recording.permission.move") != std::string::npos);
    }

    {
        RecordingActionRequest request;
        request.backendId = "missing-room";
        request.recordingId = "recording-3";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const ApiResponse response =
            controller.safety(request, context);

        assert(response.statusCode == 200);
        assert(response.body.find("\"canExecute\":false") != std::string::npos);
        assert(response.body.find("\"backendUnavailable\":true") != std::string::npos);
        assert(response.body.find("\"readOnlyBlocked\":true") != std::string::npos);
    }

    return 0;
}
