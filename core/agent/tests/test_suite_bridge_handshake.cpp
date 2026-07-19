#include "SuiteBridgeHandshakeService.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace vdrsuite::agent;

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
    const std::string& diagnostic)
{
    SuiteBridgeCommandReply value;
    value.transportStatus = status;
    value.diagnostic = diagnostic;
    return value;
}

std::string discovery(
    const std::uint64_t discoverySchema = 1,
    const std::uint64_t capabilitySchema = 1,
    const std::uint64_t snapshotSchema = 2,
    const std::uint64_t localContractSchema = 2,
    const std::string& capabilities =
        "[{\"id\":\"lifecycle\",\"state\":\"available\"},"
        "{\"id\":\"status-events\",\"state\":\"available\"},"
        "{\"id\":\"snapshots\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"},"
        "{\"id\":\"mutations\",\"state\":\"disabled\"}]")
{
    return
        "{\"discovery_schema\":" + std::to_string(discoverySchema) +
        ",\"plugin_name\":\"suitebridge\""
        ",\"plugin_version\":\"0.10.0\""
        ",\"capability_schema\":" + std::to_string(capabilitySchema) +
        ",\"snapshot_schema\":" + std::to_string(snapshotSchema) +
        ",\"local_contract_schema\":" +
        std::to_string(localContractSchema) +
        ",\"capabilities\":" + capabilities + "}";
}

std::string reorderedDiscovery()
{
    return
        "{"
        "\"ignored\":{\"nested\":[true,null,3]},"
        "\"capabilities\":["
        "{\"state\":\"available\",\"id\":\"snapshots\"},"
        "{\"id\":\"future-extra\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"},"
        "{\"id\":\"mutations\",\"state\":\"disabled\"}],"
        "\"local_contract_schema\":2,"
        "\"snapshot_schema\":2,"
        "\"plugin_version\":\"0.10.0\","
        "\"capability_schema\":1,"
        "\"plugin_name\":\"suitebridge\","
        "\"discovery_schema\":1"
        "}";
}

std::string snapshot(
    const std::string& epoch =
        "26f5b0fc557edf7767a4f2ea3a02584d",
    const bool active = true,
    const bool overflow = false,
    const std::uint64_t channelSwitch = 4,
    const std::uint64_t recording = 0,
    const std::uint64_t replaying = 0,
    const std::uint64_t timerChange = 0)
{
    const std::uint64_t total =
        channelSwitch + recording + replaying + timerChange;

    return
        "{\"contract_schema\":2"
        ",\"capability_schema\":1"
        ",\"snapshot_schema\":2"
        ",\"active\":" + std::string(active ? "true" : "false") +
        ",\"total\":" + std::to_string(total) +
        ",\"channel_switch\":" + std::to_string(channelSwitch) +
        ",\"recording\":" + std::to_string(recording) +
        ",\"replaying\":" + std::to_string(replaying) +
        ",\"timer_change\":" + std::to_string(timerChange) +
        ",\"counter_epoch\":\"" + epoch + "\""
        ",\"counter_overflow\":" +
        std::string(overflow ? "true" : "false") + "}";
}

SuiteBridgeHandshakeResult perform(
    FakeTransport& transport)
{
    SuiteBridgeHandshakeService service(transport);
    return service.perform();
}

void assertDiscoveryOnly(
    const FakeTransport& transport)
{
    assert(transport.commands.size() == 1);
    assert(transport.commands.front() ==
           SuiteBridgeLocalCommand::DiscoverSchema1);
}

