#pragma once

#include "LegacyOsdDomain.h"

#include <cstdint>
#include <string>

enum class OsdViewerBindingState
{
    Attached,
    ResyncRequired,
    Suspended,
    Closed
};

inline std::string osdViewerBindingStateName(OsdViewerBindingState state)
{
    switch (state)
    {
        case OsdViewerBindingState::Attached: return "attached";
        case OsdViewerBindingState::ResyncRequired: return "resync_required";
        case OsdViewerBindingState::Suspended: return "suspended";
        case OsdViewerBindingState::Closed:
        default: return "closed";
    }
}

struct OsdViewerBinding
{
    std::string viewerBindingId;
    std::uint64_t bindingRevision = 0;
    std::string actorId;
    std::string clientInstanceId;
    std::string legacyOsdSessionId;
    std::uint64_t sessionRevision = 0;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string osdSurfaceId;
    std::string osdEpoch;
    std::uint64_t lastAcknowledgedFrameSequence = 0;
    std::uint64_t lastDeliveredFrameSequence = 0;
    std::uint64_t lastAcknowledgedEventSequence = 0;
    std::int64_t attachedAt = 0;
    std::int64_t lastSeenAt = 0;
    std::int64_t expiresAt = 0;
    std::string renderingProfile = "semantic-full-frame-v1";
    OsdViewerBindingState state = OsdViewerBindingState::ResyncRequired;
    bool controlAuthorized = false;
    std::string closeReason;
};
