#include "IRecordingActionBackendExecutorAdapter.h"
#include "RecordingActionBackendPolicy.h"
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
        ++executeCount;
        capturedType = payload.type;
        capturedBackendId = payload.backendId;
        capturedRecordingId = payload.recordingId;
        capturedRecordingPath =
            payload.parameters.at("recordingPath");
        capturedTargetPath =
            payload.parameters.at("targetPath");

        return RecordingActionExecutionResult::ok(
            payload.type,
            payload.recordingId,
            payload.backendId,
            "action dispatched to fake restfulapi adapter");
    }

    int executeCount = 0;
    RecordingActionType capturedType = RecordingActionType::Unknown;
    std::string capturedBackendId;
    std::string capturedRecordingId;
    std::string capturedRecordingPath;
    std::string capturedTargetPath;
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

RecordingActionBackendPolicy makeAllowedPolicy()
{
    RecordingActionBackendPolicyFactory factory;
    return factory.restfulApiMutationPolicy("local-vdr");
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
            makeAllowedPolicy());

    assert(result.success);
    assert(result.type == RecordingActionType::Move);
    assert(result.backendId == "local-vdr");
    assert(result.recordingId == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(result.message == "action dispatched to fake restfulapi adapter");

    assert(adapter->executeCount == 1);
    assert(adapter->capturedType == RecordingActionType::Move);
    assert(adapter->capturedBackendId == "local-vdr");
    assert(adapter->capturedRecordingId == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(adapter->capturedRecordingPath == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(adapter->capturedTargetPath == "Archiv/Tagesschau");

    return 0;
}
