#include "SuiteBridgeHandshakeService.h"
#include "SuiteBridgeSvdrpTransport.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace vdrsuite::agent;

namespace
{

std::string environmentOrDefault(
    const char* name,
    const std::string& fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || *value == '\0')
    {
        return fallback;
    }

    return value;
}

bool parsePort(
    const std::string& text,
    int& port)
{
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);

    if (errno != 0 ||
        end == text.c_str() ||
        *end != '\0' ||
        value <= 0 ||
        value > 65535)
    {
        return false;
    }

    port = static_cast<int>(value);
    return true;
}

}

int main()
{
    SuiteBridgeSvdrpTransportConfig config;
    config.host = environmentOrDefault(
        "VDR_SUITE_SUITEBRIDGE_SVDRP_HOST",
        "127.0.0.1");

    const std::string portText = environmentOrDefault(
        "VDR_SUITE_SUITEBRIDGE_SVDRP_PORT",
        "6419");

    if (!parsePort(portText, config.port))
    {
        std::cerr
            << "invalid VDR_SUITE_SUITEBRIDGE_SVDRP_PORT"
            << std::endl;
        return 1;
    }

    SuiteBridgeSvdrpTransport transport(config);
    SuiteBridgeHandshakeService service(transport);
    const SuiteBridgeHandshakeResult result = service.perform();

    std::cout
        << "suitebridge live handshake: status="
        << suiteBridgeHandshakeStatusName(result.status)
        << ", host="
        << config.host
        << ", port="
        << config.port
        << std::endl;

    if (!result.ready())
    {
        if (!result.diagnostic.empty())
        {
            std::cerr
                << "suitebridge live handshake diagnostic: "
                << result.diagnostic
                << std::endl;
        }

        return 1;
    }

    if (result.mutationsEnabled)
    {
        std::cerr
            << "suitebridge live handshake unexpectedly enabled mutations"
            << std::endl;
        return 1;
    }

    std::cout
        << "suitebridge live contract: plugin="
        << result.discovery.pluginName
        << ", version="
        << result.discovery.pluginVersion
        << ", discovery_schema="
        << result.discovery.discoverySchema
        << ", capability_schema="
        << result.discovery.capabilitySchema
        << ", snapshot_schema="
        << result.discovery.snapshotSchema
        << ", local_contract_schema="
        << result.discovery.localContractSchema
        << ", active="
        << (result.baseline.active ? "true" : "false")
        << ", overflow="
        << (result.baseline.counterOverflow ? "true" : "false")
        << std::endl;

    std::cout
        << "test_suite_bridge_svdrp_transport_live passed"
        << std::endl;
    return 0;
}
