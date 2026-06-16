#include "RecordingActionExecutionService.h"

#include <cassert>
#include <string>

namespace
{
class CapturingRecordingActionExecutor final : public IRecordingActionExecutor
{
public:
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
            result.message = "dry-run execution skipped";
        }
        else
        {
            result.message = "recording action executed";
        }

        return result;
    }

    bool called = false;
    RecordingActionJobPayload capturedPayload;
};
}

int main()
{
    RecordingActionExecutionService service;

    RecordingActionRequest invalidRequest;
    invalidRequest.type = RecordingActionType::Move;
    invalidRequest.dryRun = false;

    CapturingRecordingActionExecutor invalidExecutor;

    const RecordingActionExecutionResult invalidResult =
        service.execute(invalidRequest, invalidExecutor);

    assert(!invalidExecutor.called);
    assert(!invalidResult.success);
    assert(invalidResult.type == RecordingActionType::Move);
    assert(invalidResult.message == "recording action validation failed");
    assert(invalidResult.hasErrors());

    RecordingActionRequest dryRunRequest;
    dryRunRequest.backendId = "living-room";
    dryRunRequest.recordingId = "recording-1";
    dryRunRequest.type = RecordingActionType::Move;
    dryRunRequest.dryRun = true;
    dryRunRequest.parameters["targetPath"] = "/video/archive";

    CapturingRecordingActionExecutor dryRunExecutor;

    const RecordingActionExecutionResult dryRunResult =
        service.execute(dryRunRequest, dryRunExecutor);

    assert(dryRunExecutor.called);
    assert(!dryRunResult.success);
    assert(dryRunResult.type == RecordingActionType::Move);
    assert(dryRunResult.backendId == "living-room");
    assert(dryRunResult.recordingId == "recording-1");
    assert(dryRunResult.message == "dry-run execution skipped");
    assert(dryRunResult.hasWarnings());
    assert(!dryRunResult.hasErrors());

    assert(dryRunExecutor.capturedPayload.backendId == "living-room");
    assert(dryRunExecutor.capturedPayload.recordingId == "recording-1");
    assert(dryRunExecutor.capturedPayload.type == RecordingActionType::Move);
    assert(dryRunExecutor.capturedPayload.jobType == "recording.move");
    assert(dryRunExecutor.capturedPayload.dryRun);
    assert(dryRunExecutor.capturedPayload.parameters.at("targetPath") == "/video/archive");
    assert(dryRunExecutor.capturedPayload.requiredCapabilities.at(0) == "recordings.action.move");
    assert(dryRunExecutor.capturedPayload.requiredPermissions.at(0) == "recordings.action.move");

    RecordingActionRequest executeRequest;
    executeRequest.backendId = "living-room";
    executeRequest.recordingId = "recording-2";
    executeRequest.type = RecordingActionType::Delete;
    executeRequest.dryRun = false;

    CapturingRecordingActionExecutor executeExecutor;

    const RecordingActionExecutionResult executeResult =
        service.execute(executeRequest, executeExecutor);

    assert(executeExecutor.called);
    assert(executeResult.success);
    assert(executeResult.type == RecordingActionType::Delete);
    assert(executeResult.backendId == "living-room");
    assert(executeResult.recordingId == "recording-2");
    assert(executeResult.message == "recording action executed");
    assert(!executeResult.hasWarnings());
    assert(!executeResult.hasErrors());

    assert(executeExecutor.capturedPayload.backendId == "living-room");
    assert(executeExecutor.capturedPayload.recordingId == "recording-2");
    assert(executeExecutor.capturedPayload.type == RecordingActionType::Delete);
    assert(executeExecutor.capturedPayload.jobType == "recording.delete");
    assert(!executeExecutor.capturedPayload.dryRun);
    assert(executeExecutor.capturedPayload.requiredCapabilities.at(0) == "recordings.action.delete");
    assert(executeExecutor.capturedPayload.requiredPermissions.at(0) == "recordings.action.delete");

    return 0;
}
