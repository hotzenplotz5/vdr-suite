#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BackendAgentChannelFact
{
    std::string channelId;
    std::uint64_t channelNumber = 0;
    std::string name;
    std::string provider;
    std::string groupName;
    bool radio = false;
    bool encrypted = false;
    bool enabled = true;
};

struct BackendAgentChannelSnapshot
{
    std::vector<BackendAgentChannelFact> channels;
    std::string canonicalPayload;
    std::string resourceRevision;
};

bool backendAgentValidChannelFact(
    const BackendAgentChannelFact& fact,
    std::string& reasonCode);

std::string backendAgentCanonicalChannelPayload(
    const std::string& kind,
    const std::vector<BackendAgentChannelFact>& channels,
    const std::vector<BackendAgentChannelFact>& upserts,
    const std::vector<std::string>& removedChannelIds);

std::string backendAgentChannelPayloadIdentity(
    const std::string& canonicalPayload);

bool readBackendAgentChannelsConfSnapshot(
    const std::string& path,
    BackendAgentChannelSnapshot& snapshot,
    std::string& reasonCode);
