#include "RecordingActionExecutionResult.h"

#include <cassert>
#include <string>
#include <vector>

int main() {
    auto success = RecordingActionExecutionResult::ok(
        "delete",
        "recording-1",
        "living-room",
        "Recording action executed"
    );

    assert(success.success);
    assert(success.action == "delete");
    assert(success.recordingId == "recording-1");
    assert(success.backendId == "living-room");
    assert(success.message == "Recording action executed");
    assert(!success.hasWarnings());
    assert(!success.hasErrors());

    auto failure = RecordingActionExecutionResult::failed(
        "delete",
        "recording-2",
        "remote-house",
        "Recording action rejected",
        {"Backend is read-only"}
    );

    assert(!failure.success);
    assert(failure.action == "delete");
    assert(failure.recordingId == "recording-2");
    assert(failure.backendId == "remote-house");
    assert(failure.message == "Recording action rejected");
    assert(!failure.hasWarnings());
    assert(failure.hasErrors());
    assert(failure.errors.size() == 1);
    assert(failure.errors[0] == "Backend is read-only");

    failure.warnings.push_back("Action requires backend dispatch");
    assert(failure.hasWarnings());

    return 0;
}
