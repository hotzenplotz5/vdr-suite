#include "SuiteBridgeObservationWorker.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
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
            SuiteBridgeCommandReply reply;
            reply.transportStatus = SuiteBridgeTransportStatus::Failed;
            reply.diagnostic = "fixture exhausted";
            return reply;
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

private:
    std::mutex mutex_;
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

SuiteBridgeCommandReply failure()
{
    SuiteBridgeCommandReply value;
    value.transportStatus = SuiteBridgeTransportStatus::Failed;
    value.diagnostic = "connection failed";
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
        "\"total\":0,"
        "\"channel_switch\":0,"
        "\"recording\":0,"
        "\"replaying\":0,"
        "\"timer_change\":0,"
        "\"counter_epoch\":\"11111111111111111111111111111111\","
        "\"counter_overflow\":false}";
}

SuiteBridgeObservationConfig fastConfig()
{
    SuiteBridgeObservationConfig value;
    value.pollInterval = 20ms;
    value.staleAfter = 60ms;
    value.offlineAfter = 200ms;
    value.reconnectInitial = 20ms;
    value.reconnectMaximum = 40ms;
    return value;
}

void testStartStopAndSuccessfulPublication()
{
    ThreadSafeFakeTransport transport({
        reply(900, discovery()),
        reply(900, snapshot())
    });

    SuiteBridgeObservationWorker worker(transport, fastConfig());

    worker.start();
    worker.start();

    assert(transport.waitForCalls(2, 1s));

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        if (worker.snapshot().state ==
            SuiteBridgeObservationState::SnapshotCurrent)
        {
            break;
        }

        std::this_thread::sleep_for(5ms);
    }

    assert(worker.running());
    assert(worker.snapshot().state ==
           SuiteBridgeObservationState::SnapshotCurrent);
    assert(worker.snapshot().hasBaseline);
    assert(!worker.snapshot().mutationsEnabled);

    worker.stop();
    worker.stop();

    assert(!worker.running());
    assert(worker.snapshot().state ==
           SuiteBridgeObservationState::Offline);
}

void testStopInterruptsReconnectWait()
{
    ThreadSafeFakeTransport transport({failure()});

    SuiteBridgeObservationConfig value = fastConfig();
    value.reconnectInitial = 5s;
    value.reconnectMaximum = 5s;

    SuiteBridgeObservationWorker worker(transport, value);
    worker.start();

    assert(transport.waitForCalls(1, 1s));

    const auto started = std::chrono::steady_clock::now();
    worker.stop();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);

    assert(elapsed < 1000ms);
    assert(!worker.running());
    assert(worker.snapshot().state ==
           SuiteBridgeObservationState::Offline);
}

}

int main()
{
    testStartStopAndSuccessfulPublication();
    testStopInterruptsReconnectWait();

    std::cout
        << "test_suite_bridge_observation_worker passed"
        << std::endl;
    return 0;
}