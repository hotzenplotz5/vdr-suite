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

void printPlan(const SnapshotUpdatePlan& plan)
{
    std::cout
        << "SnapshotUpdatePlan:"
        << std::endl
        << "  status:     " << (plan.shouldRefreshStatus() ? "yes" : "no") << std::endl
        << "  channels:   " << (plan.shouldRefreshChannels() ? "yes" : "no") << std::endl
        << "  recordings: " << (plan.shouldRefreshRecordings() ? "yes" : "no") << std::endl
        << "  timers:     " << (plan.shouldRefreshTimers() ? "yes" : "no") << std::endl
        << "  events:     " << (plan.shouldRefreshEvents() ? "yes" : "no") << std::endl
        << "  full:       " << (plan.requiresFullSnapshot() ? "yes" : "no") << std::endl;
}

} // namespace

int main()
{
    const std::string host = envOrDefault("VDR_SUITE_LOCAL_RESTFULAPI_HOST", "127.0.0.1");
    const int port = envPortOrDefault("VDR_SUITE_LOCAL_RESTFULAPI_PORT", 8002);

    std::cout
        << "Running local partial refresh validation against "
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

    const VdrSnapshot& firstSnapshot = pollingService.snapshot();

    std::cout
        << "Initial snapshot:"
        << std::endl
        << "  channels:   " << firstSnapshot.channels.size() << std::endl
        << "  recordings: " << firstSnapshot.recordings.size() << std::endl
        << "  timers:     " << firstSnapshot.timers.size() << std::endl
        << "  events:     " << firstSnapshot.events.size() << std::endl
        << std::endl;

    std::cout
        << "Now create exactly one real VDR change." << std::endl
        << "Recommended: create, edit, activate, deactivate or delete one timer." << std::endl
        << "Then press ENTER here to run the second poll." << std::endl;

    std::string line;
    std::getline(std::cin, line);

    pollingService.poll();

    const SnapshotUpdatePlan& plan = pollingService.lastUpdatePlan();

    std::cout << std::endl;
    printPlan(plan);

    std::cout
        << std::endl
        << "Detected change events: "
        << pollingService.changeEvents().size()
        << std::endl;

    assert(plan.hasRefreshWork());
    assert(!plan.requiresFullSnapshot());

    const VdrSnapshot& secondSnapshot = pollingService.snapshot();

    std::cout
        << "Updated snapshot:"
        << std::endl
        << "  channels:   " << secondSnapshot.channels.size() << std::endl
        << "  recordings: " << secondSnapshot.recordings.size() << std::endl
        << "  timers:     " << secondSnapshot.timers.size() << std::endl
        << "  events:     " << secondSnapshot.events.size() << std::endl;

    std::cout
        << "Local partial refresh validation passed"
        << std::endl;

    return 0;
}
