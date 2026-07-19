#include "SuiteBridgeEmbeddedAgentRuntime.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace vdrsuite::agent;
using namespace std::chrono_literals;

namespace
{

class ThreadSafeFakeTransport final : public ISuiteBridgeLocalTransport
{
public:
    explicit ThreadSafeFakeTransport(
        std::vector<SuiteBridgeCommandReply> replies)
        : replies_(std::move(replies))
    {
    }

    SuiteBridgeCommandReply execute(
        const SuiteBridgeLocalCommand command) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        commands_.push_back(command);
        changed_.notify_all();

        if (nextReply_ >= replies_.size())
        {
            SuiteBridgeCommandReply value;
            value.transportStatus = SuiteBridgeTransportStatus::Failed;
            value.diagnostic = "fixture exhausted";
            return value;
        }

        return replies_[nextReply_++];
    }

    bool waitForCalls(
        const std::size_t count,
        const std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return changed_.wait_for(
            lock,
            timeout,
            [this, count]() {
                return commands_.size() >= count;
            });
    }

    std::vector<SuiteBridgeLocalCommand> commands() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return commands_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<SuiteBridgeCommandReply> replies_;
    std::vector<SuiteBridgeLocalCommand> commands_;
    std::size_t nextReply_ = 0;
};

SuiteBridgeCommandReply reply(
    const int code,
    std::string payload)
{
    SuiteBridgeCommandReply value;
    value.transportStatus = SuiteBridgeTransportStatus::Success;
    value.replyCode = code;
    value.payload = std::move(payload);
    return value;
}

std::string discovery()
{
    return
        "{\"discovery_schema\":1,"
        "\"plugin_name\":\"suitebridge\","
        "\"plugin_version\":\"0.10.0\","
        "\"capability_schema\":1,"
        "\"snapshot_schema\":2,"
        "\"local_contract_schema\":2,"
        "\"capabilities\":["
        "{\"id\":\"snapshots\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"},"
        "{\"id\":\"mutations\",\"state\":\"disabled\"}]}";
}

std::string snapshot()
{
    return
        "{\"contract_schema\":2,"
        "\"capability_schema\":1,"
        "\"snapshot_schema\":2,"
        "\"active\":true,"
        "\"total\":4,"
        "\"channel_switch\":4,"
        "\"recording\":0,"
        "\"replaying\":0,"
        "\"timer_change\":0,"
        "\"counter_epoch\":\"11111111111111111111111111111111\","
        "\"counter_overflow\":false}";
}

SuiteBridgeEmbeddedAgentConfig fastConfig()
{
    SuiteBridgeEmbeddedAgentConfig value;
    value.backendId = "default";
    value.enabled = true;
    value.observation.pollInterval = 20ms;
    value.observation.staleAfter = 60ms;
    value.observation.offlineAfter = 200ms;
    value.observation.reconnectInitial = 20ms;
    value.observation.reconnectMaximum = 40ms;
    return value;
}

void testDisabledRuntimeStartsNoWorker()
{
    SuiteBridgeEmbeddedAgentConfig config;
    config.backendId = "default";
    config.enabled = false;

    SuiteBridgeEmbeddedAgentRuntime runtime(config);
    runtime.start();

    const SuiteBridgeEmbeddedAgentHealth health = runtime.health();
    assert(health.backendId == "default");
    assert(!health.configured);
    assert(!health.running);
    assert(health.observation.state ==
           SuiteBridgeObservationState::NotConfigured);
    assert(!health.observation.mutationsEnabled);

    runtime.stop();
}

void testInjectedTransportPublishesBackendScopedHealth()
{
    auto transport = std::make_unique<ThreadSafeFakeTransport>(
        std::vector<SuiteBridgeCommandReply>{
            reply(900, discovery()),
            reply(900, snapshot())
        });
    ThreadSafeFakeTransport* transportView = transport.get();

    SuiteBridgeEmbeddedAgentRuntime runtime(
        fastConfig(),
        std::move(transport));

    runtime.start();
    runtime.start();

    assert(transportView->waitForCalls(2, 1s));

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        if (runtime.health().observation.state ==
            SuiteBridgeObservationState::SnapshotCurrent)
        {
            break;
        }

        std::this_thread::sleep_for(5ms);
    }

    const SuiteBridgeEmbeddedAgentHealth current = runtime.health();
    assert(current.backendId == "default");
    assert(current.configured);
    assert(current.running);
    assert(current.observation.state ==
           SuiteBridgeObservationState::SnapshotCurrent);
    assert(current.observation.hasBaseline);
    assert(current.observation.baseline.total == 4);
    assert(!current.observation.mutationsEnabled);

    const auto commands = transportView->commands();
    assert(commands.size() >= 2);
    assert(commands[0] == SuiteBridgeLocalCommand::DiscoverSchema1);
    assert(commands[1] == SuiteBridgeLocalCommand::Snapshot);

    runtime.stop();
    runtime.stop();

    const SuiteBridgeEmbeddedAgentHealth stopped = runtime.health();
    assert(!stopped.running);
    assert(stopped.observation.state ==
           SuiteBridgeObservationState::Offline);
}

void testBackendIdentityIsBounded()
{
    SuiteBridgeEmbeddedAgentConfig config;
    config.backendId = std::string(256, 'a');
    config.enabled = false;

    SuiteBridgeEmbeddedAgentRuntime runtime(config);
    assert(runtime.health().backendId.size() == 128);
}

}

int main()
{
    testDisabledRuntimeStartsNoWorker();
    testInjectedTransportPublishesBackendScopedHealth();
    testBackendIdentityIsBounded();

    std::cout
        << "test_suite_bridge_embedded_agent_runtime passed"
        << std::endl;
    return 0;
}