void testSuccessfulHandshake()
{
    FakeTransport transport({
        reply(900, discovery()),
        reply(900, snapshot())
    });

    const SuiteBridgeHandshakeResult result = perform(transport);

    assert(result.ready());
    assert(result.status == SuiteBridgeHandshakeStatus::Ready);
    assert(!result.mutationsEnabled);
    assert(result.discovery.pluginName == "suitebridge");
    assert(result.discovery.pluginVersion == "0.10.0");
    assert(result.discovery.discoverySchema == 1);
    assert(result.discovery.capabilitySchema == 1);
    assert(result.discovery.snapshotSchema == 2);
    assert(result.discovery.localContractSchema == 2);
    assert(result.discovery.capabilityAvailable("snapshots"));
    assert(result.discovery.capabilityAvailable("local-contract"));
    assert(result.discovery.capabilityState("missing") ==
           SuiteBridgeCapabilityState::Unknown);
    assert(result.baseline.active);
    assert(result.baseline.total == 4);
    assert(result.baseline.channelSwitch == 4);
    assert(!result.baseline.counterOverflow);
    assert(transport.commands.size() == 2);
    assert(transport.commands.at(0) ==
           SuiteBridgeLocalCommand::DiscoverSchema1);
    assert(transport.commands.at(1) ==
           SuiteBridgeLocalCommand::Snapshot);
}

void testOrderAndAdditiveCapabilities()
{
    FakeTransport transport({
        reply(900, reorderedDiscovery()),
        reply(
            900,
            "{\"counter_overflow\":false,"
            "\"counter_epoch\":\"26f5b0fc557edf7767a4f2ea3a02584d\","
            "\"timer_change\":0,\"replaying\":0,\"recording\":0,"
            "\"channel_switch\":4,\"total\":4,\"active\":true,"
            "\"snapshot_schema\":2,\"capability_schema\":1,"
            "\"contract_schema\":2,\"future\":[1,2,3]}")
    });

    const SuiteBridgeHandshakeResult result = perform(transport);

    assert(result.ready());
    assert(result.discovery.capabilityAvailable("future-extra"));
    assert(!result.mutationsEnabled);
}

void testMutationFailClosed()
{
    const std::string withoutMutations =
        "[{\"id\":\"snapshots\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"}]";

    FakeTransport absentTransport({
        reply(900, discovery(1, 1, 2, 2, withoutMutations)),
        reply(900, snapshot())
    });

    const SuiteBridgeHandshakeResult absent = perform(absentTransport);
    assert(absent.ready());
    assert(absent.discovery.capabilityState("mutations") ==
           SuiteBridgeCapabilityState::Unknown);
    assert(!absent.mutationsEnabled);

    const std::string availableMutations =
        "[{\"id\":\"snapshots\",\"state\":\"available\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"},"
        "{\"id\":\"mutations\",\"state\":\"available\"}]";

    FakeTransport availableTransport({
        reply(900, discovery(1, 1, 2, 2, availableMutations)),
        reply(900, snapshot())
    });

    const SuiteBridgeHandshakeResult available =
        perform(availableTransport);
    assert(available.ready());
    assert(available.discovery.capabilityAvailable("mutations"));
    assert(!available.mutationsEnabled);
}

void testLegacyAndTransportFailures()
{
    FakeTransport unavailable({
        transportFailure(
            SuiteBridgeTransportStatus::Unavailable,
            "plugin unavailable")
    });
    assert(perform(unavailable).status ==
           SuiteBridgeHandshakeStatus::LegacyOrUnknown);
    assertDiscoveryOnly(unavailable);

    FakeTransport oldPlugin({reply(500, "unknown command")});
    assert(perform(oldPlugin).status ==
           SuiteBridgeHandshakeStatus::LegacyOrUnknown);
    assertDiscoveryOnly(oldPlugin);

    FakeTransport timeout({
        transportFailure(
            SuiteBridgeTransportStatus::Timeout,
            "timeout")
    });
    assert(perform(timeout).status ==
           SuiteBridgeHandshakeStatus::DiscoveryTransportError);
    assertDiscoveryOnly(timeout);

    FakeTransport rejected({reply(504, "unsupported")});
    assert(perform(rejected).status ==
           SuiteBridgeHandshakeStatus::DiscoveryReplyRejected);
    assertDiscoveryOnly(rejected);
}

