#include "OsdViewerBindingService.h"

#include "LegacyOsdSessionService.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <sys/random.h>
#include <utility>

namespace
{
constexpr std::size_t ViewerEntropyBytes = 16;

bool safeIdentifier(const std::string& value, bool allowColon,
                    std::size_t maximumLength = 128)
{
    return !value.empty() && value.size() <= maximumLength &&
        std::all_of(value.begin(), value.end(),
            [allowColon](unsigned char c) {
                return std::isalnum(c) != 0 || c == '-' || c == '_' ||
                    c == '.' || (allowColon && c == ':');
            });
}

std::string systemViewerId()
{
    std::array<unsigned char, ViewerEntropyBytes> bytes{};
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        const ssize_t received = getrandom(
            bytes.data() + offset, bytes.size() - offset, 0);
        if (received <= 0) return {};
        offset += static_cast<std::size_t>(received);
    }

    std::ostringstream output;
    output << "ovb_";
    for (unsigned char byte : bytes)
        output << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(byte);
    return output.str();
}

std::int64_t systemNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool sessionViewable(const LegacyOsdSession& session)
{
    return session.state == LegacyOsdSessionState::Active ||
        session.state == LegacyOsdSessionState::ResyncRequired;
}

bool sameSurface(
    const OsdViewerBinding& binding, const OsdSurfaceRef& surface)
{
    return binding.backendId == surface.backendId &&
        binding.backendGeneration == surface.backendGeneration &&
        binding.osdSurfaceId == surface.surfaceId &&
        binding.osdEpoch == surface.osdEpoch;
}
}

OsdViewerBindingService::OsdViewerBindingService(
    LegacyOsdSessionService& sessionService,
    ViewerIdFactory viewerIdFactory,
    NowProvider nowProvider)
    : sessionService_(sessionService),
      viewerIdFactory_(viewerIdFactory ? std::move(viewerIdFactory)
                                      : ViewerIdFactory(systemViewerId)),
      nowProvider_(nowProvider ? std::move(nowProvider)
                              : NowProvider(systemNow))
{
}

OsdViewerBindingResult OsdViewerBindingService::rejectBinding(
    const std::string& error) const
{
    OsdViewerBindingResult result;
    result.error = error;
    return result;
}

OsdViewerDeliveryResult OsdViewerBindingService::rejectDelivery(
    const std::string& error) const
{
    OsdViewerDeliveryResult result;
    result.error = error;
    return result;
}

void OsdViewerBindingService::reapExpired(std::int64_t now)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = bindings_.begin(); it != bindings_.end();)
    {
        if (now >= it->second.expiresAt) it = bindings_.erase(it);
        else ++it;
    }
}

std::optional<OsdViewerBinding> OsdViewerBindingService::find(
    const std::string& viewerBindingId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = bindings_.find(viewerBindingId);
    if (found == bindings_.end()) return std::nullopt;
    return found->second;
}

bool OsdViewerBindingService::updateBinding(
    OsdViewerBinding& binding, std::uint64_t expectedRevision)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = bindings_.find(binding.viewerBindingId);
    if (found == bindings_.end() ||
        found->second.bindingRevision != expectedRevision)
        return false;
    binding.bindingRevision = expectedRevision + 1;
    found->second = binding;
    return true;
}

void OsdViewerBindingService::eraseBinding(
    const std::string& viewerBindingId, std::uint64_t expectedRevision)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = bindings_.find(viewerBindingId);
    if (found != bindings_.end() &&
        found->second.bindingRevision == expectedRevision)
        bindings_.erase(found);
}

void OsdViewerBindingService::requireResync(
    OsdViewerBinding& binding, const std::string& reason)
{
    binding.state = OsdViewerBindingState::ResyncRequired;
    binding.lastAcknowledgedFrameSequence = 0;
    binding.lastDeliveredFrameSequence = 0;
    binding.lastAcknowledgedEventSequence = 0;
    binding.closeReason = reason;
}

