#include "SuiteBridgeEmbeddedAgentRuntime.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using namespace vdrsuite::agent;
using namespace std::chrono_literals;

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

bool parsePositiveInteger(
    const std::string& text,
    int& value)
{
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);

    if (errno != 0 ||
        end == text.c_str() ||
        end == nullptr ||
        *end != '\0' ||
        parsed <= 0 ||
        parsed > 3600000)
    {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

bool environmentBoolean(
    const char* name,
    const bool fallback)
{
    const std::string value = environmentOrDefault(
        name,
        fallback ? "true" : "false");

    if (value == "1" || value == "true" || value == "yes")
    {
        return true;
    }

    if (value == "0" || value == "false" || value == "no")
    {
        return false;
    }

    return fallback;
}

bool currentState(
    const SuiteBridgeObservationSnapshot& snapshot)
{
    if (snapshot.baseline.counterOverflow)
    {
        return snapshot.state == SuiteBridgeObservationState::Overflowed;
    }

    return snapshot.state == SuiteBridgeObservationState::SnapshotCurrent;
}

bool validCurrentHealth(
    const SuiteBridgeEmbeddedAgentHealth& health)
{
    return health.configured &&
           health.running &&
           health.observation.started &&
           health.observation.hasDiscovery &&
           health.observation.hasBaseline &&
           health.observation.baseline.active &&
           !health.observation.mutationsEnabled &&
           currentState(health.observation);
}

bool writeResult(
    const std::string& path,
    const std::string& status,
    const std::string& initialEpoch,
    const std::string& finalEpoch,
    const bool sawDegraded,
    const SuiteBridgeObservationSnapshot& snapshot)
{
    if (path.empty())
    {
        return true;
    }

    const std::string temporaryPath = path + ".tmp";
    std::ofstream output(temporaryPath, std::ios::trunc);

    if (!output)
    {
        return false;
    }

    output
        << "{\n"
        << "  \"status\": \"" << status << "\",\n"
        << "  \"initial_epoch\": \"" << initialEpoch << "\",\n"
        << "  \"final_epoch\": \"" << finalEpoch << "\",\n"
        << "  \"saw_degraded\": "
        << (sawDegraded ? "true" : "false") << ",\n"
        << "  \"state\": \""
        << suiteBridgeObservationStateName(snapshot.state) << "\",\n"
        << "  \"total\": " << snapshot.baseline.total << ",\n"
        << "  \"counter_overflow\": "
        << (snapshot.baseline.counterOverflow ? "true" : "false") << ",\n"
        << "  \"mutations_enabled\": "
        << (snapshot.mutationsEnabled ? "true" : "false") << "\n"
        << "}\n";

    output.close();

    if (!output)
    {
        std::remove(temporaryPath.c_str());
        return false;
    }

    if (std::rename(temporaryPath.c_str(), path.c_str()) != 0)
    {
        std::remove(temporaryPath.c_str());
        return false;
    }

    return true;
}

void printFailure(
    const std::string& stage,
    const SuiteBridgeEmbeddedAgentHealth& health)
{
    std::cerr
        << "SB.10d live runtime failed: stage=" << stage
        << ", configured=" << (health.configured ? "true" : "false")
        << ", running=" << (health.running ? "true" : "false")
        << ", state="
        << suiteBridgeObservationStateName(health.observation.state)
        << ", diagnostic=" << health.observation.diagnostic
        << std::endl;
}

}