void testSchemaFailuresStopBeforeSnapshot()
{
    struct Case
    {
        std::string payload;
        SuiteBridgeHandshakeStatus status;
    };

    const std::vector<Case> cases = {
        {discovery(2, 1, 2, 2),
         SuiteBridgeHandshakeStatus::IncompatibleDiscoverySchema},
        {discovery(1, 2, 2, 2),
         SuiteBridgeHandshakeStatus::IncompatibleCapabilitySchema},
        {discovery(1, 1, 3, 2),
         SuiteBridgeHandshakeStatus::IncompatibleSnapshotSchema},
        {discovery(1, 1, 2, 3),
         SuiteBridgeHandshakeStatus::IncompatibleLocalContractSchema}
    };

    for (const Case& testCase : cases)
    {
        FakeTransport transport({reply(900, testCase.payload)});
        assert(perform(transport).status == testCase.status);
        assertDiscoveryOnly(transport);
    }
}

void testDiscoveryPayloadFailures()
{
    FakeTransport invalid({reply(900, "{not-json}")});
    assert(perform(invalid).status ==
           SuiteBridgeHandshakeStatus::InvalidDiscoveryPayload);
    assertDiscoveryOnly(invalid);

    FakeTransport missing({
        reply(900, "{\"discovery_schema\":1}")
    });
    assert(perform(missing).status ==
           SuiteBridgeHandshakeStatus::InvalidDiscoveryPayload);
    assertDiscoveryOnly(missing);

    FakeTransport oversized({
        reply(
            900,
            std::string(
                SuiteBridgeLocalContractParser::MaximumPayloadBytes + 1,
                'x'))
    });
    assert(perform(oversized).status ==
           SuiteBridgeHandshakeStatus::DiscoveryPayloadTooLarge);
    assertDiscoveryOnly(oversized);

    FakeTransport wrongPlugin({
        reply(
            900,
            discovery().replace(
                discovery().find("suitebridge"),
                std::string("suitebridge").size(),
                "other"))
    });
    assert(perform(wrongPlugin).status ==
           SuiteBridgeHandshakeStatus::UnexpectedPlugin);
    assertDiscoveryOnly(wrongPlugin);

    const std::string unknownSnapshots =
        "[{\"id\":\"snapshots\",\"state\":\"future\"},"
        "{\"id\":\"local-contract\",\"state\":\"available\"}]";

    FakeTransport unavailableCapability({
        reply(900, discovery(1, 1, 2, 2, unknownSnapshots))
    });
    assert(perform(unavailableCapability).status ==
           SuiteBridgeHandshakeStatus::RequiredCapabilityUnavailable);
    assertDiscoveryOnly(unavailableCapability);
}

void testSnapshotFailures()
{
    FakeTransport timeout({
        reply(900, discovery()),
        transportFailure(
            SuiteBridgeTransportStatus::Timeout,
            "snapshot timeout")
    });
    assert(perform(timeout).status ==
           SuiteBridgeHandshakeStatus::SnapshotTransportError);

    FakeTransport rejected({
        reply(900, discovery()),
        reply(451, "payload unavailable")
    });
    assert(perform(rejected).status ==
           SuiteBridgeHandshakeStatus::SnapshotReplyRejected);

    FakeTransport invalid({
        reply(900, discovery()),
        reply(900, "{\"contract_schema\":2}")
    });
    assert(perform(invalid).status ==
           SuiteBridgeHandshakeStatus::InvalidSnapshotPayload);

    FakeTransport inactive({
        reply(900, discovery()),
        reply(900, snapshot(
            "26f5b0fc557edf7767a4f2ea3a02584d",
            false))
    });
    assert(perform(inactive).status ==
           SuiteBridgeHandshakeStatus::SnapshotInactive);

    FakeTransport badTotal({
        reply(900, discovery()),
        reply(
            900,
            "{\"contract_schema\":2,\"capability_schema\":1,"
            "\"snapshot_schema\":2,\"active\":true,\"total\":5,"
            "\"channel_switch\":4,\"recording\":0,\"replaying\":0,"
            "\"timer_change\":0,"
            "\"counter_epoch\":\"26f5b0fc557edf7767a4f2ea3a02584d\","
            "\"counter_overflow\":false}")
    });
    assert(perform(badTotal).status ==
           SuiteBridgeHandshakeStatus::InvalidSnapshotPayload);

    FakeTransport badEpoch({
        reply(900, discovery()),
        reply(900, snapshot("ABC"))
    });
    assert(perform(badEpoch).status ==
           SuiteBridgeHandshakeStatus::InvalidSnapshotPayload);
}

