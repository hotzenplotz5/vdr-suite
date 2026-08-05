#include "BackendAgentClient.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

namespace
{
std::atomic<bool> StopRequested(false);

void handleSignal(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) StopRequested.store(true);
}
}

int main(int argc, char** argv)
{
    std::string configPath = "/etc/vdr-suite/backend-agent.conf";
    bool once = false;
    bool rotateCredential = false;
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
        else
        {
            std::cerr << "usage: vdr-suite-backend-agent [--config PATH] "
                         "[--once | --rotate-credential]" << std::endl;
            return 64;
        }
    }
    if (once && rotateCredential)
    {
        std::cerr << "usage: vdr-suite-backend-agent [--config PATH] "
                     "[--once | --rotate-credential]" << std::endl;
        return 64;
    }

    BackendAgentClientConfig config;
    std::string reason;
    if (!BackendAgentClientRuntime::loadConfig(configPath, config, reason))
    {
        std::cerr << "Backend Agent configuration rejected: " << reason << std::endl;
        return 78;
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
