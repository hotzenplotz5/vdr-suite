#pragma once

#include <cstdint>
#include <string>

enum class OsdControllerLeaseState
{
    Requested,
    Active,
    Expiring,
    Revoked,
    Expired,
    Released
};

inline std::string osdControllerLeaseStateName(
    OsdControllerLeaseState state)
{
    switch (state)
    {
        case OsdControllerLeaseState::Requested: return "requested";
        case OsdControllerLeaseState::Active: return "active";
        case OsdControllerLeaseState::Expiring: return "expiring";
        case OsdControllerLeaseState::Revoked: return "revoked";
        case OsdControllerLeaseState::Expired: return "expired";
        case OsdControllerLeaseState::Released:
        default: return "released";
    }
}

struct OsdControllerLease
{
    std::string controllerLeaseId;
    std::uint64_t leaseRevision = 0;
    std::uint64_t controllerLeaseEpoch = 0;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string osdSurfaceId;
    std::string osdEpoch;
    std::int64_t grantedAt = 0;
    std::int64_t expiresAt = 0;
    std::int64_t renewAfter = 0;
    std::int64_t lastHeartbeatAt = 0;
    OsdControllerLeaseState state = OsdControllerLeaseState::Requested;
    std::string revocationReason;
};
