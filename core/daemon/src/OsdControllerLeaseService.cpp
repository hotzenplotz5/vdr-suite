#include "OsdControllerLeaseService.h"

#include "AuthorizationService.h"
#include "LegacyOsdSessionService.h"
#include "OsdViewerBindingService.h"

#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace
{
constexpr std::size_t LeaseEntropyBytes = 16;

bool safeIdentifier(
    const std::string& value,
    bool allowColon,
    std::size_t maximumLength = 128U)
{
    return !value.empty() && value.size() <= maximumLength &&
        std::all_of(
            value.begin(),
            value.end(),
            [allowColon](unsigned char c) {
                return std::isalnum(c) != 0 || c == '-' || c == '_' ||
                    c == '.' || (allowColon && c == ':');
            });
}

std::string systemLeaseId()
{
    std::array<unsigned char, LeaseEntropyBytes> bytes{};
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

    std::ostringstream output;
    output << "ocl_";
    for (unsigned char byte : bytes)
        output << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(byte);
    return output.str();
}

std::int64_t systemNow()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

bool activeState(OsdControllerLeaseState state)
{
    return state == OsdControllerLeaseState::Active ||
        state == OsdControllerLeaseState::Expiring;
}

bool controllableViewerState(OsdViewerBindingState state)
{
    return state == OsdViewerBindingState::Attached ||
        state == OsdViewerBindingState::ResyncRequired;
}

std::string terminalRenewError(const OsdControllerLease& lease)
{
    switch (lease.state)
    {
        case OsdControllerLeaseState::Expired:
            return "legacy_osd_controller_expired";
        case OsdControllerLeaseState::Revoked:
            return lease.revocationReason.empty()
                ? "legacy_osd_controller_revoked"
                : lease.revocationReason;
        case OsdControllerLeaseState::Released:
            return "legacy_osd_controller_released";
        default:
            return "controller_lease_conflict";
    }
}
}

OsdControllerLeaseService::OsdControllerLeaseService(
    LegacyOsdSessionService& sessionService,
    OsdViewerBindingService& viewerService,
    ContextResolver contextResolver,
    BackendPolicyLookup backendPolicyLookup,
    LeaseIdFactory leaseIdFactory,
    NowProvider nowProvider)
    : sessionService_(sessionService),
      viewerService_(viewerService),
      contextResolver_(std::move(contextResolver)),
      backendPolicyLookup_(std::move(backendPolicyLookup)),
      leaseIdFactory_(leaseIdFactory ? std::move(leaseIdFactory)
                                    : LeaseIdFactory(systemLeaseId)),
      nowProvider_(nowProvider ? std::move(nowProvider)
                              : NowProvider(systemNow))
{
}

OsdControllerLeaseResult OsdControllerLeaseService::reject(
    const std::string& error) const
{
    OsdControllerLeaseResult result;
    result.error = error;
    return result;
}

bool OsdControllerLeaseService::authorizeControl(
    const std::string& actorId,
    const std::string& backendId,
    std::string& error) const
{
    error.clear();
    if (!contextResolver_)
    {
        error = "legacy_osd_control_not_authorized";
        return false;
    }

    auto context = contextResolver_(actorId, backendId);
    if (!context.has_value() ||
        context->actor.type == ActorType::Agent ||
        !context->authenticated())
    {
        error = "legacy_osd_control_not_authorized";
        return false;
    }

    AuthorizationRequest request;
    request.permission = "osd.control";
    request.backendId = backendId;
    request.action = "osd.control";
    if (!AuthorizationService().authorize(*context, request).allowed)
    {
        error = "legacy_osd_control_not_authorized";
        return false;
    }

    if (!backendPolicyLookup_)
    {
        error = "legacy_osd_controller_backend_unavailable";
        return false;
    }

    const BackendAccessDecision policy = backendPolicyLookup_(backendId);
    if (!policy.allowed)
    {
        error = policy.readOnly
            ? "read_only_backend"
            : "legacy_osd_controller_backend_unavailable";
        return false;
    }

    return true;
}

