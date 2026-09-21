#include "IVdrTimerActionExecutor.h"

#include <cassert>
#include <string>

class StubTimerActionExecutor : public IVdrTimerActionExecutor
{
public:
    VdrTimerActionType lastType = VdrTimerActionType::Unknown;
    VdrTimerOperationRequest lastRequest;
    int calls = 0;

    VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request) override
    {
        lastType = type;
        lastRequest = request;
        calls += 1;

        return VdrTimerActionResult::ok(
            type,
            request.timerId,
            request.backendId,
            "stub timer action executed");
    }
};

static void test_executor_interface_dispatches_request()
{
    StubTimerActionExecutor executor;

    VdrTimerOperationRequest request;
    request.timerId = "timer-42";
    request.backendId = "living-room";
    request.channelId = "C-61441-10006-50021";
    request.title = "Tagesschau";
    request.day = "2026-06-18";
    request.start = 2015;
    request.stop = 2030;

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Update, request);

    assert(executor.calls == 1);
    assert(executor.lastType == VdrTimerActionType::Update);
    assert(executor.lastRequest.timerId == "timer-42");
    assert(executor.lastRequest.backendId == "living-room");
    assert(executor.lastRequest.channelId == "C-61441-10006-50021");
    assert(executor.lastRequest.title == "Tagesschau");
    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Update);
    assert(result.timerId == "timer-42");
    assert(result.backendId == "living-room");
    assert(result.message == "stub timer action executed");
}

static void test_executor_interface_supports_failed_result()
{
    class FailingTimerActionExecutor : public IVdrTimerActionExecutor
    {
    public:
        VdrTimerActionResult execute(
            VdrTimerActionType type,
            const VdrTimerOperationRequest& request) override
        {
            return VdrTimerActionResult::failed(
                type,
                request.timerId,
                request.backendId,
                "stub timer action failed",
                {"timer action rejected"});
        }
    };

    FailingTimerActionExecutor executor;

    VdrTimerOperationRequest request;
    request.timerId = "timer-42";
    request.backendId = "living-room";

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Delete, request);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.hasErrors() == true);
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "timer action rejected");
}

int main()
{
    test_executor_interface_dispatches_request();
    test_executor_interface_supports_failed_result();

    return 0;
}
