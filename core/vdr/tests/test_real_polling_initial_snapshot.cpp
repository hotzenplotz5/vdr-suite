#include "BackendPollingCoordinator.h"
#include "BasicHttpClient.h"
#include "PollingService.h"
#include "RestfulApiVdrAdapter.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrConfig.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

static std::string envOrDefault(
    const char* name,
    const std::string& fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || std::string(value).empty())
    {
        return fallback;
    }

    return value;
}

static int envPortOrDefault(
    const char* name,
    int fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || std::string(value).empty())
    {
        return fallback;
    }

    return std::stoi(value);
}

int main()
{
    const char* enabled = std::getenv("VDR_SUITE_REAL_VDR_TEST");

    if (enabled == nullptr || std::string(enabled) != "1")
    {
        std::cout
            << "test_real_polling_initial_snapshot skipped "
            << "(set VDR_SUITE_REAL_VDR_TEST=1 to run)"
            << std::endl;
        return 0;
    }

    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = envOrDefault("VDR_SUITE_VDR_HOST", "127.0.0.1");
    config.port = envPortOrDefault("VDR_SUITE_VDR_PORT", 8002);

    const std::string backendId =
        envOrDefault("VDR_SUITE_REAL_VDR_BACKEND_ID", "default");

    BasicHttpClient httpClient(config.host, config.port);
    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrService service(adapter);

    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    VdrSnapshotBuilder snapshotBuilder(service, backendId, nullptr, nullptr);
    PollingService pollingService(
        snapshotBuilder,
        service,
        cacheService,
        backendId,
        nullptr,
        nullptr);

    BackendPollingCoordinator coordinator;
    coordinator.addPollingService(backendId, pollingService);

    assert(coordinator.backendCount() == 1);
    assert(cacheService.cache().hasSnapshotForBackend(backendId) == false);

    coordinator.pollAll();

    assert(cacheService.cache().hasSnapshot() == true);
    assert(cacheService.cache().hasSnapshotForBackend(backendId) == true);

    const VdrSnapshot* snapshot =
        cacheService.cache().snapshotForBackend(backendId);

    assert(snapshot != nullptr);
    assert(snapshot->backendId == backendId);
    assert(snapshot->status.enabled == true);
    assert(snapshot->status.mode == "restfulapi");
    assert(snapshot->status.host == config.host);
    assert(snapshot->status.port == config.port);
    assert(snapshot->status.state == "restfulapi");

    assert(snapshot->channels.empty() == false);
    assert(snapshot->events.empty() == false);
    assert(snapshot->recordings.empty() == false);

    assert(pollingService.changeEvents().empty() == true);
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);

    std::cout
        << "real polling initial snapshot validation ok: "
        << "backend=" << snapshot->backendId
        << " host=" << config.host
        << " port=" << config.port
        << " channels=" << snapshot->channels.size()
        << " events=" << snapshot->events.size()
        << " timers=" << snapshot->timers.size()
        << " recordings=" << snapshot->recordings.size()
        << std::endl;

    return 0;
}
