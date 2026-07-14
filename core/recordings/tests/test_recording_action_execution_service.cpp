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
        result.success =
            payload.type == RecordingActionType::Delete ||
            !payload.dryRun;
        result.type = payload.type;
        result.backendId = payload.backendId;
        result.recordingId = payload.recordingId;

        if (payload.dryRun && payload.type == RecordingActionType::Delete)
        {
            result.message = "native recording trash preview ready";
        }
        else if (payload.dryRun)
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

    assert(!dryRunExecutor.called);
    assert(!dryRunResult.success);
    assert(dryRunResult.type == RecordingActionType::Move);
    assert(dryRunResult.backendId == "living-room");
    assert(dryRunResult.recordingId == "recording-1");
    assert(dryRunResult.message == "dry-run backend execution skipped");
    assert(dryRunResult.hasWarnings());
    assert(!dryRunResult.hasErrors());

    assert(dryRunExecutor.capturedPayload.backendId.empty());
    assert(dryRunExecutor.capturedPayload.recordingId.empty());
    assert(dryRunExecutor.capturedPayload.type == RecordingActionType::Unknown);

    RecordingActionRequest deleteDryRunRequest;
    deleteDryRunRequest.backendId = "living-room";
    deleteDryRunRequest.recordingId = "recording-preview";
    deleteDryRunRequest.type = RecordingActionType::Delete;
    deleteDryRunRequest.dryRun = true;
    deleteDryRunRequest.parameters["recordingPath"] =
        "/video/recording-preview.rec";

    CapturingRecordingActionExecutor deleteDryRunExecutor;

    const RecordingActionExecutionResult deleteDryRunResult =
        service.execute(deleteDryRunRequest, deleteDryRunExecutor);

    assert(deleteDryRunExecutor.called);
    assert(!deleteDryRunResult.success);
    assert(deleteDryRunResult.type == RecordingActionType::Delete);
    assert(deleteDryRunResult.backendId == "living-room");
    assert(deleteDryRunResult.recordingId == "recording-preview");
    assert(deleteDryRunResult.message == "dry-run backend execution skipped");
    assert(deleteDryRunResult.hasWarnings());
    assert(!deleteDryRunResult.hasErrors());
    assert(deleteDryRunExecutor.capturedPayload.dryRun);
    assert(deleteDryRunExecutor.capturedPayload.type == RecordingActionType::Delete);
    assert(deleteDryRunExecutor.capturedPayload.parameters.at("recordingPath") ==
        "/video/recording-preview.rec");

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
    assert(executeExecutor.capturedPayload.requiredCapabilities.at(0) ==
        "recording.action.delete");
    assert(executeExecutor.capturedPayload.requiredPermissions.at(0) ==
        "recording.permission.delete");

    return 0;
}