bool OsdControllerLeaseService::resolveViewScope(
    const std::string& actorId,
    const std::string& clientInstanceId,
    const std::string& backendId,
    const std::string& legacyOsdSessionId,
    const std::string& viewerBindingId,
    std::int64_t now,
    HolderScope& scope,
    std::string& error) const
{
    error.clear();
    const LegacyOsdSessionResult sessionResult = sessionService_.status(
        legacyOsdSessionId,
        actorId,
        clientInstanceId,
        backendId);
    if (!sessionResult.accepted)
    {
        if (sessionResult.error == "legacy_osd_backend_generation_changed")
            error = "generation_conflict";
        else if (sessionResult.error == "legacy_osd_session_expired")
            error = "legacy_osd_controller_session_expired";
        else
            error = "legacy_osd_controller_session_invalid";
        return false;
    }

    const LegacyOsdSession& session = sessionResult.session;
    if ((session.state != LegacyOsdSessionState::Active &&
         session.state != LegacyOsdSessionState::ResyncRequired) ||
        session.backendGeneration == 0 ||
        session.osdSurfaceId.empty() ||
        session.osdEpoch.empty() ||
        now >= session.expiresAt)
    {
        error = "legacy_osd_controller_session_invalid";
        return false;
    }

    const auto binding = viewerService_.find(viewerBindingId);
    if (!binding.has_value() ||
        binding->actorId != actorId ||
        binding->clientInstanceId != clientInstanceId ||
        binding->backendId != backendId ||
        binding->legacyOsdSessionId != legacyOsdSessionId)
    {
        error = "legacy_osd_controller_viewer_invalid";
        return false;
    }

    if (!controllableViewerState(binding->state) ||
        now >= binding->expiresAt)
    {
        error = "legacy_osd_controller_viewer_invalid";
        return false;
    }

    if (binding->backendGeneration != session.backendGeneration)
    {
        error = "generation_conflict";
        return false;
    }
    if (binding->osdSurfaceId != session.osdSurfaceId)
    {
        error = "legacy_osd_controller_surface_changed";
        return false;
    }
    if (binding->osdEpoch != session.osdEpoch)
    {
        error = "legacy_osd_controller_epoch_changed";
        return false;
    }

    scope.backendGeneration = session.backendGeneration;
    scope.osdSurfaceId = session.osdSurfaceId;
    scope.osdEpoch = session.osdEpoch;
    scope.expiresAt = std::min(session.expiresAt, binding->expiresAt);
    return scope.expiresAt > now;
}

std::string OsdControllerLeaseService::surfaceKey(
    const std::string& backendId,
    std::uint64_t backendGeneration,
    const std::string& osdSurfaceId,
    const std::string& osdEpoch)
{
    return backendId + "|" + std::to_string(backendGeneration) + "|" +
        osdSurfaceId + "|" + osdEpoch;
}

void OsdControllerLeaseService::removeOwnerLocked(
    const OsdControllerLease& lease)
{
    const std::string key = surfaceKey(
        lease.backendId,
        lease.backendGeneration,
        lease.osdSurfaceId,
        lease.osdEpoch);
    const auto owner = surfaceOwners_.find(key);
    if (owner != surfaceOwners_.end() &&
        owner->second == lease.controllerLeaseId)
        surfaceOwners_.erase(owner);
}

void OsdControllerLeaseService::reapExpiredLocked(std::int64_t now)
{
    for (auto& item : leases_)
    {
        OsdControllerLease& lease = item.second;
        if (!activeState(lease.state) || now < lease.expiresAt) continue;
        removeOwnerLocked(lease);
        ++lease.leaseRevision;
        lease.state = OsdControllerLeaseState::Expired;
        lease.revocationReason = "expired";
    }
}

void OsdControllerLeaseService::pruneTerminalLocked()
{
    if (leases_.size() < MaximumLeases) return;
    for (auto it = leases_.begin();
         it != leases_.end() && leases_.size() >= MaximumLeases;)
    {
        if (activeState(it->second.state))
        {
            ++it;
            continue;
        }
        it = leases_.erase(it);
    }
}

void OsdControllerLeaseService::pruneSurfaceEpochsLocked()
{
    if (surfaceEpochs_.size() < MaximumSurfaceScopes) return;
    for (auto it = surfaceEpochs_.begin();
         it != surfaceEpochs_.end() &&
             surfaceEpochs_.size() >= MaximumSurfaceScopes;)
    {
        if (surfaceOwners_.count(it->first) != 0U)
        {
            ++it;
            continue;
        }
        it = surfaceEpochs_.erase(it);
    }
}

