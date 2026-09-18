#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BroadcastApplicationProviderEvidence
{
    std::string providerId;
    std::uint32_t providerSchemaVersion = 0;
    std::uint64_t capabilityRevision = 0;
    std::uint64_t observedAt = 0;
};

struct BroadcastApplicationRef
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string channelId;
    BroadcastApplicationProviderEvidence provider;
    std::uint32_t applicationId = 0;
    std::uint64_t descriptorRevision = 0;
};

struct BroadcastApplicationDescriptor
{
    BroadcastApplicationRef ref;
    std::uint8_t controlCode = 0;
    std::uint8_t priority = 0;
    std::string name;
};

struct BroadcastApplicationDiscoverySnapshot
{
    bool payloadValid = false;
    bool receiverActive = false;
    std::string result;
    std::string error;
    std::string channelId;
    std::uint64_t revision = 0;
    std::uint64_t observedAt = 0;
    std::vector<BroadcastApplicationDescriptor> applications;
};
