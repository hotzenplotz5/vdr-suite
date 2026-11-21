#include "RecordingActionExecutionController.h"

#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationRequestParser.h"

#include <cassert>
#include <memory>
#include <string>

namespace
{
class CapturingBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit CapturingBackendExecutorAdapter(
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
        return "capturing";
    }

    RecordingActionCapabilitySet capabilities() const override
    {
        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        executed = true;
        lastRecordingId = payload.recordingId;
        lastBackendId = payload.backendId;
        lastType = payload.type;

        return RecordingActionExecutionResult::ok(
            payload.type,
            payload.recordingId,
            payload.backendId,
            "captured");
    }

    bool executed = false;
    std::string lastRecordingId;
    std::string lastBackendId;
    RecordingActionType lastType = RecordingActionType::Unknown;

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

    auto livingRoomAdapter =
        std::make_shared<CapturingBackendExecutorAdapter>("living-room");
    auto remoteHouseAdapter =
        std::make_shared<CapturingBackendExecutorAdapter>("remote-house");

    executorRegistry.registerAdapter(livingRoomAdapter);
    executorRegistry.registerAdapter(remoteHouseAdapter);

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
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        const ApiResponse response =
            controller.execute(request);

        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(response.body.find("\"success\":true") != std::string::npos);
        assert(response.body.find("\"message\":\"captured\"") != std::string::npos);
        assert(livingRoomAdapter->executed);
        assert(livingRoomAdapter->lastType == RecordingActionType::Delete);
    }

    {
        remoteHouseAdapter->executed = false;

        RecordingActionRequest request;
        request.backendId = "remote-house";
        request.recordingId = "recording-2";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        const ApiResponse response =
            controller.execute(request);

        assert(response.statusCode == 200);
        assert(response.body.find("\"success\":false") != std::string::npos);
        assert(response.body.find("recording action execution blocked by safety policy") != std::string::npos);
        assert(response.body.find("recording action execution is blocked by read-only backend config") != std::string::npos);
        assert(response.body.find("missing permission: recording.permission.delete") != std::string::npos);
        assert(!remoteHouseAdapter->executed);
    }

    {
        RecordingActionRequest request;
        request.backendId = "missing-room";
        request.recordingId = "recording-3";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        const ApiResponse response =
            controller.execute(request);

        assert(response.statusCode == 200);
        assert(response.body.find("\"success\":false") != std::string::npos);
        assert(response.body.find("recording action backend is unavailable") != std::string::npos);
    }

    return 0;
}
