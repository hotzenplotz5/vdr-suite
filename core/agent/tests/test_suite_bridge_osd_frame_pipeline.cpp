#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeHandshake.h"
#include "SuiteBridgeOsdFrameBuffer.h"
#include "SuiteBridgeOsdFrameParser.h"
#include "SuiteBridgeOsdFrameSource.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace vdrsuite::agent;

namespace
{

std::string menuPayload(
    const std::string& epoch,
    std::uint64_t sequence,
    bool complete = true,
    bool consistent = true,
    std::uint64_t dropped = 0,
    const std::string& title = "Main menu")
{
    return
        "{\"osd_schema\":1"
        ",\"active\":true"
        ",\"complete\":" + std::string(complete ? "true" : "false") +
        ",\"consistent\":" + std::string(consistent ? "true" : "false") +
        ",\"kind\":\"menu\""
        ",\"frame_sequence\":" + std::to_string(sequence) +
        ",\"observed_at_ms\":123456"
        ",\"dropped_updates\":" + std::to_string(dropped) +
        ",\"osd_epoch\":\"" + epoch + "\""
        ",\"title\":\"" + title + "\""
        ",\"status\":\"Ready\""
        ",\"red\":\"Red\""
        ",\"green\":\"Green\""
        ",\"yellow\":\"Yellow\""
        ",\"blue\":\"Blue\""
        ",\"items\":["
            "{\"text\":\"Recordings\",\"selectable\":true},"
            "{\"text\":\"Setup\",\"selectable\":true}"
        "]"
        ",\"selected_index\":0"
        ",\"text\":\"\""
        ",\"channel\":\"\""
        ",\"programme\":{"
            "\"present_time\":0,"
            "\"present_title\":\"\","
            "\"present_subtitle\":\"\","
            "\"following_time\":0,"
            "\"following_title\":\"\","
            "\"following_subtitle\":\"\""
        "}}";
}

std::string inactivePayload(std::uint64_t sequence)
{
    return
        "{\"osd_schema\":1"
        ",\"active\":false"
        ",\"complete\":true"
        ",\"consistent\":true"
        ",\"kind\":\"inactive\""
        ",\"frame_sequence\":" + std::to_string(sequence) +
        ",\"observed_at_ms\":123456"
        ",\"dropped_updates\":0"
        ",\"osd_epoch\":\"\""
        ",\"title\":\"\""
        ",\"status\":\"\""
        ",\"red\":\"\""
        ",\"green\":\"\""
        ",\"yellow\":\"\""
        ",\"blue\":\"\""
        ",\"items\":[]"
        ",\"selected_index\":-1"
        ",\"text\":\"\""
        ",\"channel\":\"\""
        ",\"programme\":{"
            "\"present_time\":0,"
            "\"present_title\":\"\","
            "\"present_subtitle\":\"\","
            "\"following_time\":0,"
            "\"following_title\":\"\","
            "\"following_subtitle\":\"\""
        "}}";
}

SuiteBridgeOsdObservedFrame parsed(
    const std::string& payload,
    std::uint64_t generation = 7)
{
    SuiteBridgeOsdFrameParser parser;
    SuiteBridgeOsdFrameParseResult result =
        parser.parse(payload, "default", generation);
    assert(result.ok());
    return result.value;
}

SuiteBridgeDiscovery discovery(bool osdView)
{
    SuiteBridgeDiscovery value;
    value.discoverySchema = 1;
    value.pluginName = "suitebridge";
    value.pluginVersion = "0.13.5";
    value.capabilitySchema = 1;
    value.snapshotSchema = 3;
    value.localContractSchema = 3;

    SuiteBridgeCapabilityObservation capability;
    capability.id = "osd.view";
    capability.state = osdView
        ? SuiteBridgeCapabilityState::Available
        : SuiteBridgeCapabilityState::Disabled;
    capability.rawState = osdView ? "available" : "disabled";
    value.capabilities.push_back(std::move(capability));
    return value;
}

SuiteBridgeCommandReply success(std::string payload)
{
    SuiteBridgeCommandReply reply;
    reply.transportStatus = SuiteBridgeTransportStatus::Success;
    reply.replyCode = 900;
    reply.payload = std::move(payload);
    return reply;
}

class FakeTransport final : public ISuiteBridgeLocalTransport
{
public:
    explicit FakeTransport(std::vector<SuiteBridgeCommandReply> replies)
        : replies_(std::move(replies))
    {
    }

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override
    {
        commands.push_back(command);
        if (next_ >= replies_.size())
        {
            SuiteBridgeCommandReply reply;
            reply.transportStatus = SuiteBridgeTransportStatus::Failed;
            reply.diagnostic = "fixture exhausted";
            return reply;
        }
        return replies_[next_++];
    }

