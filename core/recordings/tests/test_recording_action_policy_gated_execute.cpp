#include "RecordingActionExecutionService.h"

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
    RecordingActionExecutionService service;
    RecordingActionBackendExecutorAdapterRegistry registry;
    RecordingActionBackendPolicyFactory policyFactory;

    auto adapter =
        std::make_shared<CapturingBackendExecutorAdapter>("living-room");
    registry.registerAdapter(adapter);

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-1";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        const RecordingActionBackendPolicy policy =
            policyFactory.restfulApiMutationPolicy("living-room");

        const RecordingActionExecutionResult result =
            service.execute(request, registry, policy);

        assert(result.success);
        assert(result.message == "captured");
        assert(adapter->executed);
        assert(adapter->lastRecordingId == "recording-1");
        assert(adapter->lastBackendId == "living-room");
        assert(adapter->lastType == RecordingActionType::Delete);
    }

    {
        adapter->executed = false;

        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-2";
        request.type = RecordingActionType::Delete;
        request.dryRun = true;

        const RecordingActionBackendPolicy policy =
            policyFactory.readOnlyPolicy("living-room");

        const RecordingActionExecutionResult result =
            service.execute(request, registry, policy);

        assert(!result.success);
        assert(result.message ==
               "recording action execution blocked by safety policy");
        assert(!adapter->executed);
        assert(result.errors.size() == 3);
        assert(result.errors.at(0) ==
               "recording action execution is blocked by read-only backend config");
        assert(result.errors.at(1) ==
               "recording action permission is denied");
        assert(result.errors.at(2) ==
               "missing permission: recording.permission.delete");
    }

    {
        adapter->executed = false;

        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-3";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        RecordingActionBackendPolicy policy =
            policyFactory.restfulApiMutationPolicy("living-room");
        policy.backendAvailable = false;

        const RecordingActionExecutionResult result =
            service.execute(request, registry, policy);

        assert(!result.success);
        assert(!adapter->executed);
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) ==
               "recording action backend is unavailable");
    }

    return 0;
}
