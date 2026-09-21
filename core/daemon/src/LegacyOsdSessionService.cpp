#include "LegacyOsdSessionService.h"

#include "AuthorizationService.h"

#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <ctime>
#include <utility>

namespace
{
constexpr std::size_t SessionEntropyBytes = 16;

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

std::string systemSessionId()
{
    std::array<unsigned char, SessionEntropyBytes> bytes{};
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        const ssize_t received =
            getrandom(bytes.data() + offset, bytes.size() - offset, 0);
        if (received < 0)
        {
            if (errno == EINTR) continue;
            return {};
        }
        if (received == 0) return {};
        offset += static_cast<std::size_t>(received);
    }
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result = "los_";
    for (unsigned char value : bytes)
    {
        result.push_back(Hex[(value >> 4U) & 0x0fU]);
        result.push_back(Hex[value & 0x0fU]);
    }
    return result;
}

std::int64_t systemNow()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

bool osdDomainAdvertised(const BackendAgentStatus& status)
{
    return std::find(status.capabilities.observationDomains.begin(),
                     status.capabilities.observationDomains.end(),
                     "osd") !=
        status.capabilities.observationDomains.end();
}
}

LegacyOsdSessionService::LegacyOsdSessionService(
    ContextResolver contextResolver,
    StatusLookup statusLookup,
    ObservationRead observationRead,
    SessionIdFactory sessionIdFactory,
    NowProvider nowProvider)
    : contextResolver_(std::move(contextResolver)),
      statusLookup_(std::move(statusLookup)),
      observationRead_(std::move(observationRead)),
      sessionIdFactory_(sessionIdFactory ? std::move(sessionIdFactory)
                                        : SessionIdFactory(systemSessionId)),
      nowProvider_(nowProvider ? std::move(nowProvider)
                              : NowProvider(systemNow))
{
}

LegacyOsdSessionResult LegacyOsdSessionService::reject(
    const std::string& error) const
{
    LegacyOsdSessionResult result;
    result.error = error;
    return result;
}

std::optional<RequestSecurityContext>
LegacyOsdSessionService::authorizedContext(
    const std::string& actorId,
    const std::string& backendId) const
{
    if (!contextResolver_) return std::nullopt;
    auto context = contextResolver_(actorId, backendId);
    if (!context.has_value() ||
        context->actor.type == ActorType::Agent ||
        !context->authenticated())
        return std::nullopt;

    AuthorizationRequest request;
    request.permission = "osd.view";
    request.backendId = backendId;
    request.action = "osd.view";
    if (!AuthorizationService().authorize(*context, request).allowed)
        return std::nullopt;
    return context;
}

bool LegacyOsdSessionService::currentOrResync(
    const BackendAgentOsdReadResult& read)
{
    using State = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;
    return (read.available &&
            read.snapshot.state == State::Current &&
            read.snapshot.buffer.hasFrame) ||
        (read.reasonCode == "osd_resync_required" &&
         read.snapshot.state == State::ResyncRequired &&
         read.snapshot.buffer.hasFrame);
}

void LegacyOsdSessionService::applyObservation(
    LegacyOsdSession& session,
    const BackendAgentOsdReadResult& read)
{
    using State = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;
    session.state = read.snapshot.state == State::ResyncRequired
        ? LegacyOsdSessionState::ResyncRequired
        : LegacyOsdSessionState::Active;
    if (read.snapshot.buffer.hasFrame)
    {
        const auto& frame = read.snapshot.buffer.observed.frame;
        session.osdSurfaceId = frame.surface.surfaceId;
        session.osdEpoch = frame.surface.osdEpoch;
    }
}

void LegacyOsdSessionService::store(const LegacyOsdSession& session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session.legacyOsdSessionId] = session;
}

void LegacyOsdSessionService::reapExpired(std::int64_t now)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end();)
    {
        if (now >= it->second.expiresAt) it = sessions_.erase(it);
        else ++it;
    }
}

std::optional<LegacyOsdSession> LegacyOsdSessionService::find(
    const std::string& sessionId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = sessions_.find(sessionId);
    if (found == sessions_.end()) return std::nullopt;
    return found->second;
}

