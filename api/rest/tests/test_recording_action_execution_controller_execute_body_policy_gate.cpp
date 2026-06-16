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
    explicit CapturingBackendExecutorAdapter(std::string backendId)
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

        return RecordingActionExecutionResult::ok(
            payload.type,
            payload.recordingId,
            payload.backendId,
            "captured from body");
    }

    bool executed = false;

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
        const ApiResponse response =
            controller.executeBody(
                "{\"type\":\"DELETE\","
                "\"recordingId\":\"recording-1\","
                "\"backendId\":\"living-room\","
                "\"dryRun\":false}");

        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(response.body.find("\"success\":true") != std::string::npos);
        assert(response.body.find("captured from body") != std::string::npos);
        assert(livingRoomAdapter->executed);
    }

    {
        remoteHouseAdapter->executed = false;

        const ApiResponse response =
            controller.executeBody(
                "{\"type\":\"DELETE\","
                "\"recordingId\":\"recording-2\","
                "\"backendId\":\"remote-house\","
                "\"dryRun\":false}");

        assert(response.statusCode == 200);
        assert(response.body.find("\"success\":false") != std::string::npos);
        assert(response.body.find("recording action execution blocked by safety policy") != std::string::npos);
        assert(response.body.find("recording action execution is blocked by read-only backend config") != std::string::npos);
        assert(response.body.find("missing permission: recording.permission.delete") != std::string::npos);
        assert(!remoteHouseAdapter->executed);
    }

    return 0;
}
