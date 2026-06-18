#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "MockVdrTimerActionExecutor.h"

#include <cassert>
#include <memory>
#include <string>

class TestTimerActionExecutorAdapter final : public IVdrTimerActionExecutorAdapter
{
public:
    explicit TestTimerActionExecutorAdapter(
        std::string backendId)
        : backendId_(backendId)
    {
    }

    std::string backendId() const override
    {
        return backendId_;
    }

    IVdrTimerActionExecutor& executor() override
    {
        return executor_;
    }

private:
    std::string backendId_;
    MockVdrTimerActionExecutor executor_;
};

static void test_empty_registry_returns_not_found()
{
    VdrTimerActionExecutorAdapterRegistry registry;

    const VdrTimerActionExecutorAdapterLookupResult result =
        registry.findAdapter("living-room");

    assert(result.found == false);
    assert(result.backendId == "living-room");
    assert(result.adapter == nullptr);
    assert(result.message == "timer action executor adapter not found");
}

static void test_register_and_find_adapter()
{
    VdrTimerActionExecutorAdapterRegistry registry;

    auto adapter =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    registry.registerAdapter(adapter);

    const VdrTimerActionExecutorAdapterLookupResult result =
        registry.findAdapter("living-room");

    assert(result.found == true);
    assert(result.backendId == "living-room");
    assert(result.adapter == adapter);
    assert(result.message == "timer action executor adapter found");
}

static void test_null_adapter_is_ignored()
{
    VdrTimerActionExecutorAdapterRegistry registry;

    registry.registerAdapter(nullptr);

    const VdrTimerActionExecutorAdapterLookupResult result =
        registry.findAdapter("living-room");

    assert(result.found == false);
}

static void test_registering_same_backend_replaces_adapter()
{
    VdrTimerActionExecutorAdapterRegistry registry;

    auto first =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    auto second =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    registry.registerAdapter(first);
    registry.registerAdapter(second);

    const VdrTimerActionExecutorAdapterLookupResult result =
        registry.findAdapter("living-room");

    assert(result.found == true);
    assert(result.adapter == second);
}

static void test_adapter_exposes_executor()
{
    VdrTimerActionExecutorAdapterRegistry registry;

    auto adapter =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    registry.registerAdapter(adapter);

    const VdrTimerActionExecutorAdapterLookupResult result =
        registry.findAdapter("living-room");

    VdrTimerOperationRequest request;
    request.backendId = "living-room";
    request.timerId = "42";

    const VdrTimerActionResult actionResult =
        result.adapter->executor().execute(
            VdrTimerActionType::Create,
            request);

    assert(actionResult.success == true);
    assert(actionResult.backendId == "living-room");
    assert(actionResult.timerId == "42");
}

int main()
{
    test_empty_registry_returns_not_found();
    test_register_and_find_adapter();
    test_null_adapter_is_ignored();
    test_registering_same_backend_replaces_adapter();
    test_adapter_exposes_executor();

    return 0;
}