std::optional<OsdControllerLease> OsdControllerLeaseService::find(
    const std::string& controllerLeaseId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = leases_.find(controllerLeaseId);
    if (found == leases_.end()) return std::nullopt;
    return found->second;
}

OsdControllerLease OsdControllerLeaseService::endLease(
    const OsdControllerLease& snapshot,
    OsdControllerLeaseState state,
    const std::string& reason)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto found = leases_.find(snapshot.controllerLeaseId);
    if (found == leases_.end()) return snapshot;
    if (found->second.leaseRevision != snapshot.leaseRevision)
        return found->second;
    if (!activeState(found->second.state)) return found->second;

    removeOwnerLocked(found->second);
    ++found->second.leaseRevision;
    found->second.state = state;
    found->second.revocationReason = reason;
    return found->second;
}

std::optional<OsdControllerLease> OsdControllerLeaseService::refreshLease(
    const std::string& controllerLeaseId,
    std::int64_t now)
{
    const auto stored = find(controllerLeaseId);
    if (!stored.has_value()) return std::nullopt;
    OsdControllerLease lease = *stored;
    if (!activeState(lease.state)) return lease;

    if (now >= lease.expiresAt)
        return endLease(
            lease,
            OsdControllerLeaseState::Expired,
            "expired");

    std::string error;
    if (!authorizeControl(lease.actorId, lease.backendId, error))
        return endLease(
            lease,
            OsdControllerLeaseState::Revoked,
            error.empty() ? "osd_control_revoked" : error);

    HolderScope scope;
    if (!resolveViewScope(
            lease.actorId,
            lease.clientInstanceId,
            lease.backendId,
            lease.legacyOsdSessionId,
            lease.viewerBindingId,
            now,
            scope,
            error))
        return endLease(
            lease,
            OsdControllerLeaseState::Revoked,
            error.empty() ? "legacy_osd_controller_scope_invalid" : error);

    if (scope.backendGeneration != lease.backendGeneration)
        return endLease(
            lease,
            OsdControllerLeaseState::Revoked,
            "generation_conflict");
    if (scope.osdSurfaceId != lease.osdSurfaceId)
        return endLease(
            lease,
            OsdControllerLeaseState::Revoked,
            "legacy_osd_controller_surface_changed");
    if (scope.osdEpoch != lease.osdEpoch)
        return endLease(
            lease,
            OsdControllerLeaseState::Revoked,
            "legacy_osd_controller_epoch_changed");

    return lease;
}

OsdControllerLeaseResult OsdControllerLeaseService::acquire(
    const OsdControllerAcquireRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false) ||
        !safeIdentifier(request.viewerBindingId, false))
        return reject("legacy_osd_controller_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return reject("legacy_osd_time_unavailable");

    std::string error;
    if (!authorizeControl(request.actorId, request.backendId, error))
        return reject(error);

    HolderScope scope;
    if (!resolveViewScope(
            request.actorId,
            request.clientInstanceId,
            request.backendId,
            request.legacyOsdSessionId,
            request.viewerBindingId,
            now,
            scope,
            error))
        return reject(error);

    const std::string leaseId =
        leaseIdFactory_ ? leaseIdFactory_() : std::string{};
    if (!safeIdentifier(leaseId, false))
        return reject("legacy_osd_controller_id_unavailable");

    const std::string key = surfaceKey(
        request.backendId,
        scope.backendGeneration,
        scope.osdSurfaceId,
        scope.osdEpoch);

    std::lock_guard<std::mutex> lock(mutex_);
    reapExpiredLocked(now);

    const auto owner = surfaceOwners_.find(key);
    if (owner != surfaceOwners_.end())
    {
        const auto existing = leases_.find(owner->second);
        if (existing != leases_.end() &&
            activeState(existing->second.state))
            return reject("controller_lease_conflict");
        surfaceOwners_.erase(owner);
    }

    pruneTerminalLocked();
    if (leases_.size() >= MaximumLeases)
        return reject("legacy_osd_controller_capacity_reached");
    if (leases_.count(leaseId) != 0U)
        return reject("legacy_osd_controller_id_collision");

    if (surfaceEpochs_.count(key) == 0U)
    {
        pruneSurfaceEpochsLocked();
        if (surfaceEpochs_.size() >= MaximumSurfaceScopes)
            return reject("legacy_osd_controller_surface_capacity_reached");
    }

    std::uint64_t& epoch = surfaceEpochs_[key];
    if (epoch == std::numeric_limits<std::uint64_t>::max())
        return reject("legacy_osd_controller_epoch_exhausted");
    ++epoch;

    OsdControllerLease lease;
    lease.controllerLeaseId = leaseId;
    lease.leaseRevision = 1;
    lease.controllerLeaseEpoch = epoch;
    lease.legacyOsdSessionId = request.legacyOsdSessionId;
    lease.viewerBindingId = request.viewerBindingId;
    lease.actorId = request.actorId;
    lease.clientInstanceId = request.clientInstanceId;
    lease.backendId = request.backendId;
    lease.backendGeneration = scope.backendGeneration;
    lease.osdSurfaceId = scope.osdSurfaceId;
    lease.osdEpoch = scope.osdEpoch;
    lease.grantedAt = now;
    lease.expiresAt = std::min(
        now + LeaseLifetimeSeconds,
        scope.expiresAt);
    if (lease.expiresAt <= now)
        return reject("legacy_osd_controller_session_expired");
    lease.renewAfter = std::min(
        now + RenewAfterSeconds,
        lease.expiresAt);
    lease.lastHeartbeatAt = now;
    lease.state = OsdControllerLeaseState::Active;

    leases_.emplace(leaseId, lease);
    surfaceOwners_[key] = leaseId;

    OsdControllerLeaseResult result;
    result.accepted = true;
    result.hasLease = true;
    result.lease = lease;
    return result;
}

