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
            << "test_real_polling_stability skipped "
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

    pollingService.poll();

    assert(cache.hasSnapshot() == true);
    assert(cache.hasSnapshotForBackend(backendId) == true);
    assert(pollingService.changeEvents().empty() == true);
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);

    const VdrSnapshot* firstSnapshot =
        cache.snapshotForBackend(backendId);

    assert(firstSnapshot != nullptr);
    assert(firstSnapshot->backendId == backendId);
    assert(firstSnapshot->channels.empty() == false);
    assert(firstSnapshot->events.empty() == false);
    assert(firstSnapshot->recordings.empty() == false);

    const std::size_t firstChannelCount = firstSnapshot->channels.size();
    const std::size_t firstEventCount = firstSnapshot->events.size();
    const std::size_t firstTimerCount = firstSnapshot->timers.size();
    const std::size_t firstRecordingCount = firstSnapshot->recordings.size();

    pollingService.poll();

    assert(cache.hasSnapshot() == true);
    assert(cache.hasSnapshotForBackend(backendId) == true);

    const VdrSnapshot* secondSnapshot =
        cache.snapshotForBackend(backendId);

    assert(secondSnapshot != nullptr);
    assert(secondSnapshot->backendId == backendId);

    /*
     * This test intentionally does not modify the real VDR.
     *
     * A quiet second poll should not emit change events and should not create
     * refresh work. If the real VDR changes while the test is running, rerun
     * the test after the system is quiet again.
     */
    assert(pollingService.changeEvents().empty() == true);
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);

    assert(secondSnapshot->channels.size() == firstChannelCount);
    assert(secondSnapshot->events.size() == firstEventCount);
    assert(secondSnapshot->timers.size() == firstTimerCount);
    assert(secondSnapshot->recordings.size() == firstRecordingCount);

    std::cout
        << "real polling stability validation ok: "
        << "backend=" << secondSnapshot->backendId
        << " host=" << config.host
        << " port=" << config.port
        << " channels=" << secondSnapshot->channels.size()
        << " events=" << secondSnapshot->events.size()
        << " timers=" << secondSnapshot->timers.size()
        << " recordings=" << secondSnapshot->recordings.size()
        << " changeEvents=" << pollingService.changeEvents().size()
        << std::endl;

    return 0;
}
