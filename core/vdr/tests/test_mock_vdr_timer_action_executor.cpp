#include "MockVdrTimerActionExecutor.h"

#include <cassert>

static VdrTimerOperationRequest makeRequest()
{
    VdrTimerOperationRequest request;
    request.timerId = "timer-42";
    request.backendId = "living-room";
    request.channelId = "C-61441-10006-50021";
    request.title = "Tagesschau";
    request.day = "2026-06-18";
    request.start = 2015;
    request.stop = 2030;
    return request;
}

static void test_create_success()
{
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Create, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.timerId == "timer-42");
    assert(result.backendId == "living-room");
    assert(result.message == "Timer created");
    assert(result.hasErrors() == false);

    assert(executor.callCount() == 1);
    assert(executor.lastActionType() == VdrTimerActionType::Create);
    assert(executor.lastRequest().title == "Tagesschau");
}

static void test_update_success()
{
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Update, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Update);
    assert(result.message == "Timer updated");
}

static void test_delete_success()
{
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Delete, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.message == "Timer deleted");
}

static void test_toggle_success()
{
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Toggle, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Toggle);
    assert(result.message == "Timer toggled");
}

static void test_unknown_success_message_is_generic()
{
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        executor.execute(VdrTimerActionType::Unknown, request);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Unknown);
    assert(result.message == "Timer action executed");
}

static void test_fail_next_is_single_use()
{
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    executor.failNext(
        "Timer action failed",
        {"backend is read-only"});

    const VdrTimerActionResult failed =
        executor.execute(VdrTimerActionType::Delete, request);

    assert(failed.success == false);
    assert(failed.type == VdrTimerActionType::Delete);
    assert(failed.timerId == "timer-42");
    assert(failed.backendId == "living-room");
    assert(failed.message == "Timer action failed");
    assert(failed.hasErrors() == true);
    assert(failed.errors.size() == 1);
    assert(failed.errors.at(0) == "backend is read-only");

    const VdrTimerActionResult next =
        executor.execute(VdrTimerActionType::Delete, request);

    assert(next.success == true);
    assert(next.message == "Timer deleted");
    assert(executor.callCount() == 2);
}

int main()
{
    test_create_success();
    test_update_success();
    test_delete_success();
    test_toggle_success();
    test_unknown_success_message_is_generic();
    test_fail_next_is_single_use();

    return 0;
}
