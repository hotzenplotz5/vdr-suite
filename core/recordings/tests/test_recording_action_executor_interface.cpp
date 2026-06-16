#include "IRecordingActionExecutor.h"

#include <cassert>
#include <string>

namespace
{
class FakeRecordingActionExecutor : public IRecordingActionExecutor
{
public:
    RecordingActionExecutionResult execute(
        const RecordingActionRequest& request) override
    {
        RecordingActionExecutionResult result;

        result.success = !request.dryRun;
        result.type = request.type;
        result.backendId = request.backendId;
        result.recordingId = request.recordingId;

        if (request.dryRun)
        {
            result.message = "dry-run execution skipped";
            result.warnings.push_back("dry-run only");
        }
        else
        {
            result.message = "recording action executed";
        }

        return result;
    }
};
}

int main()
{
    FakeRecordingActionExecutor executor;

    RecordingActionRequest dryRunRequest;
    dryRunRequest.backendId = "living-room";
    dryRunRequest.recordingId = "recording-1";
    dryRunRequest.type = RecordingActionType::Move;
    dryRunRequest.dryRun = true;
    dryRunRequest.parameters["targetPath"] = "/video/archive";

    const RecordingActionExecutionResult dryRunResult =
        executor.execute(dryRunRequest);

    assert(!dryRunResult.success);
    assert(dryRunResult.type == RecordingActionType::Move);
    assert(dryRunResult.backendId == "living-room");
    assert(dryRunResult.recordingId == "recording-1");
    assert(dryRunResult.message == "dry-run execution skipped");
    assert(dryRunResult.hasWarnings());
    assert(!dryRunResult.hasErrors());

    RecordingActionRequest executeRequest;
    executeRequest.backendId = "living-room";
    executeRequest.recordingId = "recording-2";
    executeRequest.type = RecordingActionType::Delete;
    executeRequest.dryRun = false;

    const RecordingActionExecutionResult executeResult =
        executor.execute(executeRequest);

    assert(executeResult.success);
    assert(executeResult.type == RecordingActionType::Delete);
    assert(executeResult.backendId == "living-room");
    assert(executeResult.recordingId == "recording-2");
    assert(executeResult.message == "recording action executed");
    assert(!executeResult.hasWarnings());
    assert(!executeResult.hasErrors());

    return 0;
}
