#include "IRecordingActionBackendExecutorAdapter.h"
#include "RecordingActionExecutionService.h"

#include <cassert>
#include <memory>
#include <string>

namespace
{
class CapturingRestfulApiExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    std::string backendId() const override
    {
        return "local-vdr";
    }

    std::string backendType() const override
    {
        return "restfulapi";
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
        capturedType = payload.type;
        capturedBackendId = payload.backendId;
        capturedRecordingId = payload.recordingId;

        return RecordingActionExecutionResult::ok(
            payload.type,
            payload.recordingId,
            payload.backendId,
            "unexpected dispatch");
    }

    bool executed = false;
    RecordingActionType capturedType = RecordingActionType::Unknown;
    std::string capturedBackendId;
    std::string capturedRecordingId;
};

RecordingActionRequest makeMoveRequest()
{
    RecordingActionRequest request;
    request.backendId = "local-vdr";
    request.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.type = RecordingActionType::Move;
    request.dryRun = false;
    request.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.parameters["targetPath"] =
        "Archiv/Tagesschau";
    return request;
}

RecordingActionBackendPolicy makeBlockedPolicy()
{
    RecordingActionCapabilityContract contract;

    RecordingActionBackendPolicy policy;
    policy.backendId = "local-vdr";
    policy.backendType = "restfulapi";
    policy.backendAvailable = true;
    policy.readOnly = true;
    policy.executionAllowed = false;
    policy.capabilities = contract.restfulApiDefaultCapabilities();
    return policy;
}
}

int main()
{
    RecordingActionExecutionService service;
    RecordingActionBackendExecutorAdapterRegistry registry;

    auto adapter =
        std::make_shared<CapturingRestfulApiExecutorAdapter>();

    registry.registerAdapter(adapter);

    const RecordingActionExecutionResult result =
        service.execute(
            makeMoveRequest(),
            registry,
            makeBlockedPolicy());

    assert(!result.success);
    assert(result.type == RecordingActionType::Move);
    assert(result.backendId == "local-vdr");
    assert(result.recordingId == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(result.message == "recording action execution blocked by safety policy");

    assert(!result.errors.empty());
    assert(result.errors.at(0).find("read-only") != std::string::npos ||
           result.errors.at(0).find("permission") != std::string::npos ||
           result.errors.at(0).find("execution") != std::string::npos);

    assert(!adapter->executed);
    assert(adapter->capturedType == RecordingActionType::Unknown);
    assert(adapter->capturedBackendId.empty());
    assert(adapter->capturedRecordingId.empty());

    return 0;
}
