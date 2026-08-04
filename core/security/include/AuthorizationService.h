#pragma once

#include "SecurityIdentity.h"

#include <string>

struct AuthorizationRequest
{
    std::string permission;
    std::string backendId;
    std::string action;
};

struct AuthorizationDecision
{
    bool allowed = false;
    std::string reasonCode;
    std::string permission;
    std::string backendId;
    std::string action;
};

class AuthorizationService
{
public:
    AuthorizationDecision authorize(
        const RequestSecurityContext& context,
        const AuthorizationRequest& request) const
    {
        AuthorizationDecision decision;
        decision.permission = request.permission;
        decision.backendId = request.backendId;
        decision.action = request.action;

        if (context.authenticationState == AuthenticationState::Anonymous)
        {
            decision.reasonCode = "authentication_required";
            return decision;
        }
        if (context.authenticationState == AuthenticationState::Invalid)
        {
            decision.reasonCode = "invalid_credentials";
            return decision;
        }
        if (!context.actor.active || context.actor.actorId.empty())
        {
            decision.reasonCode = "actor_revoked";
            return decision;
        }
        if (context.device.has_value() && !context.device->active)
        {
            decision.reasonCode = "device_revoked";
            return decision;
        }
        if (context.credential.has_value())
        {
            if (context.credential->revoked || !context.credential->active)
            {
                decision.reasonCode = "credential_revoked";
                return decision;
            }
            if (context.credential->expired)
            {
                decision.reasonCode = "credential_expired";
                return decision;
            }
        }
        if (context.session.has_value())
        {
            if (context.session->revoked || !context.session->active)
            {
                decision.reasonCode = "session_revoked";
                return decision;
            }
            if (context.session->expired)
            {
                decision.reasonCode = "session_expired";
                return decision;
            }
        }
        if (context.authenticationState == AuthenticationState::Expired)
        {
            decision.reasonCode = "session_expired";
            return decision;
        }
        if (context.authenticationState == AuthenticationState::Revoked)
        {
            decision.reasonCode = "session_revoked";
            return decision;
        }
        if (context.permissionGrantResolution ==
            PermissionGrantResolutionState::Unavailable)
        {
            decision.reasonCode = "permission_grants_unavailable";
            return decision;
        }
        if (request.permission.empty())
        {
            decision.reasonCode = "invalid_permission";
            return decision;
        }
        if (request.backendId.empty())
        {
            decision.reasonCode = "invalid_backend_scope";
            return decision;
        }
        if (isReadOnlyMutation(context, request))
        {
            decision.reasonCode = "role_read_only";
            return decision;
        }

        bool permissionPresent = false;
        for (const PermissionGrant& grant : context.grants)
        {
            if (grant.permission != "role.admin" ||
                !adminRoleGrants(request.permission))
            {
                continue;
            }
            permissionPresent = true;
            if (grant.backendId == request.backendId)
            {
                decision.allowed = true;
                decision.reasonCode = "role_permission_granted";
                return decision;
            }
        }

        for (const PermissionGrant& grant : context.grants)
        {
            const bool permissionMatches =
                grant.permission == "*" ||
                grant.permission == request.permission;
            if (!permissionMatches)
            {
                continue;
            }
            permissionPresent = true;
            const bool backendMatches =
                grant.backendId.empty() ||
                grant.backendId == "*" ||
                grant.backendId == request.backendId;
            if (backendMatches)
            {
                decision.allowed = true;
                decision.reasonCode = "permission_granted";
                return decision;
            }
        }

        decision.reasonCode = permissionPresent
            ? "backend_scope_denied"
            : "permission_denied";
        return decision;
    }

private:
    static bool protectedMutationPermission(
        const std::string& permission)
    {
        return permission == "remote.control" ||
            permission == "timers.create" ||
            permission == "timers.modify" ||
            permission == "timers.delete" ||
            permission == "channels.move" ||
            permission == "recordings.rename" ||
            permission == "recordings.move" ||
            permission == "recordings.delete" ||
            permission == "searchtimers.create" ||
            permission == "searchtimers.modify" ||
            permission == "searchtimers.delete" ||
            permission == "searchtimers.execute" ||
            permission == "searchtimers.preview-cache.refresh" ||
            permission == "epg.cache.refresh" ||
            permission == "epgsearch.native-fuzzy.refresh" ||
            permission == "epgsearch.native-fuzzy.stale-probes.delete" ||
            permission == "backend.settings.series-artwork.modify";
    }

    static bool adminRoleGrants(const std::string& permission)
    {
        return protectedMutationPermission(permission);
    }

    static bool mutatingPermission(const std::string& permission)
    {
        return protectedMutationPermission(permission);
    }

    static bool isReadOnlyMutation(
        const RequestSecurityContext& context,
        const AuthorizationRequest& request)
    {
        if (!mutatingPermission(request.permission))
        {
            return false;
        }
        for (const PermissionGrant& grant : context.grants)
        {
            if (grant.permission == "role.read-only" &&
                grant.backendId == request.backendId)
            {
                return true;
            }
        }
        return false;
    }
};
