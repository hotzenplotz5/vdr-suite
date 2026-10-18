#pragma once

#include "AuthorizationService.h"
#include "MediaSessionRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <map>
#include <mutex>
#include <string>
#include <utility>

// Runtime-only Device binding. MediaSessionRepository ends nonterminal
// sessions on restart; no old Device cookie is valid after recovery.
// A versioned media GET always rechecks current persistent identities and
// current permission grants. Legacy route cannot bypass Device revocation.
class PublicDevicePlaybackRegistry final
{
public:
    PublicDevicePlaybackRegistry(
        const MediaSessionRepository& sessions,
        const SecurityIdentityRepository& identities,
        const SecurityPermissionGrantRepository& grants)
        : sessions_(sessions), identities_(identities), grants_(grants) {}

    bool add(
        const std::string& sessionId,
        const std::string& backendId,
        const RequestSecurityContext& verifiedContext)
    {
        if (sessionId.empty() || backendId.empty() ||
            !verifiedContext.authenticated() ||
            verifiedContext.actor.type != ActorType::Service ||
            !verifiedContext.device || !verifiedContext.credential ||
            verifiedContext.device->deviceId.empty() ||
            verifiedContext.credential->credentialId.empty() ||
            verifiedContext.session ||
            verifiedContext.permissionGrantResolution !=
                PermissionGrantResolutionState::Resolved)
            return false;

        const auto stored = sessions_.findSession(sessionId);
        if (!stored || stored->state != "ready" ||
            stored->resourceKind != "recording" ||
            stored->actorId != verifiedContext.actor.actorId ||
            stored->backendId != backendId)
            return false;

        const Owner owner{
            verifiedContext.actor.actorId,
            verifiedContext.device->deviceId,
            verifiedContext.credential->credentialId,
            backendId};
        // Admission may take seconds to probe/transcode. Recheck persistent
        // grants and revocation immediately before publishing a Session.
        if (!currentGrant(owner, sessionId)) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        if (owners_.count(sessionId) || owners_.size() >= 128)
            return false;
        std::size_t actorSessions = 0;
        for (const auto& entry : owners_)
            if (entry.second.actorId == stored->actorId)
                ++actorSessions;
        if (actorSessions >= 4)
            return false;
        owners_.emplace(sessionId, owner);
        return true;
    }

    bool authorized(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& backendId,
        bool publicV1Path) const
    {
        Owner owner;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto found = owners_.find(sessionId);
            if (found == owners_.end())
                return !publicV1Path; // legacy-owned session
            owner = found->second;
        }
        if (owner.actorId != actorId || owner.backendId != backendId)
            return false;
        return currentGrant(owner, sessionId);
    }

    bool owned(
        const std::string& sessionId,
        const std::string& backendId,
        const RequestSecurityContext& verifiedContext) const
    {
        if (!verifiedContext.authenticated() ||
            !verifiedContext.device || !verifiedContext.credential ||
            verifiedContext.actor.type != ActorType::Service)
            return false;
        Owner owner;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto found = owners_.find(sessionId);
            if (found == owners_.end()) return false;
            owner = found->second;
        }
        return owner.backendId == backendId &&
            owner.actorId == verifiedContext.actor.actorId &&
            owner.deviceId == verifiedContext.device->deviceId &&
            owner.credentialId == verifiedContext.credential->credentialId &&
            currentGrant(owner, sessionId);
    }

    std::optional<StoredMediaSession> describeOwned(
        const std::string& sessionId,
        const RequestSecurityContext& verifiedDevice) const
    {
        // No caller-provided backend and no native recording ID in the API.
        // Resolve the session only after the same persistent Device,
        // Credential and current grant checks used for media bytes.
        const auto stored = sessions_.findSession(sessionId);
        if (!stored || stored->state != "ready" ||
            stored->resourceKind != "recording" ||
            !owned(sessionId, stored->backendId, verifiedDevice))
            return std::nullopt;
        return stored;
    }

    void erase(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        owners_.erase(sessionId);
    }

    void pruneTerminal()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto i = owners_.begin(); i != owners_.end();) {
            const auto stored = sessions_.findSession(i->first);
            if (!stored || stored->state != "ready")
                i = owners_.erase(i);
            else
                ++i;
        }
    }

private:
    struct Owner
    {
        std::string actorId;
        std::string deviceId;
        std::string credentialId;
        std::string backendId;
    };

    bool currentGrant(
        const Owner& owner,
        const std::string& sessionId) const
    {
        const auto session = sessions_.findSession(sessionId);
        if (!session || session->state != "ready" ||
            session->resourceKind != "recording" ||
            session->actorId != owner.actorId ||
            session->backendId != owner.backendId)
            return false;

        const auto actor = identities_.findActor(owner.actorId);
        const auto device = identities_.findDevice(owner.deviceId);
        const auto credential = identities_.findCredential(owner.credentialId);
        if (!actor || !actor->active || actor->revoked ||
            actor->type != ActorType::Service ||
            !device || !device->active || device->revoked ||
            device->actorId != owner.actorId ||
            !credential || !credential->active || credential->revoked ||
            credential->expired ||
            credential->actorId != owner.actorId ||
            credential->credentialType != "device-app")
            return false;

        const auto resolved = grants_.findActiveGrantsForActor(owner.actorId);
        if (!resolved.available) return false;
        RequestSecurityContext context;
        context.authenticationState = AuthenticationState::Authenticated;
        context.actor = {owner.actorId, ActorType::Service, actor->displayName, true};
        context.device = DeviceIdentity{owner.deviceId, true};
        context.credential = CredentialIdentity{owner.credentialId, true, false, false};
        context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
        context.grants = resolved.grants;
        return AuthorizationService().authorize(
            context, {"media.recording.play", owner.backendId,
                      "media.recording.play"}).allowed;
    }

    const MediaSessionRepository& sessions_;
    const SecurityIdentityRepository& identities_;
    const SecurityPermissionGrantRepository& grants_;
    mutable std::mutex mutex_;
    std::map<std::string, Owner> owners_;
};
