#include "HbbtvApplicationSessionService.h"

#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <utility>

namespace
{

constexpr std::size_t SessionEntropyBytes = 16;

bool safeIdentifier(const std::string& value, std::size_t maximumLength = 128)
{
    return !value.empty() && value.size() <= maximumLength &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
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
    std::string result = "bas_";
    result.reserve(4 + bytes.size() * 2);
    for (unsigned char value : bytes)
    {
        result.push_back(Hex[(value >> 4) & 0x0f]);
        result.push_back(Hex[value & 0x0f]);
    }
    return result;
}

std::int64_t systemNow()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

BroadcastApplicationRuntimeCapabilityProfile runtimeCapabilities()
{
    BroadcastApplicationRuntimeCapabilityProfile profile;
    profile.inputActions = {
        "up", "down", "left", "right", "ok", "back",
        "red", "green", "yellow", "blue",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "play", "pause", "stop", "fast_forward", "rewind",
    };
    return profile;
}

bool sameApplication(
    const BroadcastApplicationRef& expected,
    const BroadcastApplicationRef& current)
{
    return current.backendId == expected.backendId &&
        current.backendGeneration == expected.backendGeneration &&
        current.channelId == expected.channelId &&
        current.provider.providerId == expected.provider.providerId &&
        current.provider.providerSchemaVersion ==
            expected.provider.providerSchemaVersion &&
        current.provider.capabilityRevision ==
            expected.provider.capabilityRevision &&
        current.applicationId == expected.applicationId &&
        current.descriptorRevision == expected.descriptorRevision;
}

bool validApplicationRef(const BroadcastApplicationRef& application)
{
    return safeIdentifier(application.backendId) &&
        application.backendGeneration != 0 &&
        safeIdentifier(application.channelId) &&
        application.provider.providerId == "vdr-plugin-web" &&
        application.provider.providerSchemaVersion == 1 &&
        application.provider.capabilityRevision != 0 &&
        application.applicationId != 0 &&
        application.descriptorRevision != 0 &&
        application.provider.capabilityRevision ==
            application.descriptorRevision;
}

bool terminal(BroadcastApplicationSessionState state)
{
    return state == BroadcastApplicationSessionState::Closed ||
        state == BroadcastApplicationSessionState::Expired ||
        state == BroadcastApplicationSessionState::Failed;
}

void applyRuntimeState(
    BroadcastApplicationSession& session,
    const SuiteBridgeHbbtvRuntimeResolution& runtime)
{
    switch (runtime.stateCode)
    {
        case 1:
            session.state = BroadcastApplicationSessionState::Starting;
            break;
        case 2:
            session.state = BroadcastApplicationSessionState::Active;
            break;
        case 3:
            session.state = BroadcastApplicationSessionState::Closing;
            break;
        case 4:
            session.state = BroadcastApplicationSessionState::Failed;
            break;
        default:
            break;
    }
}

SuiteBridgeHbbtvRuntimeRequest runtimeRequest(
    const BroadcastApplicationSession& session,
    SuiteBridgeHbbtvRuntimeOperation operation)
{
    SuiteBridgeHbbtvRuntimeRequest request;
    request.operation = operation;
    request.sessionId = session.broadcastApplicationSessionId;
    request.channelId = session.application.channelId;
    request.applicationId = session.application.applicationId;
    request.descriptorRevision = session.application.descriptorRevision;
    return request;
}

} // namespace

HbbtvApplicationSessionService::HbbtvApplicationSessionService(
    IHbbtvApplicationDiscoveryService& discoveryService,
    RuntimeLookup runtimeLookup,
    AuthorizationCheck authorizationCheck,
    SessionIdFactory sessionIdFactory,
    NowProvider nowProvider)
    : discoveryService_(discoveryService),
      runtimeLookup_(std::move(runtimeLookup)),
      authorizationCheck_(std::move(authorizationCheck)),
      sessionIdFactory_(
          sessionIdFactory ? std::move(sessionIdFactory) :
              SessionIdFactory(systemSessionId)),
      nowProvider_(
          nowProvider ? std::move(nowProvider) : NowProvider(systemNow))
{
}

BroadcastApplicationSessionResult HbbtvApplicationSessionService::reject(
    const std::string& error) const
{
    BroadcastApplicationSessionResult result;
    result.error = error;
    return result;
}

