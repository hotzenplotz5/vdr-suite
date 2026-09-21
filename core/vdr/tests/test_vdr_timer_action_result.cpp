#include "VdrTimerActionResult.h"

#include <cassert>
#include <string>
#include <vector>

static void test_default_result_is_unknown_failure()
{
    VdrTimerActionResult result;

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Unknown);
    assert(result.timerId.empty() == true);
    assert(result.backendId.empty() == true);
    assert(result.message.empty() == true);
    assert(result.warnings.empty() == true);
    assert(result.errors.empty() == true);
    assert(result.hasWarnings() == false);
    assert(result.hasErrors() == false);
}

static void test_success_result_carries_action_context()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::ok(
            VdrTimerActionType::Create,
            "timer-42",
            "living-room",
            "timer action executed");

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.timerId == "timer-42");
    assert(result.backendId == "living-room");
    assert(result.message == "timer action executed");
    assert(result.hasWarnings() == false);
    assert(result.hasErrors() == false);
}

static void test_failed_result_carries_errors()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::failed(
            VdrTimerActionType::Delete,
            "timer-42",
            "living-room",
            "timer action failed",
            {"timer is currently recording"});

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.timerId == "timer-42");
    assert(result.backendId == "living-room");
    assert(result.message == "timer action failed");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "timer is currently recording");
    assert(result.hasErrors() == true);
    assert(result.hasWarnings() == false);
}

static void test_warnings_are_supported()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::ok(
            VdrTimerActionType::Update,
            "timer-42",
            "living-room",
            "timer action executed");

    result.warnings.push_back("timer uses VPS");

    assert(result.success == true);
    assert(result.hasWarnings() == true);
    assert(result.warnings.size() == 1);
    assert(result.warnings.at(0) == "timer uses VPS");
}

static void test_toggle_action_type_exists()
{
    VdrTimerActionResult result =
        VdrTimerActionResult::ok(
            VdrTimerActionType::Toggle,
            "timer-42",
            "living-room",
            "timer action executed");

    assert(result.type == VdrTimerActionType::Toggle);
}

int main()
{
    test_default_result_is_unknown_failure();
    test_success_result_carries_action_context();
    test_failed_result_carries_errors();
    test_warnings_are_supported();
    test_toggle_action_type_exists();

    return 0;
}