OsdViewerBindingResult OsdViewerBindingService::attach(
    const OsdViewerAttachRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false))
        return rejectBinding("legacy_osd_viewer_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return rejectBinding("legacy_osd_time_unavailable");
    reapExpired(now);

    const LegacyOsdSessionResult sessionResult = sessionService_.status(
        request.legacyOsdSessionId, request.actorId,
        request.clientInstanceId, request.backendId);
    if (!sessionResult.accepted)
        return rejectBinding(sessionResult.error.empty()
            ? "legacy_osd_viewer_session_invalid" : sessionResult.error);

    const LegacyOsdSession& session = sessionResult.session;
    if (!sessionViewable(session) || session.backendGeneration == 0 ||
        session.osdSurfaceId.empty() || session.osdEpoch.empty() ||
        now >= session.expiresAt)
        return rejectBinding("legacy_osd_viewer_session_unavailable");

    const std::string viewerId =
        viewerIdFactory_ ? viewerIdFactory_() : std::string{};
    if (!safeIdentifier(viewerId, false))
        return rejectBinding("legacy_osd_viewer_id_unavailable");

    OsdViewerBinding binding;
    binding.viewerBindingId = viewerId;
    binding.bindingRevision = 1;
    binding.actorId = request.actorId;
    binding.clientInstanceId = request.clientInstanceId;
    binding.legacyOsdSessionId = request.legacyOsdSessionId;
    binding.sessionRevision = session.sessionRevision;
    binding.backendId = session.backendId;
    binding.backendGeneration = session.backendGeneration;
    binding.osdSurfaceId = session.osdSurfaceId;
    binding.osdEpoch = session.osdEpoch;
    binding.attachedAt = now;
    binding.lastSeenAt = now;
    binding.expiresAt = session.expiresAt;
    binding.state = OsdViewerBindingState::ResyncRequired;
    binding.controlAuthorized = false;
    binding.closeReason = "initial_full_frame_required";

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (bindings_.size() >= MaximumBindings)
            return rejectBinding("legacy_osd_viewer_capacity_reached");
        std::size_t sessionBindings = 0;
        for (const auto& item : bindings_)
        {
            if (item.second.legacyOsdSessionId ==
                request.legacyOsdSessionId)
                ++sessionBindings;
        }
        if (sessionBindings >= MaximumBindingsPerSession)
            return rejectBinding(
                "legacy_osd_viewer_session_capacity_reached");
        if (bindings_.count(viewerId) != 0U)
            return rejectBinding("legacy_osd_viewer_id_collision");
        bindings_.emplace(viewerId, binding);
    }

    OsdViewerBindingResult result;
    result.accepted = true;
    result.binding = binding;
    return result;
}

