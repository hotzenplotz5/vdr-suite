#pragma once

#include "LegacyOsdViewerDomain.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class LegacyOsdSessionService;

struct OsdViewerAttachRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
};

struct OsdViewerReadRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
    std::uint64_t acknowledgedFrameSequence = 0;
};

struct OsdViewerDetachRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
};

enum class OsdViewerDeliveryState
{
    Frame,
    NoChange,
    ResyncRequired,
    Suspended
};

inline std::string osdViewerDeliveryStateName(OsdViewerDeliveryState state)
{
    switch (state)
    {
        case OsdViewerDeliveryState::Frame: return "frame";
        case OsdViewerDeliveryState::NoChange: return "no_change";
        case OsdViewerDeliveryState::ResyncRequired: return "resync_required";
        case OsdViewerDeliveryState::Suspended:
        default: return "suspended";
    }
}

struct OsdViewerBindingResult
{
    bool accepted = false;
    std::string error;
    OsdViewerBinding binding;
};

struct OsdViewerDeliveryResult
{
    bool accepted = false;
    std::string error;
    std::string reasonCode;
    OsdViewerDeliveryState state = OsdViewerDeliveryState::Suspended;
    OsdViewerBinding binding;
    bool hasFrame = false;
    OsdFrame frame;
};

class OsdViewerBindingService
{
public:
    static constexpr std::size_t MaximumBindings = 64;
    static constexpr std::size_t MaximumBindingsPerSession = 8;

    using ViewerIdFactory = std::function<std::string()>;
    using NowProvider = std::function<std::int64_t()>;

    explicit OsdViewerBindingService(
        LegacyOsdSessionService& sessionService,
        ViewerIdFactory viewerIdFactory = {},
        NowProvider nowProvider = {});

    OsdViewerBindingResult attach(const OsdViewerAttachRequest& request);
    OsdViewerDeliveryResult read(const OsdViewerReadRequest& request);
    OsdViewerBindingResult detach(const OsdViewerDetachRequest& request);
    std::optional<OsdViewerBinding> find(
        const std::string& viewerBindingId) const;

private:
    OsdViewerBindingResult rejectBinding(const std::string& error) const;
    OsdViewerDeliveryResult rejectDelivery(const std::string& error) const;
    void reapExpired(std::int64_t now);
    bool updateBinding(
        OsdViewerBinding& binding, std::uint64_t expectedRevision);
    void eraseBinding(
        const std::string& viewerBindingId, std::uint64_t expectedRevision);
    static void requireResync(
        OsdViewerBinding& binding, const std::string& reason);

    LegacyOsdSessionService& sessionService_;
    ViewerIdFactory viewerIdFactory_;
    NowProvider nowProvider_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, OsdViewerBinding> bindings_;
};