bool HbbtvApplicationSessionService::authorized(
    const char* permission,
    const BroadcastApplicationSession& session) const
{
    return permission != nullptr &&
        authorizationCheck_ &&
        authorizationCheck_(
            permission,
            session.actorId,
            session.backendId);
}

bool HbbtvApplicationSessionService::owns(
    const BroadcastApplicationSession& session,
    const std::string& actorId,
    const std::string& clientContext) const
{
    return session.actorId == actorId &&
        session.clientContext == clientContext;
}

bool HbbtvApplicationSessionService::applicationCurrent(
    const BroadcastApplicationRef& application) const
{
    const BroadcastApplicationDiscoverySnapshot snapshot =
        discoveryService_.discoverApplications(
            application.backendId,
            application.channelId);

    if (!snapshot.payloadValid ||
        snapshot.backendId != application.backendId ||
        snapshot.backendGeneration != application.backendGeneration ||
        snapshot.channelId != application.channelId ||
        snapshot.result != "ok")
    {
        return false;
    }

    return std::any_of(
        snapshot.applications.begin(),
        snapshot.applications.end(),
        [&application](const BroadcastApplicationDescriptor& descriptor) {
            return sameApplication(application, descriptor.ref);
        });
}

IHbbtvRuntimeControl* HbbtvApplicationSessionService::runtimeFor(
    const BroadcastApplicationSession& session) const
{
    return runtimeLookup_ ? runtimeLookup_(session.backendId) : nullptr;
}

void HbbtvApplicationSessionService::store(
    const BroadcastApplicationSession& session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session.broadcastApplicationSessionId] = session;
}

std::optional<BroadcastApplicationSession>
HbbtvApplicationSessionService::find(
    const std::string& sessionId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = sessions_.find(sessionId);
    if (found == sessions_.end()) return std::nullopt;
    return found->second;
}

BroadcastApplicationSessionResult HbbtvApplicationSessionService::launch(
    const BroadcastApplicationLaunchRequest& request)
{
    if (!safeIdentifier(request.actorId) ||
        !safeIdentifier(request.clientContext) ||
        !safeIdentifier(request.correlationContext) ||
        !validApplicationRef(request.application) ||
        request.lifetimeSeconds < 60 ||
        request.lifetimeSeconds > 21600)
    {
        return reject("hbbtv_session_request_invalid");
    }

    BroadcastApplicationSession session;
    session.actorId = request.actorId;
    session.clientContext = request.clientContext;
    session.backendId = request.application.backendId;
    session.backendGeneration = request.application.backendGeneration;
    session.application = request.application;
    session.applicationDescriptorRevision =
        request.application.descriptorRevision;
    session.state = BroadcastApplicationSessionState::Requested;
    session.createdAt = nowProvider_();
    session.expiresAt = session.createdAt + request.lifetimeSeconds;
    session.runtimeCapabilityProfile = runtimeCapabilities();
    session.correlationContext = request.correlationContext;

    if (!authorized("broadcast.hbbtv.launch", session))
        return reject("hbbtv_launch_not_authorized");

    if (!applicationCurrent(request.application))
        return reject("hbbtv_application_context_stale");

    const std::string sessionId =
        sessionIdFactory_ ? sessionIdFactory_() : std::string{};
    if (!safeIdentifier(sessionId))
        return reject("hbbtv_session_id_unavailable");

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sessions_.find(sessionId) != sessions_.end())
            return reject("hbbtv_session_id_collision");
        session.broadcastApplicationSessionId = sessionId;
        sessions_.emplace(sessionId, session);
    }

    IHbbtvRuntimeControl* runtime = runtimeFor(session);
    if (runtime == nullptr)
    {
        session.state = BroadcastApplicationSessionState::Failed;
        session.closeReason = "hbbtv_runtime_unavailable";
        store(session);
        BroadcastApplicationSessionResult result;
        result.error = session.closeReason;
        result.session = session;
        return result;
    }

    const SuiteBridgeHbbtvRuntimeResolution runtimeResult =
        runtime->control(runtimeRequest(
            session,
            SuiteBridgeHbbtvRuntimeOperation::Launch));

    if (!runtimeResult.payloadValid)
    {
        session.state = BroadcastApplicationSessionState::Failed;
        session.closeReason = runtimeResult.error.empty()
            ? "hbbtv_runtime_launch_failed"
            : runtimeResult.error;
        store(session);
        BroadcastApplicationSessionResult result;
        result.error = session.closeReason;
        result.session = session;
        return result;
    }

    if ((runtimeResult.resultCode != 0 && runtimeResult.resultCode != 1) ||
        (runtimeResult.stateCode != 1 && runtimeResult.stateCode != 2))
    {
        session.state = BroadcastApplicationSessionState::Failed;
        session.closeReason = "hbbtv_runtime_" + runtimeResult.result;
        store(session);
        BroadcastApplicationSessionResult result;
        result.error = session.closeReason;
        result.session = session;
        return result;
    }

    applyRuntimeState(session, runtimeResult);
    store(session);

    BroadcastApplicationSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}

