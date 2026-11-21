#include "BasicHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrChangeState.h"
#include "VdrConfig.h"

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
            << "test_real_change_state skipped "
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

    const VdrChangeState state = adapter.getChangeState();

    assert(state.isEmpty() == false);
    assert(state.bootId.empty() == false);

    /*
     * Real restfulapi instances may legitimately report zero for some update
     * counters, for example when no timers exist. The important integration
     * contract here is that boot identity and at least one domain update
     * counter are visible from the real endpoint.
     */
    assert(
        state.channelsVersion > 0 ||
        state.recordingsVersion > 0 ||
        state.timersVersion > 0 ||
        state.eventsVersion > 0);

    std::cout
        << "real change-state validation ok: "
        << "host=" << config.host
        << " port=" << config.port
        << " bootId=" << state.bootId
        << " channelsVersion=" << state.channelsVersion
        << " recordingsVersion=" << state.recordingsVersion
        << " timersVersion=" << state.timersVersion
        << " eventsVersion=" << state.eventsVersion
        << std::endl;

    return 0;
}
