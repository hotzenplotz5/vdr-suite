#include "BackendAgentLiveProviderAuthority.h"

#include <algorithm>

namespace vdrsuite::agent
{
namespace
{

bool currentChannel(
    const std::vector<BackendAgentChannelFact>& channels,
    const std::string& channelId)
{
    const auto match = std::find_if(
        channels.begin(), channels.end(), [&](const BackendAgentChannelFact& channel) {
            return channel.channelId == channelId;
        });
    return match != channels.end() && match->enabled;
}

bool validAgentForFence(
    const BackendAgentRecord& agent,
    const std::string& backendId)
{
    return !agent.agentId.empty() && agent.backendId == backendId &&
        !agent.agentInstanceId.empty() && agent.backendGeneration != 0 &&
        !agent.revoked && !agent.incompatible;
}

bool validCursorForFence(
    const BackendAgentObservationCursor& cursor,
    const BackendAgentRecord& agent,
    const std::string& backendId)
{
    return cursor.present && cursor.backendId == backendId &&
        cursor.observationDomain == BackendAgentLiveProviderAuthority::ChannelObservationDomain &&
        cursor.agentId == agent.agentId &&
        cursor.agentInstanceId == agent.agentInstanceId &&
        cursor.backendGeneration == agent.backendGeneration &&
        cursor.snapshotGeneration != 0 && cursor.producerSequence != 0 &&
        !cursor.resourceRevision.empty();
}

}

BackendAgentLiveProviderPin BackendAgentLiveProviderAuthority::pin(
    const std::string& backendId,
    const std::string& channelId,
    const BackendAgentRecord& agent,
    const BackendAgentObservationCursor& channelCursor,
    const std::vector<BackendAgentChannelFact>& channels,
    const BackendAgentLocalProviderSelection& selection,
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts) const
{
    BackendAgentLiveProviderPin result;
    if (backendId.empty() || channelId.empty()) {
        result.reasonCode = "live_resource_identity_required";
        return result;
    }
    if (!validAgentForFence(agent, backendId)) {
        result.reasonCode = "live_backend_authority_required";
        return result;
    }
    if (!validCursorForFence(channelCursor, agent, backendId)) {
        result.reasonCode = "live_channel_observation_stale";
        return result;
    }
    if (!currentChannel(channels, channelId)) {
        result.reasonCode = "live_channel_not_current";
        return result;
    }
    if (selection.backendId != backendId ||
        selection.authorityDomain != AuthorityDomain ||
        selection.requiredCapability != RequiredCapability ||
        selection.providerKind != RequiredProviderKind) {
        result.reasonCode = "live_provider_selection_required";
        return result;
    }

    std::string providerReason;
    if (!backendAgentLocalProviderSelectionUsable(
            selection, ownership, facts, providerReason)) {
        result.reasonCode = providerReason.empty()
            ? "live_provider_selection_stale"
            : providerReason;
        return result;
    }

    result.providerSelection = selection;
    result.channelFence.backendId = backendId;
    result.channelFence.channelId = channelId;
    result.channelFence.agentId = agent.agentId;
    result.channelFence.agentInstanceId = agent.agentInstanceId;
    result.channelFence.backendGeneration = agent.backendGeneration;
    result.channelFence.snapshotGeneration = channelCursor.snapshotGeneration;
    result.channelFence.producerSequence = channelCursor.producerSequence;
    result.channelFence.channelResourceRevision = channelCursor.resourceRevision;
    result.valid = true;
    result.reasonCode = "live_provider_pinned";
    return result;
}

bool BackendAgentLiveProviderAuthority::usable(
    const BackendAgentLiveProviderPin& pinValue,
    const BackendAgentRecord& agent,
    const BackendAgentObservationCursor& channelCursor,
    const std::vector<BackendAgentChannelFact>& channels,
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode) const
{
    if (!pinValue.valid) {
        reasonCode = "live_provider_pin_required";
        return false;
    }
    const auto& fence = pinValue.channelFence;
    if (!validAgentForFence(agent, fence.backendId) ||
        agent.agentId != fence.agentId ||
        agent.agentInstanceId != fence.agentInstanceId ||
        agent.backendGeneration != fence.backendGeneration) {
        reasonCode = "live_backend_generation_stale";
        return false;
    }
    if (!validCursorForFence(channelCursor, agent, fence.backendId) ||
        channelCursor.snapshotGeneration != fence.snapshotGeneration ||
        channelCursor.producerSequence != fence.producerSequence ||
        channelCursor.resourceRevision != fence.channelResourceRevision) {
        reasonCode = "live_channel_observation_stale";
        return false;
    }
    if (!currentChannel(channels, fence.channelId)) {
        reasonCode = "live_channel_not_current";
        return false;
    }
    if (!backendAgentLocalProviderSelectionUsable(
            pinValue.providerSelection, ownership, facts, reasonCode)) {
        if (reasonCode.empty()) reasonCode = "live_provider_selection_stale";
        return false;
    }
    reasonCode = "live_provider_pin_current";
    return true;
}

}