void testOverflowAndBaselineTracking()
{
    FakeTransport overflowTransport({
        reply(900, discovery()),
        reply(
            900,
            snapshot(
                "26f5b0fc557edf7767a4f2ea3a02584d",
                true,
                true,
                4))
    });

    const SuiteBridgeHandshakeResult overflow =
        perform(overflowTransport);
    assert(overflow.ready());
    assert(overflow.baseline.counterOverflow);

    SuiteBridgeBaselineTracker tracker;
    assert(!tracker.hasBaseline());
    assert(!tracker.deltaAvailable());

    SuiteBridgeSnapshotBaseline first;
    first.active = true;
    first.counterEpoch = "11111111111111111111111111111111";
    first.channelSwitch = 4;
    first.total = 4;

    assert(tracker.apply(first) ==
           SuiteBridgeBaselineUpdate::AdoptedInitial);
    assert(tracker.hasBaseline());
    assert(!tracker.deltaAvailable());

    SuiteBridgeSnapshotBaseline comparable = first;
    comparable.channelSwitch = 8;
    comparable.total = 8;

    assert(tracker.apply(comparable) ==
           SuiteBridgeBaselineUpdate::UpdatedComparable);
    assert(tracker.deltaAvailable());

    SuiteBridgeSnapshotBaseline restarted = comparable;
    restarted.counterEpoch = "22222222222222222222222222222222";
    restarted.channelSwitch = 4;
    restarted.total = 4;

    assert(tracker.apply(restarted) ==
           SuiteBridgeBaselineUpdate::ReplacedEpochChanged);
    assert(!tracker.deltaAvailable());
    assert(tracker.baseline().counterEpoch ==
           "22222222222222222222222222222222");

    SuiteBridgeSnapshotBaseline overflowed = restarted;
    overflowed.counterOverflow = true;

    assert(tracker.apply(overflowed) ==
           SuiteBridgeBaselineUpdate::ReplacedOverflowed);
    assert(!tracker.deltaAvailable());
}

void testStatusVocabulary()
{
    assert(std::string(suiteBridgeHandshakeStatusName(
               SuiteBridgeHandshakeStatus::Ready)) == "ready");
    assert(std::string(suiteBridgeHandshakeStatusName(
               SuiteBridgeHandshakeStatus::LegacyOrUnknown)) ==
           "legacy_or_unknown");
    assert(std::string(suiteBridgeHandshakeStatusName(
               SuiteBridgeHandshakeStatus::InvalidSnapshotPayload)) ==
           "invalid_snapshot_payload");
}

}

int main()
{
    testSuccessfulHandshake();
    testOrderAndAdditiveCapabilities();
    testMutationFailClosed();
    testLegacyAndTransportFailures();
    testSchemaFailuresStopBeforeSnapshot();
    testDiscoveryPayloadFailures();
    testSnapshotFailures();
    testOverflowAndBaselineTracking();
    testStatusVocabulary();

    std::cout << "test_suite_bridge_handshake passed" << std::endl;
    return 0;
}
