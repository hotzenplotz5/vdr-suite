#include "BasicHttpClient.h"
#include "PollingService.h"
#include "RestfulApiVdrAdapter.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrConfig.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string envOrDefault(const char* name, const std::string& fallback)
{
    const char* value = std::getenv(name);
    return value == nullptr || std::string(value).empty()
        ? fallback
        : std::string(value);
}

int envPortOrDefault(const char* name, int fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

VdrConfig makeConfig(const std::string& host, int port)
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = host;
    config.port = port;
    return config;
}

} // namespace

int main()
{
    const std::string host = envOrDefault("VDR_SUITE_LOCAL_RESTFULAPI_HOST", "127.0.0.1");
    const int port = envPortOrDefault("VDR_SUITE_LOCAL_RESTFULAPI_PORT", 8002);

    std::cout
        << "Running local snapshot runtime integration test against "
        << host
        << ":"
        << port
        << std::endl;

    BasicHttpClient httpClient(host, port);
    VdrConfig config = makeConfig(host, port);
    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    PollingService pollingService(builder, service, cacheService);

    pollingService.poll();

    assert(cache.hasSnapshot());
    assert(pollingService.changeEvents().empty());
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);

    const VdrSnapshot& firstSnapshot = pollingService.snapshot();

    assert(firstSnapshot.status.enabled == true);
    assert(firstSnapshot.status.mode == "restfulapi");
    assert(firstSnapshot.status.host == host);
    assert(firstSnapshot.status.port == port);
    assert(firstSnapshot.status.state == "restfulapi");

    assert(!firstSnapshot.channels.empty());

    const std::size_t firstChannelCount = firstSnapshot.channels.size();
    const std::size_t firstRecordingCount = firstSnapshot.recordings.size();
    const std::size_t firstTimerCount = firstSnapshot.timers.size();
    const std::size_t firstEventCount = firstSnapshot.events.size();

    pollingService.poll();

    assert(cache.hasSnapshot());
    assert(pollingService.changeEvents().empty());
    assert(pollingService.lastUpdatePlan().hasRefreshWork() == false);

    const VdrSnapshot& secondSnapshot = pollingService.snapshot();

    assert(secondSnapshot.channels.size() == firstChannelCount);
    assert(secondSnapshot.recordings.size() == firstRecordingCount);
    assert(secondSnapshot.timers.size() == firstTimerCount);
    assert(secondSnapshot.events.size() == firstEventCount);

    std::cout
        << "Local snapshot runtime integration test passed"
        << std::endl
        << "Channels: "
        << secondSnapshot.channels.size()
        << std::endl
        << "Recordings: "
        << secondSnapshot.recordings.size()
        << std::endl
        << "Timers: "
        << secondSnapshot.timers.size()
        << std::endl
        << "Events: "
        << secondSnapshot.events.size()
        << std::endl;

    return 0;
}
