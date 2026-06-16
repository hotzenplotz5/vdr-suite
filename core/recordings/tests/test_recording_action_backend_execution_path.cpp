#include "RecordingActionExecutionService.h"

#include <cassert>
#include <string>

namespace
{
class CapturingBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit CapturingBackendExecutorAdapter(
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
        called = true;
        capturedPayload = payload;

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

    bool called = false;
    RecordingActionJobPayload capturedPayload;

private:
    std::string backendIdValue_;
};
}

int main()
{
    RecordingActionExecutionService service;

    RecordingActionBackendExecutorAdapterRegistry emptyRegistry;

    RecordingActionRequest missingBackendRequest;
    missingBackendRequest.backendId = "missing-backend";
    missingBackendRequest.recordingId = "recording-1";
    missingBackendRequest.type = RecordingActionType::Delete;
    missingBackendRequest.dryRun = false;

    const RecordingActionExecutionResult missingBackendResult =
        service.execute(missingBackendRequest, emptyRegistry);

    assert(!missingBackendResult.success);
    assert(missingBackendResult.type == RecordingActionType::Delete);
    assert(missingBackendResult.backendId == "missing-backend");
    assert(missingBackendResult.recordingId == "recording-1");
    assert(missingBackendResult.message == "backend executor adapter not found");
    assert(missingBackendResult.hasErrors());

    RecordingActionBackendExecutorAdapterRegistry registry;

    auto adapter =
        std::make_shared<CapturingBackendExecutorAdapter>("living-room");

    registry.registerAdapter(adapter);

    RecordingActionRequest dryRunRequest;
    dryRunRequest.backendId = "living-room";
    dryRunRequest.recordingId = "recording-2";
    dryRunRequest.type = RecordingActionType::Move;
    dryRunRequest.dryRun = true;
    dryRunRequest.parameters["targetPath"] = "/video/archive";

    const RecordingActionExecutionResult dryRunResult =
        service.execute(dryRunRequest, registry);

    assert(adapter->called);
    assert(!dryRunResult.success);
    assert(dryRunResult.type == RecordingActionType::Move);
    assert(dryRunResult.backendId == "living-room");
    assert(dryRunResult.recordingId == "recording-2");
    assert(dryRunResult.message == "dry-run backend execution skipped");
    assert(dryRunResult.hasWarnings());
    assert(!dryRunResult.hasErrors());

    assert(adapter->capturedPayload.backendId == "living-room");
    assert(adapter->capturedPayload.recordingId == "recording-2");
    assert(adapter->capturedPayload.type == RecordingActionType::Move);
    assert(adapter->capturedPayload.jobType == "recording.move");
    assert(adapter->capturedPayload.dryRun);
    assert(adapter->capturedPayload.parameters.at("targetPath") == "/video/archive");

    RecordingActionBackendExecutorAdapterRegistry executionRegistry;

    auto executionAdapter =
        std::make_shared<CapturingBackendExecutorAdapter>("living-room");

    executionRegistry.registerAdapter(executionAdapter);

    RecordingActionRequest executeRequest;
    executeRequest.backendId = "living-room";
    executeRequest.recordingId = "recording-3";
    executeRequest.type = RecordingActionType::Delete;
    executeRequest.dryRun = false;

    const RecordingActionExecutionResult executeResult =
        service.execute(executeRequest, executionRegistry);

    assert(executionAdapter->called);
    assert(executeResult.success);
    assert(executeResult.type == RecordingActionType::Delete);
    assert(executeResult.backendId == "living-room");
    assert(executeResult.recordingId == "recording-3");
    assert(executeResult.message == "backend execution completed");
    assert(!executeResult.hasWarnings());
    assert(!executeResult.hasErrors());

    assert(executionAdapter->capturedPayload.backendId == "living-room");
    assert(executionAdapter->capturedPayload.recordingId == "recording-3");
    assert(executionAdapter->capturedPayload.type == RecordingActionType::Delete);
    assert(executionAdapter->capturedPayload.jobType == "recording.delete");
    assert(!executionAdapter->capturedPayload.dryRun);

    return 0;
}
