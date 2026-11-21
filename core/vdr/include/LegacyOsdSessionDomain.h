#pragma once

#include <cstdint>
#include <string>

enum class LegacyOsdSessionMode
{
    ViewOnly
};

enum class LegacyOsdSessionState
{
    Requested,
    Starting,
    Active,
    Degraded,
    ResyncRequired,
    Suspended,
    Closing,
    Closed,
    Expired,
    Failed
};

inline std::string legacyOsdSessionStateName(LegacyOsdSessionState state)
{
    switch (state)
    {
        case LegacyOsdSessionState::Requested: return "requested";
        case LegacyOsdSessionState::Starting: return "starting";
        case LegacyOsdSessionState::Active: return "active";
        case LegacyOsdSessionState::Degraded: return "degraded";
        case LegacyOsdSessionState::ResyncRequired: return "resync_required";
        case LegacyOsdSessionState::Suspended: return "suspended";
        case LegacyOsdSessionState::Closing: return "closing";
        case LegacyOsdSessionState::Closed: return "closed";
        case LegacyOsdSessionState::Expired: return "expired";
        case LegacyOsdSessionState::Failed:
        default: return "failed";
    }
}

struct LegacyOsdCapabilitySnapshot
{
    bool viewAvailable = false;
    bool controlAvailable = false;
};

struct LegacyOsdPolicySnapshot
{
    bool viewAuthorized = false;
    bool controlAuthorized = false;
};

struct LegacyOsdSession
{
    std::string legacyOsdSessionId;
    std::uint64_t sessionRevision = 0;
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    LegacyOsdSessionMode mode = LegacyOsdSessionMode::ViewOnly;
    LegacyOsdSessionState state = LegacyOsdSessionState::Requested;
    std::int64_t createdAt = 0;
    std::int64_t expiresAt = 0;
    std::int64_t idleExpiresAt = 0;
    std::int64_t lastActivityAt = 0;
    LegacyOsdCapabilitySnapshot capabilitySnapshot;
    LegacyOsdPolicySnapshot policySnapshot;
    std::string osdSurfaceId;
    std::string osdEpoch;
    std::string closeReason;
    std::string correlationId;
};