OsdControllerLeaseResult OsdControllerLeaseService::renew(
    const OsdControllerRenewRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false) ||
        !safeIdentifier(request.viewerBindingId, false) ||
        !safeIdentifier(request.controllerLeaseId, false) ||
        request.controllerLeaseEpoch == 0 ||
        request.leaseRevision == 0)
        return reject("legacy_osd_controller_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return reject("legacy_osd_time_unavailable");

    const auto refreshed = refreshLease(request.controllerLeaseId, now);
    if (!refreshed.has_value())
        return reject("legacy_osd_controller_not_found");
    OsdControllerLease lease = *refreshed;

    if (lease.actorId != request.actorId ||
        lease.clientInstanceId != request.clientInstanceId ||
        lease.backendId != request.backendId ||
        lease.legacyOsdSessionId != request.legacyOsdSessionId ||
        lease.viewerBindingId != request.viewerBindingId)
        return reject("legacy_osd_controller_not_found");
    if (lease.controllerLeaseEpoch != request.controllerLeaseEpoch)
        return reject("controller_lease_conflict");
    if (lease.leaseRevision != request.leaseRevision)
        return reject("revision_conflict");
    if (!activeState(lease.state))
        return reject(terminalRenewError(lease));
    if (now < lease.renewAfter)
        return reject("legacy_osd_controller_renew_too_early");

    HolderScope scope;
    std::string error;
    if (!resolveViewScope(
            request.actorId,
            request.clientInstanceId,
            request.backendId,
            request.legacyOsdSessionId,
            request.viewerBindingId,
            now,
            scope,
            error))
        return reject(error);

    std::lock_guard<std::mutex> lock(mutex_);
    auto found = leases_.find(request.controllerLeaseId);
    if (found == leases_.end())
        return reject("legacy_osd_controller_not_found");
    if (found->second.leaseRevision != request.leaseRevision ||
        found->second.controllerLeaseEpoch != request.controllerLeaseEpoch)
        return reject("revision_conflict");
    if (!activeState(found->second.state))
        return reject(terminalRenewError(found->second));

    ++found->second.leaseRevision;
    found->second.lastHeartbeatAt = now;
    found->second.expiresAt = std::min(
        now + LeaseLifetimeSeconds,
        scope.expiresAt);
    if (found->second.expiresAt <= now)
    {
        removeOwnerLocked(found->second);
        found->second.state = OsdControllerLeaseState::Expired;
        found->second.revocationReason = "expired";
        return reject("legacy_osd_controller_expired");
    }
    found->second.renewAfter = std::min(
        now + RenewAfterSeconds,
        found->second.expiresAt);
    found->second.state = OsdControllerLeaseState::Active;
    found->second.revocationReason.clear();

    OsdControllerLeaseResult result;
    result.accepted = true;
    result.hasLease = true;
    result.lease = found->second;
    return result;
}

