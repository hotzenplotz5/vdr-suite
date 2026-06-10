#include "BackendRuntimeContext.h"
#include "IVdrAdapter.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

class TestVdrAdapter : public IVdrAdapter
{
public:
    VdrStatus getStatus() const override
    {
        VdrStatus status;
        status.enabled = true;
        status.mode = "runtime-context-test";
        status.state = "connected";
        return status;
    }

    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }

    std::vector<VdrChannel> getChannels() const override
    {
        return {};
    }

    std::vector<VdrTimer> getTimers() const override
    {
        return {};
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        return {};
    }

    VdrChangeState getChangeState() const override
    {
        return {};
    }
};

static void test_backend_runtime_context_can_own_backend_runtime_chain()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);

    BackendRuntimeContext context;
    context.backendId = "home-vdr";
    context.adapter = std::make_unique<TestVdrAdapter>();
    context.service = std::make_unique<VdrService>(*context.adapter);
    context.snapshotBuilder = std::make_unique<VdrSnapshotBuilder>(
        *context.service,
        context.backendId,
        nullptr,
        nullptr);
    context.pollingService = std::make_unique<PollingService>(
        *context.snapshotBuilder,
        *context.service,
        cacheService,
        context.backendId);

    context.pollingService->poll();

    assert(context.backendId == "home-vdr");
    assert(context.adapter != nullptr);
    assert(context.service != nullptr);
    assert(context.snapshotBuilder != nullptr);
    assert(context.pollingService != nullptr);

    assert(cache.hasSnapshotForBackend("home-vdr"));

    const VdrSnapshot* snapshot =
        cache.snapshotForBackend("home-vdr");

    assert(snapshot != nullptr);
    assert(snapshot->backendId == "home-vdr");
    assert(snapshot->status.mode == "runtime-context-test");
}

int main()
{
    test_backend_runtime_context_can_own_backend_runtime_chain();

    std::cout << "test_backend_runtime_context passed" << std::endl;
    return 0;
}
