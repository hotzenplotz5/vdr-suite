#include "VdrTimerActionExecutionService.h"

#include "BackendAccessPolicy.h"
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

    int callCount() const
    {
        return executor_.callCount();
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

static BackendAccessDecision allowedAccessDecision()
{
    BackendAccessDecision decision;
    decision.allowed = true;
    decision.backendFound = true;
    decision.backendId = "living-room";
    decision.accessMode = "read-write";
    decision.reason = "backend write access allowed";
    return decision;
}

static BackendAccessDecision deniedAccessDecision()
{
    BackendAccessDecision decision;
    decision.allowed = false;
    decision.backendFound = true;
    decision.readOnly = true;
    decision.backendId = "living-room";
    decision.accessMode = "read-only";
    decision.reason = "backend is read-only";
    decision.errors.push_back("backend is read-only");
    return decision;
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
            VdrTimerActionType::Update,
            makeRequest(),
            registry);

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Update);
    assert(result.backendId == "living-room");
    assert(result.timerId == "42");
    assert(result.message == "timer action executor adapter not found");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "timer action executor adapter not found");
}

static void test_gated_execute_allows_write_decision()
{
    VdrTimerActionExecutionService service;
    VdrTimerActionExecutorAdapterRegistry executorRegistry;

    auto adapter =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    executorRegistry.registerAdapter(adapter);

    const VdrTimerActionResult result =
        service.execute(
            VdrTimerActionType::Create,
            makeRequest(),
            executorRegistry,
            allowedAccessDecision());

    assert(result.success == true);
    assert(result.type == VdrTimerActionType::Create);
    assert(result.backendId == "living-room");
    assert(adapter->callCount() == 1);
}

static void test_gated_execute_denies_read_only_decision()
{
    VdrTimerActionExecutionService service;
    VdrTimerActionExecutorAdapterRegistry executorRegistry;

    auto adapter =
        std::make_shared<TestTimerActionExecutorAdapter>(
            "living-room");

    executorRegistry.registerAdapter(adapter);

    const VdrTimerActionResult result =
        service.execute(
            VdrTimerActionType::Update,
            makeRequest(),
            executorRegistry,
            deniedAccessDecision());

    assert(result.success == false);
    assert(result.type == VdrTimerActionType::Update);
    assert(result.backendId == "living-room");
    assert(result.timerId == "42");
    assert(result.message == "backend is read-only");
    assert(result.errors.size() == 1);
    assert(result.errors.at(0) == "backend is read-only");
    assert(adapter->callCount() == 0);
}

int main()
{
    test_execute_direct_executor();
    test_execute_via_registry();
    test_missing_registry_adapter_returns_failure();
    test_gated_execute_allows_write_decision();
    test_gated_execute_denies_read_only_decision();

    return 0;
}