OsdViewerDeliveryResult OsdViewerBindingService::read(
    const OsdViewerReadRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false) ||
        !safeIdentifier(request.viewerBindingId, false))
        return rejectDelivery("legacy_osd_viewer_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return rejectDelivery("legacy_osd_time_unavailable");
    reapExpired(now);

    const auto stored = find(request.viewerBindingId);
    if (!stored.has_value() ||
        stored->actorId != request.actorId ||
        stored->clientInstanceId != request.clientInstanceId ||
        stored->backendId != request.backendId ||
        stored->legacyOsdSessionId != request.legacyOsdSessionId)
        return rejectDelivery("legacy_osd_viewer_not_found");

    OsdViewerBinding binding = *stored;
    const std::uint64_t expectedRevision = binding.bindingRevision;

    const LegacyOsdSessionResult sessionResult = sessionService_.status(
        binding.legacyOsdSessionId, binding.actorId,
        binding.clientInstanceId, binding.backendId);
    if (!sessionResult.accepted)
    {
        eraseBinding(binding.viewerBindingId, expectedRevision);
        return rejectDelivery(sessionResult.error.empty()
            ? "legacy_osd_viewer_session_invalid" : sessionResult.error);
    }

    const LegacyOsdSession& session = sessionResult.session;
    binding.sessionRevision = session.sessionRevision;
    binding.lastSeenAt = now;
    binding.expiresAt = std::min(binding.expiresAt, session.expiresAt);

    if (session.backendGeneration != binding.backendGeneration)
    {
        eraseBinding(binding.viewerBindingId, expectedRevision);
        return rejectDelivery("legacy_osd_backend_generation_changed");
    }

    if (session.state == LegacyOsdSessionState::Suspended)
    {
        binding.state = OsdViewerBindingState::Suspended;
        binding.closeReason = session.closeReason.empty()
            ? "osd_suspended" : session.closeReason;
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::Suspended;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    if (!sessionViewable(session) ||
        session.osdSurfaceId.empty() || session.osdEpoch.empty())
    {
        eraseBinding(binding.viewerBindingId, expectedRevision);
        return rejectDelivery("legacy_osd_viewer_session_unavailable");
    }

    if (binding.osdSurfaceId != session.osdSurfaceId ||
        binding.osdEpoch != session.osdEpoch)
    {
        binding.osdSurfaceId = session.osdSurfaceId;
        binding.osdEpoch = session.osdEpoch;
        requireResync(binding, "osd_surface_or_epoch_changed");
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::ResyncRequired;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    if (session.state == LegacyOsdSessionState::ResyncRequired)
    {
        requireResync(binding, "osd_resync_required");
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::ResyncRequired;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    if (binding.state == OsdViewerBindingState::Suspended)
    {
        requireResync(binding, "viewer_resume_resync_required");
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::ResyncRequired;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    const BackendAgentOsdReadResult& observation =
        sessionResult.observation;
    if (!observation.available ||
        !observation.snapshot.buffer.hasFrame)
    {
        binding.state = OsdViewerBindingState::Suspended;
        binding.closeReason = observation.reasonCode.empty()
            ? "osd_unavailable" : observation.reasonCode;
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::Suspended;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    const OsdFrame& frame = observation.snapshot.buffer.observed.frame;
    if (!frame.fullFrame || !frame.complete || frame.frameSequence == 0 ||
        !osdFrameWithinLimits(frame) || !sameSurface(binding, frame.surface))
    {
        requireResync(binding, "viewer_frame_inconsistent");
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::ResyncRequired;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    if (request.acknowledgedFrameSequence !=
            binding.lastAcknowledgedFrameSequence &&
        request.acknowledgedFrameSequence !=
            binding.lastDeliveredFrameSequence)
    {
        requireResync(binding, "viewer_ack_conflict");
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::ResyncRequired;
        result.reasonCode = binding.closeReason;
        result.binding = binding;
        return result;
    }

    if (binding.lastDeliveredFrameSequence != 0 &&
        request.acknowledgedFrameSequence ==
            binding.lastDeliveredFrameSequence)
        binding.lastAcknowledgedFrameSequence =
            binding.lastDeliveredFrameSequence;

    if (binding.lastDeliveredFrameSequence !=
        binding.lastAcknowledgedFrameSequence)
    {
        if (frame.frameSequence != binding.lastDeliveredFrameSequence)
        {
            requireResync(binding,
                "viewer_backpressure_resync_required");
            if (!updateBinding(binding, expectedRevision))
                return rejectDelivery("legacy_osd_viewer_concurrent_update");

            OsdViewerDeliveryResult result;
            result.accepted = true;
            result.state = OsdViewerDeliveryState::ResyncRequired;
            result.reasonCode = binding.closeReason;
            result.binding = binding;
            return result;
        }

        binding.state = OsdViewerBindingState::Attached;
        binding.closeReason.clear();
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::Frame;
        result.binding = binding;
        result.hasFrame = true;
        result.frame = frame;
        return result;
    }

    bool deliver = binding.lastAcknowledgedFrameSequence == 0 ||
        binding.state == OsdViewerBindingState::ResyncRequired;
    if (!deliver)
    {
        if (frame.frameSequence == binding.lastAcknowledgedFrameSequence)
        {
            binding.state = OsdViewerBindingState::Attached;
            binding.closeReason.clear();
            if (!updateBinding(binding, expectedRevision))
                return rejectDelivery("legacy_osd_viewer_concurrent_update");

            OsdViewerDeliveryResult result;
            result.accepted = true;
            result.state = OsdViewerDeliveryState::NoChange;
            result.binding = binding;
            return result;
        }

        const bool exactNext =
            binding.lastAcknowledgedFrameSequence !=
                std::numeric_limits<std::uint64_t>::max() &&
            frame.frameSequence ==
                binding.lastAcknowledgedFrameSequence + 1;
        if (!exactNext)
        {
            requireResync(binding, "viewer_sequence_gap");
            if (!updateBinding(binding, expectedRevision))
                return rejectDelivery("legacy_osd_viewer_concurrent_update");

            OsdViewerDeliveryResult result;
            result.accepted = true;
            result.state = OsdViewerDeliveryState::ResyncRequired;
            result.reasonCode = binding.closeReason;
            result.binding = binding;
            return result;
        }
        deliver = true;
    }

    if (deliver)
    {
        binding.state = OsdViewerBindingState::Attached;
        binding.closeReason.clear();
        binding.lastDeliveredFrameSequence = frame.frameSequence;
        if (!updateBinding(binding, expectedRevision))
            return rejectDelivery("legacy_osd_viewer_concurrent_update");

        OsdViewerDeliveryResult result;
        result.accepted = true;
        result.state = OsdViewerDeliveryState::Frame;
        result.binding = binding;
        result.hasFrame = true;
        result.frame = frame;
        return result;
    }

    return rejectDelivery("legacy_osd_viewer_delivery_unavailable");
}

OsdViewerBindingResult OsdViewerBindingService::detach(
    const OsdViewerDetachRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false) ||
        !safeIdentifier(request.viewerBindingId, false))
        return rejectBinding("legacy_osd_viewer_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return rejectBinding("legacy_osd_time_unavailable");
    reapExpired(now);

    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = bindings_.find(request.viewerBindingId);
    if (found == bindings_.end() ||
        found->second.actorId != request.actorId ||
        found->second.clientInstanceId != request.clientInstanceId ||
        found->second.backendId != request.backendId ||
        found->second.legacyOsdSessionId != request.legacyOsdSessionId)
        return rejectBinding("legacy_osd_viewer_not_found");

    OsdViewerBinding binding = found->second;
    bindings_.erase(found);
    ++binding.bindingRevision;
    binding.lastSeenAt = now;
    binding.state = OsdViewerBindingState::Closed;
    binding.controlAuthorized = false;
    binding.closeReason = "detached";

    OsdViewerBindingResult result;
    result.accepted = true;
    result.binding = binding;
    return result;
}
