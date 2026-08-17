#include "BackendAgentClient.h"
#include "BackendAgentCommandClient.h"
#include "SuiteBridgeNativeTimerCreateTransport.h"
#include "SuiteBridgeNativeTimerDeleteTransport.h"
#include "SuiteBridgeNativeTimerModifyTransport.h"
#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace
{
std::atomic<bool> StopRequested(false);

void handleSignal(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) StopRequested.store(true);
}

bool loopbackHost(const std::string& host)
{
    return host == "127.0.0.1" || host == "::1" || host == "localhost";
}

bool portNumber(const std::string& value, std::uint16_t& port)
{
    if (value.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' ||
        parsed == 0 || parsed > 65535) return false;
    port = static_cast<std::uint16_t>(parsed);
    return true;
}

void usage()
{
    std::cerr
        << "usage: vdr-suite-backend-agent [--config PATH] "
           "[--once | --rotate-credential] "
           "[--native-probe --suitebridge-host LOOPBACK "
           "--suitebridge-port PORT]" << std::endl;
}
}

int main(int argc, char** argv)
{
    std::string configPath = "/etc/vdr-suite/backend-agent.conf";
    bool once = false;
    bool rotateCredential = false;
    bool nativeProbe = false;
    std::string suiteBridgeHost;
    std::uint16_t suiteBridgePort = 0;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--config" && index + 1 < argc)
        {
            configPath = argv[++index];
        }
        else if (argument == "--once")
        {
            once = true;
        }
        else if (argument == "--rotate-credential")
        {
            rotateCredential = true;
        }
        else if (argument == "--native-probe")
        {
            nativeProbe = true;
        }
        else if (argument == "--suitebridge-host" && index + 1 < argc)
        {
            suiteBridgeHost = argv[++index];
        }
        else if (argument == "--suitebridge-port" && index + 1 < argc)
        {
            if (!portNumber(argv[++index], suiteBridgePort))
            {
                usage();
                return 64;
            }
        }
        else
        {
            usage();
            return 64;
        }
    }
    if (once && rotateCredential)
    {
        usage();
        return 64;
    }
    if (nativeProbe)
    {
        if (!loopbackHost(suiteBridgeHost) || suiteBridgePort == 0)
        {
            std::cerr << "native probe requires an explicit loopback SuiteBridge host and port" << std::endl;
            return 78;
        }
    }
    else if (!suiteBridgeHost.empty() || suiteBridgePort != 0)
    {
        usage();
        return 64;
    }

    BackendAgentClientConfig config;
    std::string reason;
    if (!BackendAgentClientRuntime::loadConfig(configPath, config, reason))
    {
        std::cerr << "Backend Agent configuration rejected: " << reason << std::endl;
        return 78;
    }

    std::unique_ptr<vdrsuite::agent::SuiteBridgeNativeTimerCreateTransport>
        nativeTimerCreateTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeNativeTimerDeleteTransport>
        nativeTimerDeleteTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeNativeTimerModifyTransport>
        nativeTimerModifyTransport;
    if (!config.suiteBridgeHost.empty())
    {
        vdrsuite::agent::SuiteBridgeSvdrpTransportConfig timerTransportConfig;
        timerTransportConfig.host = config.suiteBridgeHost;
        timerTransportConfig.port = config.suiteBridgePort;
        nativeTimerCreateTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeNativeTimerCreateTransport>(
                timerTransportConfig);
        nativeTimerDeleteTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeNativeTimerDeleteTransport>(
                timerTransportConfig);
        nativeTimerModifyTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeNativeTimerModifyTransport>(
                timerTransportConfig);
        config.nativeTimerCreateTransport =
            nativeTimerCreateTransport.get();
        config.nativeTimerDeleteTransport =
            nativeTimerDeleteTransport.get();
        config.nativeTimerModifyTransport =
            nativeTimerModifyTransport.get();
    }

    std::unique_ptr<vdrsuite::agent::SuiteBridgeSvdrpTransport> nativeTransport;
    if (nativeProbe)
    {
        if (!config.commandTypes.empty())
        {
            std::cerr << "native probe CLI activation requires COMMAND_TYPES to remain empty" << std::endl;
            return 78;
        }
        config.commandTypes = {"vdr.native.probe"};
        vdrsuite::agent::SuiteBridgeSvdrpTransportConfig transportConfig;
        transportConfig.host = suiteBridgeHost;
        transportConfig.port = suiteBridgePort;
        nativeTransport =
            std::make_unique<vdrsuite::agent::SuiteBridgeSvdrpTransport>(transportConfig);
        setBackendAgentNativeProbeTransport(nativeTransport.get());
    }

    CurlBackendAgentControlPlaneTransport transport(config);
    BackendAgentClientRuntime runtime(
        config,
        transport,
        {},
        [](const std::string& message) { std::cout << message << std::endl; });

    if (once || rotateCredential)
    {
        if (!runtime.synchronize(reason))
        {
            std::cerr << "Backend Agent synchronization failed: " << reason << std::endl;
            return 1;
        }
        if (rotateCredential)
        {
            if (!runtime.rotateCredential(reason))
            {
                std::cerr << "Backend Agent credential rotation failed: "
                          << reason << std::endl;
                return 1;
            }
            std::cout << "Backend Agent credential rotation succeeded" << std::endl;
            return 0;
        }
        std::cout << "Backend Agent synchronization succeeded" << std::endl;
        return 0;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    return runtime.run([] { return StopRequested.load(); });
}