OsdControllerLeaseResult OsdControllerLeaseService::release(
    const OsdControllerReleaseRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false) ||
        !safeIdentifier(request.viewerBindingId, false) ||
        !safeIdentifier(request.controllerLeaseId, false) ||
        request.controllerLeaseEpoch == 0)
        return reject("legacy_osd_controller_request_invalid");

    const auto stored = find(request.controllerLeaseId);
    if (!stored.has_value())
        return reject("legacy_osd_controller_not_found");
    const OsdControllerLease& lease = *stored;
    if (lease.actorId != request.actorId ||
        lease.clientInstanceId != request.clientInstanceId ||
        lease.backendId != request.backendId ||
        lease.legacyOsdSessionId != request.legacyOsdSessionId ||
        lease.viewerBindingId != request.viewerBindingId)
        return reject("legacy_osd_controller_not_found");
    if (lease.controllerLeaseEpoch != request.controllerLeaseEpoch)
        return reject("controller_lease_conflict");

    OsdControllerLease released = lease;
    if (activeState(lease.state))
        released = endLease(
            lease,
            OsdControllerLeaseState::Released,
            "released");

    OsdControllerLeaseResult result;
    result.accepted = true;
    result.hasLease = true;
    result.lease = released;
    return result;
}

OsdControllerLeaseResult OsdControllerLeaseService::current(
    const OsdControllerStatusRequest& request)
{
    if (!safeIdentifier(request.actorId, true) ||
        !safeIdentifier(request.clientInstanceId, true) ||
        !safeIdentifier(request.backendId, false) ||
        !safeIdentifier(request.legacyOsdSessionId, false) ||
        !safeIdentifier(request.viewerBindingId, false))
        return reject("legacy_osd_controller_request_invalid");

    const std::int64_t now = nowProvider_ ? nowProvider_() : -1;
    if (now < 0) return reject("legacy_osd_time_unavailable");

    {
        std::lock_guard<std::mutex> lock(mutex_);
        reapExpiredLocked(now);
    }

    const auto binding = viewerService_.find(request.viewerBindingId);
    if (!binding.has_value() ||
        binding->actorId != request.actorId ||
        binding->clientInstanceId != request.clientInstanceId ||
        binding->backendId != request.backendId ||
        binding->legacyOsdSessionId != request.legacyOsdSessionId ||
        now >= binding->expiresAt)
        return reject("legacy_osd_controller_viewer_invalid");

    const std::string observedKey = surfaceKey(
        binding->backendId,
        binding->backendGeneration,
        binding->osdSurfaceId,
        binding->osdEpoch);
    std::string leaseId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto owner = surfaceOwners_.find(observedKey);
        if (owner != surfaceOwners_.end()) leaseId = owner->second;
    }

    if (!leaseId.empty())
        refreshLease(leaseId, now);

    HolderScope scope;
    std::string error;
    if (!resolveViewScope(
            request.actorId,
            request.clientInstanceId,
            request.backendId,
            request.legacyOsdSessionId,
            request.viewerBindingId,
            now,
            scope,
            error))
        return reject(error);

    const std::string currentKey = surfaceKey(
        request.backendId,
        scope.backendGeneration,
        scope.osdSurfaceId,
        scope.osdEpoch);

    OsdControllerLeaseResult result;
    result.accepted = true;
    if (currentKey != observedKey) return result;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto owner = surfaceOwners_.find(currentKey);
        if (owner == surfaceOwners_.end()) return result;
        leaseId = owner->second;
    }

    const auto refreshed = refreshLease(leaseId, now);
    if (!refreshed.has_value() || !activeState(refreshed->state))
        return result;

    result.hasLease = true;
    result.lease = *refreshed;
    return result;
}

OsdControllerLeaseResult OsdControllerLeaseService::revoke(
    const std::string& controllerLeaseId,
    const std::string& reason)
{
    if (!safeIdentifier(controllerLeaseId, false) ||
        !safeIdentifier(reason, false))
        return reject("legacy_osd_controller_request_invalid");

    const auto stored = find(controllerLeaseId);
    if (!stored.has_value())
        return reject("legacy_osd_controller_not_found");

    OsdControllerLease lease = *stored;
    if (activeState(lease.state))
        lease = endLease(
            lease,
            OsdControllerLeaseState::Revoked,
            reason);

    OsdControllerLeaseResult result;
    result.accepted = true;
    result.hasLease = true;
    result.lease = lease;
    return result;
}