BroadcastApplicationSessionResult HbbtvApplicationSessionService::refresh(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& clientContext)
{
    const auto stored = find(sessionId);
    if (!stored.has_value())
        return reject("hbbtv_session_not_found");

    BroadcastApplicationSession session = *stored;
    if (!owns(session, actorId, clientContext))
        return reject("hbbtv_session_owner_mismatch");
    if (!authorized("broadcast.session.manage_own", session))
        return reject("hbbtv_session_manage_not_authorized");

    if (!terminal(session.state) && nowProvider_() >= session.expiresAt)
    {
        session.state = BroadcastApplicationSessionState::Expired;
        session.closeReason = "expired";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_session_expired";
        result.session = session;
        return result;
    }

    if (terminal(session.state))
    {
        BroadcastApplicationSessionResult result;
        result.accepted =
            session.state == BroadcastApplicationSessionState::Closed;
        result.error = result.accepted ? std::string{} :
            "hbbtv_session_not_active";
        result.session = session;
        return result;
    }

    if (session.state != BroadcastApplicationSessionState::Closing &&
        !applicationCurrent(session.application))
    {
        session.state = BroadcastApplicationSessionState::Suspended;
        session.closeReason = "application_context_stale";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_application_context_stale";
        result.session = session;
        return result;
    }

    IHbbtvRuntimeControl* runtime = runtimeFor(session);
    if (runtime == nullptr)
        return reject("hbbtv_runtime_unavailable");

    const SuiteBridgeHbbtvRuntimeResolution runtimeResult =
        runtime->control(runtimeRequest(
            session,
            SuiteBridgeHbbtvRuntimeOperation::Status));

    if (!runtimeResult.payloadValid)
        return reject(runtimeResult.error);

    if (runtimeResult.resultCode == 6 &&
        session.state == BroadcastApplicationSessionState::Closing)
    {
        session.state = BroadcastApplicationSessionState::Closed;
        if (session.closeReason.empty())
            session.closeReason = "closed";
        store(session);

        BroadcastApplicationSessionResult result;
        result.accepted = true;
        result.session = session;
        return result;
    }

    if (runtimeResult.resultCode != 0)
    {
        session.state = BroadcastApplicationSessionState::Degraded;
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_runtime_" + runtimeResult.result;
        result.session = session;
        return result;
    }

    applyRuntimeState(session, runtimeResult);
    store(session);

    BroadcastApplicationSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}

BroadcastApplicationSessionResult
HbbtvApplicationSessionService::authorizePresentation(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& clientContext)
{
    const auto stored = find(sessionId);
    if (!stored.has_value())
        return reject("hbbtv_session_not_found");

    BroadcastApplicationSession session = *stored;
    if (!owns(session, actorId, clientContext))
        return reject("hbbtv_session_owner_mismatch");
    if (!authorized("broadcast.session.manage_own", session))
        return reject("hbbtv_session_manage_not_authorized");

    if (nowProvider_() >= session.expiresAt)
    {
        session.state = BroadcastApplicationSessionState::Expired;
        session.closeReason = "expired";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_session_expired";
        result.session = session;
        return result;
    }

    if (session.state != BroadcastApplicationSessionState::Starting &&
        session.state != BroadcastApplicationSessionState::Active &&
        session.state != BroadcastApplicationSessionState::Degraded)
        return reject("hbbtv_session_not_active");

    if (!applicationCurrent(session.application))
    {
        session.state = BroadcastApplicationSessionState::Suspended;
        session.closeReason = "application_context_stale";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_application_context_stale";
        result.session = session;
        return result;
    }

    BroadcastApplicationSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}

