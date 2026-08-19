#pragma once

#include "BackendAgentLifecycle.h"
#include "BackendAgentLocalProvider.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

struct BackendAgentLiveChannelFence
{
    std::string backendId;
    std::string channelId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::uint64_t snapshotGeneration = 0;
    std::uint64_t producerSequence = 0;
    std::string channelResourceRevision;
};

struct BackendAgentLiveProviderPin
{
    bool valid = false;
    std::string reasonCode;
    BackendAgentLocalProviderSelection providerSelection;
    BackendAgentLiveChannelFence channelFence;
};

class BackendAgentLiveProviderAuthority
{
public:
    static constexpr const char* AuthorityDomain = "vdr.live";
    static constexpr const char* RequiredCapability = "vdr.live.stream";
    static constexpr const char* RequiredProviderKind = "suitebridge";
    static constexpr const char* ChannelObservationDomain = "channels";

    BackendAgentLiveProviderPin pin(
        const std::string& backendId,
        const std::string& channelId,
        const BackendAgentRecord& agent,
        const BackendAgentObservationCursor& channelCursor,
        const std::vector<BackendAgentChannelFact>& channels,
        const BackendAgentLocalProviderSelection& selection,
        const BackendAgentLocalProviderOwnership& ownership,
        const BackendAgentLocalProviderFacts& facts) const;

    bool usable(
        const BackendAgentLiveProviderPin& pin,
        const BackendAgentRecord& agent,
        const BackendAgentObservationCursor& channelCursor,
        const std::vector<BackendAgentChannelFact>& channels,
        const BackendAgentLocalProviderOwnership& ownership,
        const BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) const;
};

}