    std::vector<SuiteBridgeLocalCommand> commands;

private:
    std::vector<SuiteBridgeCommandReply> replies_;
    std::size_t next_ = 0;
};

void testParser()
{
    SuiteBridgeOsdFrameParser parser;
    const std::string epoch = "0123456789abcdef0123456789abcdef";

    SuiteBridgeOsdFrameParseResult current =
        parser.parse(menuPayload(epoch, 42), "backend-a", 9);
    assert(current.ok());
    assert(current.value.sourceConsistent);
    assert(current.value.droppedUpdates == 0);
    assert(current.value.frame.surface.backendId == "backend-a");
    assert(current.value.frame.surface.backendGeneration == 9);
    assert(current.value.frame.surface.surfaceId == "primary-native-osd");
    assert(current.value.frame.surface.osdEpoch == epoch);
    assert(current.value.frame.state == OsdSurfaceState::Active);
    assert(current.value.frame.kind == OsdFrameKind::Menu);
    assert(current.value.frame.frameSequence == 42);
    assert(current.value.frame.fullFrame);
    assert(current.value.frame.complete);
    assert(current.value.frame.items.size() == 2);
    assert(current.value.frame.items.at(0).text == "Recordings");
    assert(current.value.frame.selectedIndex == 0);

    SuiteBridgeOsdFrameParseResult degraded =
        parser.parse(menuPayload(epoch, 43, false, true, 1), "backend-a", 9);
    assert(degraded.ok());
    assert(degraded.value.frame.state == OsdSurfaceState::Degraded);
    assert(!degraded.value.frame.complete);
    assert(degraded.value.droppedUpdates == 1);

    SuiteBridgeOsdFrameParseResult inactive =
        parser.parse(inactivePayload(44), "backend-a", 9);
    assert(inactive.ok());
    assert(inactive.value.frame.state == OsdSurfaceState::Inactive);
    assert(inactive.value.frame.kind == OsdFrameKind::None);
    assert(inactive.value.frame.surface.osdEpoch.empty());

    std::string wrongSchema = menuPayload(epoch, 42);
    wrongSchema.replace(
        wrongSchema.find("\"osd_schema\":1"),
        std::string("\"osd_schema\":1").size(),
        "\"osd_schema\":2");
    assert(
        parser.parse(wrongSchema, "backend-a", 9).status ==
        SuiteBridgeOsdFrameParseStatus::UnsupportedSchema);

    std::string badEpoch = menuPayload("not-an-epoch", 42);
    assert(
        parser.parse(badEpoch, "backend-a", 9).status ==
        SuiteBridgeOsdFrameParseStatus::InvalidFrame);

    std::string badSelection = menuPayload(epoch, 42);
    badSelection.replace(
        badSelection.find("\"selected_index\":0"),
        std::string("\"selected_index\":0").size(),
        "\"selected_index\":99");
    assert(
        parser.parse(badSelection, "backend-a", 9).status ==
        SuiteBridgeOsdFrameParseStatus::InvalidFrame);

    const std::string oversized(
        SuiteBridgeOsdFrameParser::MaximumPayloadBytes + 1,
        'x');
    assert(
        parser.parse(oversized, "backend-a", 9).status ==
        SuiteBridgeOsdFrameParseStatus::PayloadTooLarge);
}

void testBufferContinuity()
{
    const std::string epochA = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string epochB = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    const std::string epochC = "cccccccccccccccccccccccccccccccc";

    SuiteBridgeOsdFrameBuffer buffer;

    assert(
        buffer.apply(parsed(menuPayload(epochA, 10))) ==
        SuiteBridgeOsdFrameBufferApplyResult::Adopted);
    assert(buffer.snapshot().hasFrame);
    assert(!buffer.snapshot().resyncRequired);

    assert(
        buffer.apply(parsed(menuPayload(epochA, 12))) ==
        SuiteBridgeOsdFrameBufferApplyResult::Updated);
    assert(buffer.snapshot().observed.frame.frameSequence == 12);

    assert(
        buffer.apply(parsed(menuPayload(epochA, 12))) ==
        SuiteBridgeOsdFrameBufferApplyResult::Duplicate);

    assert(
        buffer.apply(parsed(menuPayload(epochA, 13, false, true, 1))) ==
        SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired);
    assert(buffer.snapshot().resyncRequired);

    assert(
        buffer.apply(parsed(menuPayload(epochA, 14, true, true, 1))) ==
        SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired);
    assert(buffer.snapshot().resyncRequired);

    assert(
        buffer.apply(parsed(menuPayload(epochB, 1, true, true, 1))) ==
        SuiteBridgeOsdFrameBufferApplyResult::ReplacedEpoch);
    assert(!buffer.snapshot().resyncRequired);
    assert(buffer.snapshot().observed.frame.surface.osdEpoch == epochB);

    assert(
        buffer.apply(parsed(menuPayload(epochB, 0, true, true, 1))) ==
        SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired);
    assert(buffer.snapshot().resyncRequired);

    assert(
        buffer.apply(parsed(inactivePayload(2))) ==
        SuiteBridgeOsdFrameBufferApplyResult::BecameInactive);
    assert(!buffer.snapshot().resyncRequired);
    assert(
        buffer.snapshot().observed.frame.state ==
        OsdSurfaceState::Inactive);

    assert(
        buffer.apply(parsed(menuPayload(epochC, 1))) ==
        SuiteBridgeOsdFrameBufferApplyResult::ReplacedEpoch);
    assert(!buffer.snapshot().resyncRequired);

    assert(
        buffer.apply(parsed(menuPayload(epochC, 1), 8)) ==
        SuiteBridgeOsdFrameBufferApplyResult::ReplacedBackendGeneration);
    assert(
        buffer.snapshot().observed.frame.surface.backendGeneration == 8);
}

void testCapabilityGatedSource()
{
    const std::string epochA = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string epochB = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

    FakeTransport transport({
        success(menuPayload(epochA, 10)),
        success(menuPayload(epochA, 11, false, true, 1)),
        success(menuPayload(epochB, 1, true, true, 1)),
    });

    SuiteBridgeOsdFrameSource source(
        transport,
        "default",
        7);

    SuiteBridgeOsdFrameSourceSnapshot current =
        source.read(discovery(true));
    assert(current.state == SuiteBridgeOsdFrameSourceState::Current);
    assert(current.buffer.hasFrame);
    assert(!current.buffer.resyncRequired);

    SuiteBridgeOsdFrameSourceSnapshot degraded =
        source.read(discovery(true));
    assert(
        degraded.state ==
        SuiteBridgeOsdFrameSourceState::ResyncRequired);
    assert(degraded.buffer.resyncRequired);

    SuiteBridgeOsdFrameSourceSnapshot replaced =
        source.read(discovery(true));
    assert(replaced.state == SuiteBridgeOsdFrameSourceState::Current);
    assert(!replaced.buffer.resyncRequired);
    assert(replaced.buffer.observed.frame.surface.osdEpoch == epochB);

    assert(transport.commands.size() == 3);
    for (const SuiteBridgeLocalCommand command : transport.commands)
    {
        assert(command == SuiteBridgeLocalCommand::OsdSnapshot);
    }

    FakeTransport disabledTransport({});
    SuiteBridgeOsdFrameSource disabled(
        disabledTransport,
        "default",
        7);
    const SuiteBridgeOsdFrameSourceSnapshot unavailable =
        disabled.read(discovery(false));
    assert(
        unavailable.state ==
        SuiteBridgeOsdFrameSourceState::CapabilityUnavailable);
    assert(disabledTransport.commands.empty());

    SuiteBridgeCommandReply failed;
    failed.transportStatus = SuiteBridgeTransportStatus::Timeout;
    failed.diagnostic = "timeout";
    FakeTransport failingTransport({failed});
    SuiteBridgeOsdFrameSource failing(
        failingTransport,
        "default",
        7);
    const SuiteBridgeOsdFrameSourceSnapshot transportFailure =
        failing.read(discovery(true));
    assert(
        transportFailure.state ==
        SuiteBridgeOsdFrameSourceState::TransportDegraded);
    assert(transportFailure.diagnostic == "timeout");
}

}

int main()
{
    testParser();
    testBufferContinuity();
    testCapabilityGatedSource();

    std::cout
        << "test_suite_bridge_osd_frame_pipeline passed"
        << std::endl;
    return 0;
}
