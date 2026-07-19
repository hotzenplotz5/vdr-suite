#include "SuiteBridgeObservationService.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace vdrsuite::agent;
using namespace std::chrono_literals;

namespace
{

class FakeTransport final : public ISuiteBridgeLocalTransport
{
public:
    explicit FakeTransport(
        std::vector<SuiteBridgeCommandReply> replies)
        : replies_(std::move(replies))
    {
    }

    SuiteBridgeCommandReply execute(
        const SuiteBridgeLocalCommand command) override
    {
        commands.push_back(command);

        if (nextReply_ >= replies_.size())
        {
            SuiteBridgeCommandReply reply;
            reply.transportStatus = SuiteBridgeTransportStatus::Failed;
            reply.diagnostic = "unexpected transport invocation";
            return reply;
        }

        return replies_[nextReply_++];
    }

    std::vector<SuiteBridgeLocalCommand> commands;

private:
    std::vector<SuiteBridgeCommandReply> replies_;
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

SuiteBridgeCommandReply transportFailure(
    const SuiteBridgeTransportStatus status,
    std::string diagnostic)
{
    SuiteBridgeCommandReply value;
    value.transportStatus = status;
    value.diagnostic = std::move(diagnostic);
    return value;
}

std::string discovery(
    const std::uint64_t discoverySchema = 1,
    const std::string& mutationState = "disabled")
{
    return
        "{\"discovery_schema\":" + std::to_string(discoverySchema) +
        ",\"plugin_name\":\"suitebridge\""
        ",\"plugin_version\":\"0.10.0\""
        ",\"capability_schema\":1"
        ",\"snapshot_schema\":2"
        ",\"local_contract_schema\":2"
        ",\"capabilities\":["
        "{\"id\":\"snapshots\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"},"
        "{\"id\":\"mutations\",\"state\":\"" +
        mutationState + "\"}]}";
}

std::string snapshot(
    const std::string& epoch,
    const std::uint64_t channelSwitch,
    const bool overflow = false)
{
    return
        "{\"contract_schema\":2"
        ",\"capability_schema\":1"
        ",\"snapshot_schema\":2"
        ",\"active\":true"
        ",\"total\":" + std::to_string(channelSwitch) +
        ",\"channel_switch\":" + std::to_string(channelSwitch) +
        ",\"recording\":0"
        ",\"replaying\":0"
        ",\"timer_change\":0"
        ",\"counter_epoch\":\"" + epoch + "\""
        ",\"counter_overflow\":" +
        std::string(overflow ? "true" : "false") + "}";
}

SuiteBridgeObservationConfig config()
{
    SuiteBridgeObservationConfig value;
    value.pollInterval = 5s;
    value.staleAfter = 15s;
    value.offlineAfter = 60s;
    value.reconnectInitial = 1s;
    value.reconnectMaximum = 30s;
    return value;
}

SuiteBridgeObservationTimePoint at(
    const std::chrono::milliseconds offset)
{
    return SuiteBridgeObservationTimePoint{} + offset;
}

void testInitialHandshakeAndSnapshotOnlyPolling()
{
    FakeTransport transport({
        reply(900, discovery()),
        reply(900, snapshot("11111111111111111111111111111111", 4)),
        reply(900, snapshot("11111111111111111111111111111111", 6))
    });

    SuiteBridgeObservationService service(transport, config());
    service.start(at(0ms));

    assert(service.snapshot().state ==
           SuiteBridgeObservationState::Connecting);
    assert(service.attemptDue(at(0ms)));

    service.attempt(at(0ms));

    assert(service.snapshot().state ==
           SuiteBridgeObservationState::SnapshotCurrent);
    assert(service.snapshot().hasBaseline);
    assert(!service.snapshot().hasDelta);
    assert(!service.snapshot().mutationsEnabled);
    assert(transport.commands.size() == 2);
    assert(transport.commands.at(0) ==
           SuiteBridgeLocalCommand::DiscoverSchema1);
    assert(transport.commands.at(1) ==
           SuiteBridgeLocalCommand::Snapshot);

    service.attempt(at(4999ms));
    assert(transport.commands.size() == 2);

    service.attempt(at(5000ms));

    assert(transport.commands.size() == 3);
    assert(transport.commands.back() ==
           SuiteBridgeLocalCommand::Snapshot);
    assert(service.snapshot().hasDelta);
    assert(service.snapshot().delta.total == 2);
    assert(service.snapshot().delta.channelSwitch == 2);
    assert(service.snapshot().consecutiveFailures == 0);
}

void testReconnectRediscoversAndResetsBackoff()
{
    FakeTransport transport({
        reply(900, discovery()),
        reply(900, snapshot("11111111111111111111111111111111", 4)),
        transportFailure(SuiteBridgeTransportStatus::Timeout, "timeout"),
        reply(900, discovery()),
        reply(900, snapshot("11111111111111111111111111111111", 5))
    });

    SuiteBridgeObservationService service(transport, config());
    service.start(at(0ms));
    service.attempt(at(0ms));
    service.attempt(at(5000ms));

    assert(service.snapshot().state ==
           SuiteBridgeObservationState::TransportDegraded);
    assert(service.snapshot().consecutiveFailures == 1);
    assert(service.snapshot().nextAttemptAt == at(6000ms));

    service.attempt(at(6000ms));

    assert(service.snapshot().state ==
           SuiteBridgeObservationState::SnapshotCurrent);
    assert(service.snapshot().consecutiveFailures == 0);
    assert(service.snapshot().nextAttemptAt == at(11000ms));
    assert(transport.commands.size() == 5);
    assert(transport.commands.at(2) ==
           SuiteBridgeLocalCommand::Snapshot);
    assert(transport.commands.at(3) ==
           SuiteBridgeLocalCommand::DiscoverSchema1);
    assert(transport.commands.at(4) ==
           SuiteBridgeLocalCommand::Snapshot);
}

void testDistinctUnavailableStates()
{
    {
        FakeTransport transport({
            transportFailure(
                SuiteBridgeTransportStatus::Unavailable,
                "not configured")
        });
        SuiteBridgeObservationService service(transport, config());
        service.start(at(0ms));
        service.attempt(at(0ms));

        assert(service.snapshot().state ==
               SuiteBridgeObservationState::NotConfigured);
        assert(!service.snapshot().nextAttemptAt);
    }

    {
        FakeTransport transport({reply(550, "plugin missing")});
        SuiteBridgeObservationService service(transport, config());
        service.start(at(0ms));
        service.attempt(at(0ms));

        assert(service.snapshot().state ==
               SuiteBridgeObservationState::PluginMissing);
        assert(service.snapshot().nextAttemptAt == at(1000ms));
    }

    {
        FakeTransport transport({reply(500, "unknown command")});
        SuiteBridgeObservationService service(transport, config());
        service.start(at(0ms));
        service.attempt(at(0ms));

        assert(service.snapshot().state ==
               SuiteBridgeObservationState::LegacyOrUnknown);
    }

    {
        FakeTransport transport({reply(900, discovery(2))});
        SuiteBridgeObservationService service(transport, config());
        service.start(at(0ms));
        service.attempt(at(0ms));

        assert(service.snapshot().state ==
               SuiteBridgeObservationState::Incompatible);
        assert(transport.commands.size() == 1);
    }
}

void testBoundedReconnectBackoff()
{
    std::vector<SuiteBridgeCommandReply> replies;

    for (int i = 0; i < 7; ++i)
    {
        replies.push_back(transportFailure(
            SuiteBridgeTransportStatus::Failed,
            "connection failed"));
    }

    FakeTransport transport(std::move(replies));
    SuiteBridgeObservationService service(transport, config());
    service.start(at(0ms));

    const std::vector<std::chrono::milliseconds> attempts = {
        0ms, 1000ms, 3000ms, 7000ms, 15000ms, 31000ms, 61000ms
    };
    const std::vector<std::chrono::milliseconds> nextAttempts = {
        1000ms, 3000ms, 7000ms, 15000ms, 31000ms, 61000ms, 91000ms
    };

    for (std::size_t index = 0; index < attempts.size(); ++index)
    {
        service.attempt(at(attempts.at(index)));
        assert(service.snapshot().nextAttemptAt ==
               at(nextAttempts.at(index)));
    }
}

void testFreshnessBoundaries()
{
    FakeTransport transport({
        reply(900, discovery()),
        reply(900, snapshot("11111111111111111111111111111111", 4)),
        transportFailure(SuiteBridgeTransportStatus::Failed, "offline")
    });

    SuiteBridgeObservationService service(transport, config());
    service.start(at(0ms));
    service.attempt(at(0ms));
    service.attempt(at(5000ms));

    service.refresh(at(14999ms));
    assert(service.snapshot().state ==
           SuiteBridgeObservationState::TransportDegraded);

    service.refresh(at(15000ms));
    assert(service.snapshot().state ==
           SuiteBridgeObservationState::SnapshotStale);

    service.refresh(at(59999ms));
    assert(service.snapshot().state ==
           SuiteBridgeObservationState::SnapshotStale);

    service.refresh(at(60000ms));
    assert(service.snapshot().state ==
           SuiteBridgeObservationState::Offline);
}

void testEpochOverflowAndCounterRegression()
{
    FakeTransport transport({
        reply(900, discovery("1"[0] - '0', "available")),
        reply(900, snapshot("11111111111111111111111111111111", 4)),
        reply(900, snapshot("11111111111111111111111111111111", 8)),
        reply(900, snapshot("22222222222222222222222222222222", 2)),
        reply(900, snapshot("22222222222222222222222222222222", 3, true))
    });

    SuiteBridgeObservationService service(transport, config());
    service.start(at(0ms));
    service.attempt(at(0ms));
    assert(!service.snapshot().mutationsEnabled);

    service.attempt(at(5000ms));
    assert(service.snapshot().hasDelta);
    assert(service.snapshot().delta.total == 4);

    service.attempt(at(10000ms));
    assert(!service.snapshot().hasDelta);
    assert(service.snapshot().baseline.counterEpoch ==
           "22222222222222222222222222222222");

    service.attempt(at(15000ms));
    assert(service.snapshot().state ==
           SuiteBridgeObservationState::Overflowed);
    assert(!service.snapshot().hasDelta);

    FakeTransport regressionTransport({
        reply(900, discovery()),
        reply(900, snapshot("33333333333333333333333333333333", 10)),
        reply(900, snapshot("33333333333333333333333333333333", 9))
    });

    SuiteBridgeObservationService regression(
        regressionTransport,
        config());
    regression.start(at(0ms));
    regression.attempt(at(0ms));
    regression.attempt(at(5000ms));

    assert(regression.snapshot().state ==
           SuiteBridgeObservationState::TransportDegraded);
    assert(regression.snapshot().baseline.total == 10);
    assert(!regression.snapshot().hasDelta);
    assert(regression.snapshot().diagnostic ==
           "suite bridge counters regressed within one epoch");
}

void testStateVocabularyAndStop()
{
    assert(std::string(suiteBridgeObservationStateName(
               SuiteBridgeObservationState::NotConfigured)) ==
           "not_configured");
    assert(std::string(suiteBridgeObservationStateName(
               SuiteBridgeObservationState::SnapshotCurrent)) ==
           "snapshot_current");
    assert(std::string(suiteBridgeObservationStateName(
               SuiteBridgeObservationState::TransportDegraded)) ==
           "transport_degraded");

    FakeTransport transport({});
    SuiteBridgeObservationService service(transport, config());
    service.start(at(0ms));
    service.start(at(1ms));
    service.stop(at(2ms));
    service.stop(at(3ms));

    assert(!service.snapshot().started);
    assert(service.snapshot().state ==
           SuiteBridgeObservationState::Offline);
    assert(!service.snapshot().nextAttemptAt);
}

}

int main()
{
    testInitialHandshakeAndSnapshotOnlyPolling();
    testReconnectRediscoversAndResetsBackoff();
    testDistinctUnavailableStates();
    testBoundedReconnectBackoff();
    testFreshnessBoundaries();
    testEpochOverflowAndCounterRegression();
    testStateVocabularyAndStop();

    std::cout
        << "test_suite_bridge_observation_service passed"
        << std::endl;
    return 0;
}