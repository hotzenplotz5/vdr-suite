#include "BackendAgentLiveProviderAuthority.h"

#include <cassert>
#include <string>
#include <vector>

using namespace vdrsuite::agent;

namespace
{
BackendAgentRecord agent()
{
    BackendAgentRecord value;
    value.agentId = "agt_live";
    value.backendId = "default";
    value.agentInstanceId = "inst_live";
    value.backendGeneration = 7;
    value.capabilityRevision = 3;
    return value;
}

BackendAgentObservationCursor cursor()
{
    BackendAgentObservationCursor value;
    value.present = true;
    value.backendId = "default";
    value.observationDomain = "channels";
    value.agentId = "agt_live";
    value.agentInstanceId = "inst_live";
    value.backendGeneration = 7;
    value.snapshotGeneration = 9;
    value.producerSequence = 11;
    value.resourceRevision = "chanrev_11";
    return value;
}

std::vector<BackendAgentChannelFact> channels()
{
    BackendAgentChannelFact channel;
    channel.channelId = "S19.2E-1-1019-10301";
    channel.channelNumber = 1;
    channel.name = "Channel A";
    channel.enabled = true;
    return {channel};
}

BackendAgentLocalProviderFacts facts()
{
    BackendAgentLocalProviderFacts value;
    value.providerId = "suitebridge:local";
    value.providerKind = "suitebridge";
    value.providerInstanceEpoch = "pie_live_1";
    value.providerGeneration = 4;
    value.capabilityRevision = 5;
    value.available = true;
    value.capabilities = {"vdr.native.probe", "vdr.live.stream"};
    return value;
}

BackendAgentLocalProviderOwnership ownership()
{
    BackendAgentLocalProviderOwnership value;
    value.backendId = "default";
    value.authorityDomain = "vdr.live";
    value.providerId = "suitebridge:local";
    value.providerKind = "suitebridge";
    value.ownershipGeneration = 2;
    value.allowedCapabilities = {"vdr.live.stream"};
    return value;
}

BackendAgentLocalProviderSelection selection()
{
    std::string reason;
    const auto value = backendAgentLocalProviderSelect(
        ownership(), facts(), "vdr.live.stream", reason);
    assert(reason == "local_provider_selected");
    assert(backendAgentLocalProviderValidSelection(value));
    return value;
}
}

int main()
{
    BackendAgentLiveProviderAuthority authority;
    const auto baseAgent = agent();
    const auto baseCursor = cursor();
    const auto baseChannels = channels();
    const auto baseFacts = facts();
    const auto baseOwnership = ownership();
    const auto baseSelection = selection();

    const auto pin = authority.pin(
        "default", "S19.2E-1-1019-10301", baseAgent, baseCursor,
        baseChannels, baseSelection, baseOwnership, baseFacts);
    assert(pin.valid);
    assert(pin.reasonCode == "live_provider_pinned");
    assert(pin.channelFence.backendId == "default");
    assert(pin.channelFence.channelId == "S19.2E-1-1019-10301");
    assert(pin.providerSelection.providerId == "suitebridge:local");

    std::string reason;
    assert(authority.usable(
        pin, baseAgent, baseCursor, baseChannels, baseOwnership, baseFacts, reason));
    assert(reason == "live_provider_pin_current");

    auto staleBackend = baseAgent;
    ++staleBackend.backendGeneration;
    assert(!authority.usable(
        pin, staleBackend, baseCursor, baseChannels, baseOwnership, baseFacts, reason));
    assert(reason == "live_backend_generation_stale");

    auto staleEpoch = baseFacts;
    staleEpoch.providerInstanceEpoch = "pie_live_2";
    assert(!authority.usable(
        pin, baseAgent, baseCursor, baseChannels, baseOwnership, staleEpoch, reason));
    assert(reason == "local_provider_instance_epoch_changed");

    auto staleGeneration = baseFacts;
    ++staleGeneration.providerGeneration;
    assert(!authority.usable(
        pin, baseAgent, baseCursor, baseChannels, baseOwnership, staleGeneration, reason));
    assert(reason == "local_provider_generation_changed");

    auto staleCapability = baseFacts;
    ++staleCapability.capabilityRevision;
    assert(!authority.usable(
        pin, baseAgent, baseCursor, baseChannels, baseOwnership, staleCapability, reason));
    assert(reason == "local_provider_capability_revision_changed");

    auto staleObservation = baseCursor;
    staleObservation.resourceRevision = "chanrev_12";
    assert(!authority.usable(
        pin, baseAgent, staleObservation, baseChannels, baseOwnership, baseFacts, reason));
    assert(reason == "live_channel_observation_stale");

    auto revokedOwnership = baseOwnership;
    ++revokedOwnership.ownershipGeneration;
    assert(!authority.usable(
        pin, baseAgent, baseCursor, baseChannels, revokedOwnership, baseFacts, reason));
    assert(reason == "local_provider_ownership_changed");

    auto unavailableFacts = baseFacts;
    unavailableFacts.available = false;
    assert(!authority.usable(
        pin, baseAgent, baseCursor, baseChannels, baseOwnership, unavailableFacts, reason));
    assert(reason == "local_provider_unavailable");

    auto alternateFacts = baseFacts;
    alternateFacts.providerId = "restfulapi:local";
    alternateFacts.providerKind = "restfulapi";
    alternateFacts.providerInstanceEpoch = "rest_live_1";
    assert(!authority.usable(
        pin, baseAgent, baseCursor, baseChannels, baseOwnership, alternateFacts, reason));
    assert(reason == "local_provider_identity_changed");

    auto missingChannel = baseChannels;
    missingChannel.front().channelId = "S19.2E-1-1019-99999";
    assert(!authority.usable(
        pin, baseAgent, baseCursor, missingChannel, baseOwnership, baseFacts, reason));
    assert(reason == "live_channel_not_current");

    auto disabledChannel = baseChannels;
    disabledChannel.front().enabled = false;
    const auto disabledPin = authority.pin(
        "default", "S19.2E-1-1019-10301", baseAgent, baseCursor,
        disabledChannel, baseSelection, baseOwnership, baseFacts);
    assert(!disabledPin.valid);
    assert(disabledPin.reasonCode == "live_channel_not_current");

    auto wrongProviderSelection = baseSelection;
    wrongProviderSelection.providerKind = "restfulapi";
    const auto noFallback = authority.pin(
        "default", "S19.2E-1-1019-10301", baseAgent, baseCursor,
        baseChannels, wrongProviderSelection, baseOwnership, baseFacts);
    assert(!noFallback.valid);
    assert(noFallback.reasonCode == "live_provider_selection_required");

    return 0;
}