LegacyOsdSessionResult LegacyOsdSessionService::create(
    const LegacyOsdSessionCreateRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.correlationId, true))
        return reject("legacy_osd_session_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return reject("legacy_osd_time_unavailable");

    auto context = authorizedContext(request.actorId, request.backendId);
    if (!context.has_value())
        return reject("legacy_osd_view_not_authorized");
    if (!statusLookup_ || !observationRead_)
        return reject("legacy_osd_runtime_unavailable");

    const BackendAgentStatus status =
        statusLookup_(request.backendId, now);
    if (!status.present ||
        status.state != BackendAgentConnectionState::Online ||
        status.backendGeneration == 0 ||
        !osdDomainAdvertised(status))
        return reject("legacy_osd_backend_unavailable");

    const BackendAgentOsdReadResult read =
        observationRead_(*context, request.backendId,
                         status.backendGeneration, now);
    if (!currentOrResync(read))
        return reject(read.reasonCode.empty()
            ? "legacy_osd_observation_unavailable"
            : read.reasonCode);

    reapExpired(now);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sessions_.size() >= MaximumSessions)
            return reject("legacy_osd_session_capacity_reached");
    }

    const std::string sessionId =
        sessionIdFactory_ ? sessionIdFactory_() : std::string{};
    if (!safeIdentifier(sessionId, false))
        return reject("legacy_osd_session_id_unavailable");
    if (find(sessionId).has_value())
        return reject("legacy_osd_session_id_collision");

    LegacyOsdSession session;
    session.legacyOsdSessionId = sessionId;
    session.sessionRevision = 1;
    session.actorId = request.actorId;
    session.clientInstanceId = request.clientInstanceId;
    session.backendId = request.backendId;
    session.backendGeneration = status.backendGeneration;
    session.mode = LegacyOsdSessionMode::ViewOnly;
    session.createdAt = now;
    session.expiresAt = now + SessionLifetimeSeconds;
    session.lastActivityAt = now;
    session.capabilitySnapshot.viewAvailable = true;
    session.capabilitySnapshot.controlAvailable = false;
    session.policySnapshot.viewAuthorized = true;
    session.policySnapshot.controlAuthorized = false;
    session.correlationId = request.correlationId;
    applyObservation(session, read);
    store(session);

    LegacyOsdSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}

LegacyOsdSessionResult LegacyOsdSessionService::status(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& clientInstanceId,
    const std::string& backendId)
{
    if (!safeIdentifier(sessionId, false) ||
        !safeIdentifier(actorId, true) ||
        !safeIdentifier(clientInstanceId, true) ||
        !safeIdentifier(backendId, false))
        return reject("legacy_osd_session_request_invalid");

    const auto stored = find(sessionId);
    if (!stored.has_value() ||
        stored->actorId != actorId ||
        stored->clientInstanceId != clientInstanceId ||
        stored->backendId != backendId)
        return reject("legacy_osd_session_not_found");

    LegacyOsdSession session = *stored;
    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return reject("legacy_osd_time_unavailable");

    if (now >= session.expiresAt)
    {
        session.state = LegacyOsdSessionState::Expired;
        session.closeReason = "expired";
        ++session.sessionRevision;
        session.lastActivityAt = now;
        store(session);
        LegacyOsdSessionResult result;
        result.error = "legacy_osd_session_expired";
        result.session = session;
        return result;
    }

    auto context = authorizedContext(actorId, backendId);
    if (!context.has_value())
    {
        session.state = LegacyOsdSessionState::Closed;
        session.closeReason = "osd_view_revoked";
        ++session.sessionRevision;
        session.lastActivityAt = now;
        store(session);
        LegacyOsdSessionResult result;
        result.error = "legacy_osd_view_not_authorized";
        result.session = session;
        return result;
    }

    const BackendAgentStatus status =
        statusLookup_ ? statusLookup_(backendId, now) : BackendAgentStatus{};
    if (!status.present || status.backendGeneration == 0 ||
        !osdDomainAdvertised(status))
    {
        session.state = LegacyOsdSessionState::Suspended;
        session.closeReason = "backend_unavailable";
        ++session.sessionRevision;
        session.lastActivityAt = now;
        store(session);
        LegacyOsdSessionResult result;
        result.accepted = true;
        result.session = session;
        return result;
    }

    if (status.backendGeneration != session.backendGeneration)
    {
        session.state = LegacyOsdSessionState::Closed;
        session.closeReason = "backend_generation_changed";
        ++session.sessionRevision;
        session.lastActivityAt = now;
        store(session);
        LegacyOsdSessionResult result;
        result.error = "legacy_osd_backend_generation_changed";
        result.session = session;
        return result;
    }

    if (status.state != BackendAgentConnectionState::Online)
    {
        session.state = LegacyOsdSessionState::Suspended;
        session.closeReason = "backend_not_online";
        ++session.sessionRevision;
        session.lastActivityAt = now;
        store(session);
        LegacyOsdSessionResult result;
        result.accepted = true;
        result.session = session;
        return result;
    }

    const BackendAgentOsdReadResult read =
        observationRead_(*context, backendId,
                         session.backendGeneration, now);
    if (!currentOrResync(read))
    {
        session.state = LegacyOsdSessionState::Suspended;
        session.closeReason = read.reasonCode.empty()
            ? "osd_unavailable" : read.reasonCode;
    }
    else
    {
        session.closeReason.clear();
        applyObservation(session, read);
    }

    ++session.sessionRevision;
    session.lastActivityAt = now;
    store(session);

    LegacyOsdSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}
