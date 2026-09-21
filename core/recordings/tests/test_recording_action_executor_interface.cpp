#include "IRecordingActionExecutor.h"

#include <cassert>
#include <string>

namespace
{
class FakeRecordingActionExecutor : public IRecordingActionExecutor
{
public:
    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;

        result.success = !payload.dryRun;
        result.type = payload.type;
        result.backendId = payload.backendId;
        result.recordingId = payload.recordingId;

        if (payload.dryRun)
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

    RecordingActionJobPayload dryRunPayload;
    dryRunPayload.backendId = "living-room";
    dryRunPayload.recordingId = "recording-1";
    dryRunPayload.type = RecordingActionType::Move;
    dryRunPayload.dryRun = true;
    dryRunPayload.parameters["targetPath"] = "/video/archive";

    const RecordingActionExecutionResult dryRunResult =
        executor.execute(dryRunPayload);

    assert(!dryRunResult.success);
    assert(dryRunResult.type == RecordingActionType::Move);
    assert(dryRunResult.backendId == "living-room");
    assert(dryRunResult.recordingId == "recording-1");
    assert(dryRunResult.message == "dry-run execution skipped");
    assert(dryRunResult.hasWarnings());
    assert(!dryRunResult.hasErrors());

    RecordingActionJobPayload executePayload;
    executePayload.backendId = "living-room";
    executePayload.recordingId = "recording-2";
    executePayload.type = RecordingActionType::Delete;
    executePayload.dryRun = false;

    const RecordingActionExecutionResult executeResult =
        executor.execute(executePayload);

    assert(executeResult.success);
    assert(executeResult.type == RecordingActionType::Delete);
    assert(executeResult.backendId == "living-room");
    assert(executeResult.recordingId == "recording-2");
    assert(executeResult.message == "recording action executed");
    assert(!executeResult.hasWarnings());
    assert(!executeResult.hasErrors());

    return 0;
}
