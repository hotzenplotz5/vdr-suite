#include "BasicHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

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
            << "test_real_restfulapi_integration skipped "
            << "(set VDR_SUITE_REAL_VDR_TEST=1 to run)"
            << std::endl;
        return 0;
    }

    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = envOrDefault("VDR_SUITE_VDR_HOST", "127.0.0.1");
    config.port = envPortOrDefault("VDR_SUITE_VDR_PORT", 8002);

    BasicHttpClient httpClient(config.host, config.port);
    RestfulApiVdrAdapter adapter(config, httpClient);

    const VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == config.host);
    assert(status.port == config.port);
    assert(status.state == "restfulapi");

    const auto channels = adapter.getChannels();
    const auto events = adapter.getEvents();
    const auto timers = adapter.getTimers();
    const auto recordings = adapter.getRecordings();
    const auto changeState = adapter.getChangeState();

    std::cout
        << "real restfulapi integration ok: "
        << "host=" << config.host
        << " port=" << config.port
        << " channels=" << channels.size()
        << " events=" << events.size()
        << " timers=" << timers.size()
        << " recordings=" << recordings.size()
        << " bootId=" << changeState.bootId
        << std::endl;

    return 0;
}
