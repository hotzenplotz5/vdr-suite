#include "VdrTimerActionExecutionService.h"

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

static VdrTimerOperationRequest makeRequest()
{
    VdrTimerOperationRequest request;
    request.backendId = "living-room";
    request.timerId = "42";
    return request;
}

static void test_execute_direct_executor()
{
    VdrTimerActionExecutionService service;
    MockVdrTimerActionExecutor executor;

    const VdrTimerActionResult result =
        service.execute(
            VdrTimerActionType::Create,
            makeRequest(),
            executor);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.backendId == "living-room");
    assert(result.timerId == "42");
    assert(executor.callCount() == 1);
}

static void test_execute_via_registry()
{
    VdrTimerActionExecutionService service;
    VdrTimerActionExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room"));

    const VdrTimerActionResult result =
        service.execute(
            VdrTimerActionType::Update,
            makeRequest(),
            registry);

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Update);
    assert(result.backendId == "living-room");
    assert(result.timerId == "42");
}

static void test_missing_registry_adapter_returns_failure()
{
    VdrTimerActionExecutionService service;
    VdrTimerActionExecutorAdapterRegistry registry;

    const VdrTimerActionResult result =
        service.execute(
            VdrTimerActionType::Delete,
            makeRequest(),
            registry);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Delete);
    assert(result.backendId == "living-room");
    assert(result.timerId == "42");
    assert(result.message == "timer action executor adapter not found");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "timer action executor adapter not found");
}

int main()
{
    test_execute_direct_executor();
    test_execute_via_registry();
    test_missing_registry_adapter_returns_failure();

    return 0;
}
