#include "SuiteBridgeObservationService.h"
#include "SuiteBridgeObservationWorker.h"
#include "SuiteBridgeSvdrpTransport.h"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

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

class CountingTransport final : public ISuiteBridgeLocalTransport
{
public:
    explicit CountingTransport(
        ISuiteBridgeLocalTransport& delegate)
        : delegate_(delegate)
    {
    }

    SuiteBridgeCommandReply execute(
        const SuiteBridgeLocalCommand command) override
    {
        commands.push_back(command);
        return delegate_.execute(command);
    }

    std::vector<SuiteBridgeLocalCommand> commands;

private:
    ISuiteBridgeLocalTransport& delegate_;
};

bool countersMonotonic(
    const SuiteBridgeSnapshotBaseline& previous,
    const SuiteBridgeSnapshotBaseline& current)
{
    return current.total >= previous.total &&
           current.channelSwitch >= previous.channelSwitch &&
           current.recording >= previous.recording &&
           current.replaying >= previous.replaying &&
           current.timerChange >= previous.timerChange;
}

bool validCurrentState(
    const SuiteBridgeObservationSnapshot& snapshot)
{
    if (snapshot.baseline.counterOverflow)
    {
        return snapshot.state == SuiteBridgeObservationState::Overflowed;
    }

    return snapshot.state == SuiteBridgeObservationState::SnapshotCurrent;
}

void printFailure(
    const std::string& stage,
    const SuiteBridgeObservationSnapshot& snapshot)
{
    std::cerr
        << "suitebridge live observation failed: stage="
        << stage
        << ", state="
        << suiteBridgeObservationStateName(snapshot.state)
        << ", diagnostic="
        << snapshot.diagnostic
        << std::endl;
}

}

int main()
{
    SuiteBridgeSvdrpTransportConfig transportConfig;
    transportConfig.host = environmentOrDefault(
        "VDR_SUITE_SUITEBRIDGE_SVDRP_HOST",
        "127.0.0.1");

    const std::string portText = environmentOrDefault(
        "VDR_SUITE_SUITEBRIDGE_SVDRP_PORT",
        "6419");

    if (!parsePort(portText, transportConfig.port))
    {
        std::cerr
            << "invalid VDR_SUITE_SUITEBRIDGE_SVDRP_PORT"
            << std::endl;
        return 1;
    }

    SuiteBridgeSvdrpTransport directTransport(transportConfig);
    CountingTransport countingTransport(directTransport);
    SuiteBridgeObservationConfig observationConfig;
    SuiteBridgeObservationService service(
        countingTransport,
        observationConfig);

    const SuiteBridgeObservationTimePoint initialTime{};
    service.start(initialTime);
    service.attempt(initialTime);

    const SuiteBridgeObservationSnapshot first = service.snapshot();

    if (!first.started ||
        !first.hasDiscovery ||
        !first.hasBaseline ||
        !first.baseline.active ||
        first.mutationsEnabled ||
        !validCurrentState(first))
    {
        printFailure("initial", first);
        return 1;
    }

    if (countingTransport.commands.size() != 2 ||
        countingTransport.commands.at(0) !=
            SuiteBridgeLocalCommand::DiscoverSchema1 ||
        countingTransport.commands.at(1) !=
            SuiteBridgeLocalCommand::Snapshot)
    {
        std::cerr
            << "suitebridge live observation initial command order invalid"
            << std::endl;
        return 1;
    }

    const SuiteBridgeObservationTimePoint secondTime =
        initialTime + observationConfig.pollInterval;

    if (!service.attemptDue(secondTime))
    {
        std::cerr
            << "suitebridge live observation second poll was not due"
            << std::endl;
        return 1;
    }

    service.attempt(secondTime);

    const SuiteBridgeObservationSnapshot second = service.snapshot();

    if (!second.started ||
        !second.hasDiscovery ||
        !second.hasBaseline ||
        !second.baseline.active ||
        second.mutationsEnabled ||
        !validCurrentState(second))
    {
        printFailure("second-poll", second);
        return 1;
    }

    if (countingTransport.commands.size() != 3 ||
        countingTransport.commands.at(2) !=
            SuiteBridgeLocalCommand::Snapshot)
    {
        std::cerr
            << "suitebridge live observation repeated discovery during trusted polling"
            << std::endl;
        return 1;
    }

    if (second.baseline.counterEpoch != first.baseline.counterEpoch)
    {
        std::cerr
            << "suitebridge live observation epoch changed between immediate polls"
            << std::endl;
        return 1;
    }

    if (!countersMonotonic(first.baseline, second.baseline))
    {
        std::cerr
            << "suitebridge live observation counters regressed"
            << std::endl;
        return 1;
    }

    if (!second.baseline.counterOverflow && !second.hasDelta)
    {
        std::cerr
            << "suitebridge live observation comparable poll has no delta"
            << std::endl;
        return 1;
    }

    service.refresh(secondTime + observationConfig.staleAfter);
    const SuiteBridgeObservationSnapshot stale = service.snapshot();

    if (stale.state != SuiteBridgeObservationState::SnapshotStale ||
        !stale.hasBaseline ||
        stale.baseline.counterEpoch != second.baseline.counterEpoch)
    {
        printFailure("stale", stale);
        return 1;
    }

    service.refresh(secondTime + observationConfig.offlineAfter);
    const SuiteBridgeObservationSnapshot offline = service.snapshot();

    if (offline.state != SuiteBridgeObservationState::Offline ||
        !offline.hasBaseline ||
        offline.baseline.counterEpoch != second.baseline.counterEpoch)
    {
        printFailure("offline", offline);
        return 1;
    }

    service.stop(secondTime + observationConfig.offlineAfter);
    const SuiteBridgeObservationSnapshot stopped = service.snapshot();

    if (stopped.started ||
        stopped.nextAttemptAt ||
        stopped.mutationsEnabled ||
        stopped.state != SuiteBridgeObservationState::Offline)
    {
        printFailure("stopped", stopped);
        return 1;
    }

    SuiteBridgeSvdrpTransport workerTransport(transportConfig);
    SuiteBridgeObservationWorker worker(workerTransport, observationConfig);

    worker.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    worker.stop();

    if (worker.running())
    {
        std::cerr
            << "suitebridge live observation worker remained active after stop"
            << std::endl;
        return 1;
    }

    const SuiteBridgeObservationSnapshot workerSnapshot = worker.snapshot();

    if (workerSnapshot.started ||
        workerSnapshot.nextAttemptAt ||
        workerSnapshot.mutationsEnabled ||
        workerSnapshot.state != SuiteBridgeObservationState::Offline)
    {
        printFailure("worker-stop", workerSnapshot);
        return 1;
    }

    std::cout
        << "suitebridge live observation: state="
        << suiteBridgeObservationStateName(second.state)
        << ", host="
        << transportConfig.host
        << ", port="
        << transportConfig.port
        << ", commands=CAPS,SNAP,SNAP"
        << ", epoch="
        << second.baseline.counterEpoch
        << ", total="
        << second.baseline.total
        << ", overflow="
        << (second.baseline.counterOverflow ? "true" : "false")
        << ", worker_stop=clean"
        << std::endl;

    std::cout
        << "test_suite_bridge_observation_live passed"
        << std::endl;
    return 0;
}
