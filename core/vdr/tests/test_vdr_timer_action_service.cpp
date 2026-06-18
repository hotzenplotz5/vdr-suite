#include "MockVdrTimerActionExecutor.h"
#include "VdrTimerActionService.h"

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

static void test_create_delegates_to_executor()
{
    VdrTimerActionService service;
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        service.create(request, executor);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.message == "Timer created");
    assert(executor.callCount() == 1);
    assert(executor.lastActionType() == VdrTimerActionType::Create);
    assert(executor.lastRequest().title == "Tagesschau");
}

static void test_update_delegates_to_executor()
{
    VdrTimerActionService service;
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        service.update(request, executor);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Update);
    assert(result.message == "Timer updated");
    assert(executor.callCount() == 1);
    assert(executor.lastActionType() == VdrTimerActionType::Update);
}

static void test_remove_delegates_to_executor_as_delete_action()
{
    VdrTimerActionService service;
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        service.remove(request, executor);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.message == "Timer deleted");
    assert(executor.callCount() == 1);
    assert(executor.lastActionType() == VdrTimerActionType::Delete);
}

static void test_toggle_delegates_to_executor()
{
    VdrTimerActionService service;
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    const VdrTimerActionResult result =
        service.toggle(request, executor);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Toggle);
    assert(result.message == "Timer toggled");
    assert(executor.callCount() == 1);
    assert(executor.lastActionType() == VdrTimerActionType::Toggle);
}

static void test_executor_failure_is_returned_unchanged()
{
    VdrTimerActionService service;
    MockVdrTimerActionExecutor executor;
    const VdrTimerOperationRequest request = makeRequest();

    executor.failNext(
        "Timer action failed",
        {"backend is read-only"});

    const VdrTimerActionResult result =
        service.remove(request, executor);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.message == "Timer action failed");
    assert(result.hasErrors() == true);
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backend is read-only");
    assert(executor.callCount() == 1);
}

int main()
{
    test_create_delegates_to_executor();
    test_update_delegates_to_executor();
    test_remove_delegates_to_executor_as_delete_action();
    test_toggle_delegates_to_executor();
    test_executor_failure_is_returned_unchanged();

    return 0;
}