BroadcastApplicationSessionResult HbbtvApplicationSessionService::input(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& clientContext,
    SuiteBridgeHbbtvInputAction action)
{
    const auto stored = find(sessionId);
    if (!stored.has_value())
        return reject("hbbtv_session_not_found");

    BroadcastApplicationSession session = *stored;
    if (!owns(session, actorId, clientContext))
        return reject("hbbtv_session_owner_mismatch");
    if (!authorized("broadcast.hbbtv.input", session))
        return reject("hbbtv_input_not_authorized");

    if (nowProvider_() >= session.expiresAt)
    {
        session.state = BroadcastApplicationSessionState::Expired;
        session.closeReason = "expired";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_session_expired";
        result.session = session;
        return result;
    }

    if (session.state != BroadcastApplicationSessionState::Active)
        return reject("hbbtv_session_not_active");

    if (action == SuiteBridgeHbbtvInputAction::None)
        return reject("hbbtv_input_action_invalid");

    if (!applicationCurrent(session.application))
    {
        session.state = BroadcastApplicationSessionState::Suspended;
        session.closeReason = "application_context_stale";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_application_context_stale";
        result.session = session;
        return result;
    }

    IHbbtvRuntimeControl* runtime = runtimeFor(session);
    if (runtime == nullptr)
        return reject("hbbtv_runtime_unavailable");

    SuiteBridgeHbbtvRuntimeRequest request = runtimeRequest(
        session,
        SuiteBridgeHbbtvRuntimeOperation::Input);
    request.inputAction = action;

    const SuiteBridgeHbbtvRuntimeResolution runtimeResult =
        runtime->control(request);

    if (!runtimeResult.payloadValid)
        return reject(runtimeResult.error);

    if (runtimeResult.resultCode == 3)
    {
        session.state = BroadcastApplicationSessionState::Suspended;
        session.closeReason = "discovery_stale";
        store(session);

        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_runtime_discovery_stale";
        result.session = session;
        return result;
    }

    if (runtimeResult.resultCode != 0 ||
        runtimeResult.stateCode != 2)
    {
        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_runtime_" + runtimeResult.result;
        result.session = session;
        return result;
    }

    session.state = BroadcastApplicationSessionState::Active;
    store(session);

    BroadcastApplicationSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}

BroadcastApplicationSessionResult HbbtvApplicationSessionService::close(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& clientContext,
    const std::string& reason)
{
    const auto stored = find(sessionId);
    if (!stored.has_value())
        return reject("hbbtv_session_not_found");

    BroadcastApplicationSession session = *stored;
    if (!owns(session, actorId, clientContext))
        return reject("hbbtv_session_owner_mismatch");
    if (!authorized("broadcast.session.manage_own", session))
        return reject("hbbtv_session_manage_not_authorized");
    if (!safeIdentifier(reason))
        return reject("hbbtv_close_reason_invalid");

    if (session.state == BroadcastApplicationSessionState::Closed)
    {
        BroadcastApplicationSessionResult result;
        result.accepted = true;
        result.session = session;
        return result;
    }

    IHbbtvRuntimeControl* runtime = runtimeFor(session);
    if (runtime == nullptr)
        return reject("hbbtv_runtime_unavailable");

    const SuiteBridgeHbbtvRuntimeResolution runtimeResult =
        runtime->control(runtimeRequest(
            session,
            SuiteBridgeHbbtvRuntimeOperation::Close));

    if (!runtimeResult.payloadValid)
        return reject(runtimeResult.error);

    if (runtimeResult.resultCode == 6)
    {
        session.state = BroadcastApplicationSessionState::Closed;
        session.closeReason = reason;
        store(session);

        BroadcastApplicationSessionResult result;
        result.accepted = true;
        result.session = session;
        return result;
    }

    if ((runtimeResult.resultCode != 0 && runtimeResult.resultCode != 1) ||
        runtimeResult.stateCode != 3)
    {
        BroadcastApplicationSessionResult result;
        result.error = "hbbtv_runtime_" + runtimeResult.result;
        result.session = session;
        return result;
    }

    session.state = BroadcastApplicationSessionState::Closing;
    session.closeReason = reason;
    store(session);

    BroadcastApplicationSessionResult result;
    result.accepted = true;
    result.session = session;
    return result;
}