int main()
{
    SuiteBridgeEmbeddedAgentConfig config;
    config.backendId = environmentOrDefault(
        "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
        "default");
    config.enabled = true;
    config.transport.host = environmentOrDefault(
        "VDR_SUITE_SUITE_BRIDGE_HOST",
        "127.0.0.1");

    int port = 6419;
    if (!parsePositiveInteger(
            environmentOrDefault(
                "VDR_SUITE_SUITE_BRIDGE_PORT",
                "6419"),
            port) ||
        port > 65535)
    {
        std::cerr << "invalid VDR_SUITE_SUITE_BRIDGE_PORT" << std::endl;
        return 1;
    }
    config.transport.port = port;

    config.transport.connectTimeout = 1000ms;
    config.transport.ioTimeout = 1000ms;
    config.transport.operationTimeout = 3000ms;
    config.observation.pollInterval = 250ms;
    config.observation.staleAfter = 750ms;
    config.observation.offlineAfter = 3000ms;
    config.observation.reconnectInitial = 250ms;
    config.observation.reconnectMaximum = 1000ms;

    int timeoutSeconds = 90;
    if (!parsePositiveInteger(
            environmentOrDefault(
                "VDR_SUITE_SB10D_TIMEOUT_SECONDS",
                "90"),
            timeoutSeconds))
    {
        std::cerr << "invalid VDR_SUITE_SB10D_TIMEOUT_SECONDS" << std::endl;
        return 1;
    }

    const bool expectEpochChange = environmentBoolean(
        "VDR_SUITE_SB10D_EXPECT_EPOCH_CHANGE",
        true);
    const std::string readyFile = environmentOrDefault(
        "VDR_SUITE_SB10D_READY_FILE",
        "");
    const std::string resultFile = environmentOrDefault(
        "VDR_SUITE_SB10D_RESULT_FILE",
        "");

    SuiteBridgeEmbeddedAgentRuntime runtime(config);
    runtime.start();

    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(timeoutSeconds);

    SuiteBridgeEmbeddedAgentHealth health;
    bool initialReady = false;

    while (std::chrono::steady_clock::now() < deadline)
    {
        health = runtime.health();

        if (health.observation.mutationsEnabled)
        {
            printFailure("mutation-enabled", health);
            runtime.stop();
            return 1;
        }

        if (validCurrentHealth(health))
        {
            initialReady = true;
            break;
        }

        std::this_thread::sleep_for(100ms);
    }

    if (!initialReady)
    {
        printFailure("initial-current-timeout", health);
        runtime.stop();
        return 1;
    }

    const std::string initialEpoch =
        health.observation.baseline.counterEpoch;

    if (initialEpoch.empty())
    {
        printFailure("empty-initial-epoch", health);
        runtime.stop();
        return 1;
    }

    if (!writeResult(
            readyFile,
            "ready",
            initialEpoch,
            initialEpoch,
            false,
            health.observation))
    {
        std::cerr << "failed to write SB.10d ready file" << std::endl;
        runtime.stop();
        return 1;
    }

    std::string finalEpoch = initialEpoch;
    bool sawDegraded = false;
    bool restartAccepted = !expectEpochChange;

    while (!restartAccepted &&
           std::chrono::steady_clock::now() < deadline)
    {
        health = runtime.health();

        if (health.observation.mutationsEnabled)
        {
            printFailure("mutation-enabled-after-restart", health);
            runtime.stop();
            return 1;
        }

        if (health.observation.state !=
                SuiteBridgeObservationState::SnapshotCurrent &&
            health.observation.state !=
                SuiteBridgeObservationState::Overflowed)
        {
            sawDegraded = true;
        }

        if (validCurrentHealth(health) &&
            health.observation.baseline.counterEpoch != initialEpoch)
        {
            finalEpoch = health.observation.baseline.counterEpoch;
            restartAccepted = true;
            break;
        }

        std::this_thread::sleep_for(100ms);
    }

    if (!restartAccepted)
    {
        printFailure("epoch-change-timeout", health);
        runtime.stop();
        return 1;
    }

    runtime.stop();
    const SuiteBridgeEmbeddedAgentHealth stopped = runtime.health();

    if (stopped.running ||
        stopped.observation.started ||
        stopped.observation.nextAttemptAt ||
        stopped.observation.mutationsEnabled ||
        stopped.observation.state != SuiteBridgeObservationState::Offline)
    {
        printFailure("worker-stop", stopped);
        return 1;
    }

    if (!writeResult(
            resultFile,
            "passed",
            initialEpoch,
            finalEpoch,
            sawDegraded,
            health.observation))
    {
        std::cerr << "failed to write SB.10d result file" << std::endl;
        return 1;
    }

    std::cout
        << "SB.10d live embedded runtime passed: backend="
        << config.backendId
        << ", initial_epoch=" << initialEpoch
        << ", final_epoch=" << finalEpoch
        << ", saw_degraded=" << (sawDegraded ? "true" : "false")
        << ", final_state="
        << suiteBridgeObservationStateName(health.observation.state)
        << ", total=" << health.observation.baseline.total
        << ", overflow="
        << (health.observation.baseline.counterOverflow ? "true" : "false")
        << ", worker_stop=clean"
        << std::endl;

    return 0;
}
