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
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    bool payloadValid = false;
    bool receiverActive = false;
    std::string result;
    std::string error;
    std::string channelId;
    std::uint64_t revision = 0;
    std::uint64_t observedAt = 0;
    std::vector<BroadcastApplicationDescriptor> applications;
};

enum class BroadcastApplicationSessionState
{
    Requested,
    Starting,
    Active,
    Degraded,
    Suspended,
    Closing,
    Closed,
    Expired,
    Failed
};

struct BroadcastApplicationRuntimeCapabilityProfile
{
    bool status = true;
    bool close = true;
    std::vector<std::string> inputActions;
};

struct BroadcastApplicationSession
{
    std::string broadcastApplicationSessionId;
    std::string actorId;
    std::string clientContext;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    BroadcastApplicationRef application;
    std::uint64_t applicationDescriptorRevision = 0;
    BroadcastApplicationSessionState state =
        BroadcastApplicationSessionState::Requested;
    std::int64_t createdAt = 0;
    std::int64_t expiresAt = 0;
    BroadcastApplicationRuntimeCapabilityProfile runtimeCapabilityProfile;
    std::string closeReason;
    std::string correlationContext;
};
