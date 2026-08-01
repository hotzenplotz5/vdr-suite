#include "AuthorizationService.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
RequestSecurityContext authenticatedContext()
{
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor.actorId = "user-1";
    context.actor.type = ActorType::User;
    context.actor.active = true;
    context.device = DeviceIdentity{"device-1", true};
    context.session = SessionIdentity{"session-1", true, false, false};
    context.credential = CredentialIdentity{
        "credential-1",
        true,
        false,
        false};
    return context;
}

AuthorizationRequest permissionRequest(
    const std::string& permission,
    const std::string& backendId)
{
    AuthorizationRequest request;
    request.permission = permission;
    request.backendId = backendId;
    request.action = permission;
    return request;
}

AuthorizationRequest requestFor(const std::string& backendId)
{
    return permissionRequest("remote.control", backendId);
}

AuthorizationRequest recordingsViewRequest(const std::string& backendId)
{
    return permissionRequest("recordings.view", backendId);
}

const std::vector<std::string> kProtectedMutationPermissions = {
    "timers.create",
    "timers.modify",
    "timers.delete",
    "channels.move"
};

const std::string kGlobalStaleProbeDeletePermission =
    "epgsearch.native-fuzzy.stale-probes.delete";
}

int main()
{
    AuthorizationService service;

    RequestSecurityContext anonymous;
    assert(service.authorize(anonymous, requestFor("default")).reasonCode ==
        "authentication_required");

    RequestSecurityContext allowed = authenticatedContext();
    allowed.grants.push_back(PermissionGrant{"remote.control", "default"});
    assert(service.authorize(allowed, requestFor("default")).allowed);

    for (const std::string& permission : kProtectedMutationPermissions)
    {
        RequestSecurityContext directTimerGrant = authenticatedContext();
        directTimerGrant.grants.push_back(
            PermissionGrant{permission, "default"});
        const AuthorizationDecision directTimerAllowed =
            service.authorize(
                directTimerGrant,
                permissionRequest(permission, "default"));
        assert(directTimerAllowed.allowed);
        assert(directTimerAllowed.reasonCode == "permission_granted");

        const AuthorizationDecision directTimerWrongScope =
            service.authorize(
                directTimerGrant,
                permissionRequest(permission, "house-b"));
        assert(!directTimerWrongScope.allowed);
        assert(directTimerWrongScope.reasonCode == "backend_scope_denied");
    }

    RequestSecurityContext adminRole = authenticatedContext();
    adminRole.grants.push_back(PermissionGrant{"role.admin", "default"});
    const AuthorizationDecision adminAllowed =
        service.authorize(adminRole, requestFor("default"));
    assert(adminAllowed.allowed);
    assert(adminAllowed.reasonCode == "role_permission_granted");

    for (const std::string& permission : kProtectedMutationPermissions)
    {
        const AuthorizationDecision adminTimerAllowed =
            service.authorize(
                adminRole,
                permissionRequest(permission, "default"));
        assert(adminTimerAllowed.allowed);
        assert(adminTimerAllowed.reasonCode == "role_permission_granted");

        const AuthorizationDecision adminTimerWrongScope =
            service.authorize(
                adminRole,
                permissionRequest(permission, "house-b"));
        assert(!adminTimerWrongScope.allowed);
        assert(adminTimerWrongScope.reasonCode == "backend_scope_denied");
    }

    const AuthorizationDecision adminWrongScope =
        service.authorize(adminRole, requestFor("house-b"));
    assert(!adminWrongScope.allowed);
    assert(adminWrongScope.reasonCode == "backend_scope_denied");

    RequestSecurityContext wildcardAdminRole = authenticatedContext();
    wildcardAdminRole.grants.push_back(PermissionGrant{"role.admin", "*"});
    const AuthorizationDecision wildcardAdminDenied =
        service.authorize(wildcardAdminRole, requestFor("default"));
    assert(!wildcardAdminDenied.allowed);
    assert(wildcardAdminDenied.reasonCode == "backend_scope_denied");
    for (const std::string& permission : kProtectedMutationPermissions)
    {
        const AuthorizationDecision wildcardAdminTimerDenied =
            service.authorize(
                wildcardAdminRole,
                permissionRequest(permission, "default"));
        assert(!wildcardAdminTimerDenied.allowed);
        assert(wildcardAdminTimerDenied.reasonCode ==
            "backend_scope_denied");
    }

    RequestSecurityContext globalDirect = authenticatedContext();
    globalDirect.grants.push_back(PermissionGrant{
        kGlobalStaleProbeDeletePermission,
        "*"});
    const AuthorizationDecision globalDirectAllowed =
        service.authorize(
            globalDirect,
            permissionRequest(
                kGlobalStaleProbeDeletePermission,
                "*"));
    assert(globalDirectAllowed.allowed);
    assert(globalDirectAllowed.reasonCode == "permission_granted");

    RequestSecurityContext concreteGlobalPermission = authenticatedContext();
    concreteGlobalPermission.grants.push_back(PermissionGrant{
        kGlobalStaleProbeDeletePermission,
        "default"});
    const AuthorizationDecision concreteGlobalPermissionDenied =
        service.authorize(
            concreteGlobalPermission,
            permissionRequest(
                kGlobalStaleProbeDeletePermission,
                "*"));
    assert(!concreteGlobalPermissionDenied.allowed);
    assert(concreteGlobalPermissionDenied.reasonCode ==
        "backend_scope_denied");

    const AuthorizationDecision globalAdminAllowed =
        service.authorize(
            wildcardAdminRole,
            permissionRequest(
                kGlobalStaleProbeDeletePermission,
                "*"));
    assert(globalAdminAllowed.allowed);
    assert(globalAdminAllowed.reasonCode ==
        "role_permission_granted");

    const AuthorizationDecision concreteAdminGlobalDenied =
        service.authorize(
            adminRole,
            permissionRequest(
                kGlobalStaleProbeDeletePermission,
                "*"));
    assert(!concreteAdminGlobalDenied.allowed);
    assert(concreteAdminGlobalDenied.reasonCode ==
        "backend_scope_denied");

    RequestSecurityContext globalReadOnly = globalDirect;
    globalReadOnly.grants.push_back(
        PermissionGrant{"role.read-only", "*"});
    const AuthorizationDecision globalReadOnlyDenied =
        service.authorize(
            globalReadOnly,
            permissionRequest(
                kGlobalStaleProbeDeletePermission,
                "*"));
    assert(!globalReadOnlyDenied.allowed);
    assert(globalReadOnlyDenied.reasonCode == "role_read_only");

    RequestSecurityContext concreteReadOnlyGlobal = globalDirect;
    concreteReadOnlyGlobal.grants.push_back(
        PermissionGrant{"role.read-only", "default"});
    assert(service.authorize(
        concreteReadOnlyGlobal,
        permissionRequest(
            kGlobalStaleProbeDeletePermission,
            "*")).allowed);

    RequestSecurityContext readOnlyRole = allowed;
    readOnlyRole.grants.push_back(PermissionGrant{"role.read-only", "default"});
    const AuthorizationDecision readOnlyDenied =
        service.authorize(readOnlyRole, requestFor("default"));
    assert(!readOnlyDenied.allowed);
    assert(readOnlyDenied.reasonCode == "role_read_only");

    for (const std::string& permission : kProtectedMutationPermissions)
    {
        RequestSecurityContext readOnlyTimer = adminRole;
        readOnlyTimer.grants.push_back(
            PermissionGrant{permission, "default"});
        readOnlyTimer.grants.push_back(
            PermissionGrant{"role.read-only", "default"});
        const AuthorizationDecision readOnlyTimerDenied =
            service.authorize(
                readOnlyTimer,
                permissionRequest(permission, "default"));
        assert(!readOnlyTimerDenied.allowed);
        assert(readOnlyTimerDenied.reasonCode == "role_read_only");
    }

    RequestSecurityContext scopedReadOnlyRole = allowed;
    scopedReadOnlyRole.grants.push_back(
        PermissionGrant{"role.read-only", "house-b"});
    assert(service.authorize(
        scopedReadOnlyRole,
        requestFor("default")).allowed);

    RequestSecurityContext scopedReadOnlyTimer = authenticatedContext();
    scopedReadOnlyTimer.grants.push_back(
        PermissionGrant{"timers.modify", "default"});
    scopedReadOnlyTimer.grants.push_back(
        PermissionGrant{"role.read-only", "house-b"});
    assert(service.authorize(
        scopedReadOnlyTimer,
        permissionRequest("timers.modify", "default")).allowed);

    RequestSecurityContext conflictingRoles = adminRole;
    conflictingRoles.grants.push_back(
        PermissionGrant{"role.read-only", "default"});
    const AuthorizationDecision conflictDenied =
        service.authorize(conflictingRoles, requestFor("default"));
    assert(!conflictDenied.allowed);
    assert(conflictDenied.reasonCode == "role_read_only");

    const AuthorizationDecision adminCannotInventPermissions =
        service.authorize(adminRole, recordingsViewRequest("default"));
    assert(!adminCannotInventPermissions.allowed);
    assert(adminCannotInventPermissions.reasonCode == "permission_denied");

    const AuthorizationDecision wrongScope =
        service.authorize(allowed, requestFor("house-b"));
    assert(!wrongScope.allowed);
    assert(wrongScope.reasonCode == "backend_scope_denied");

    RequestSecurityContext unavailableGrants = authenticatedContext();
    unavailableGrants.permissionGrantResolution =
        PermissionGrantResolutionState::Unavailable;
    const AuthorizationDecision unavailable =
        service.authorize(unavailableGrants, requestFor("default"));
    assert(!unavailable.allowed);
    assert(unavailable.reasonCode ==
        "permission_grants_unavailable");

    RequestSecurityContext missingPermission = authenticatedContext();
    missingPermission.grants.push_back(PermissionGrant{"recordings.view", "*"});
    const AuthorizationDecision forbidden =
        service.authorize(missingPermission, requestFor("default"));
    assert(!forbidden.allowed);
    assert(forbidden.reasonCode == "permission_denied");

    RequestSecurityContext wildcard = authenticatedContext();
    wildcard.grants.push_back(PermissionGrant{"*", "*"});
    assert(service.authorize(wildcard, requestFor("house-b")).allowed);
    for (const std::string& permission : kProtectedMutationPermissions)
    {
        assert(service.authorize(
            wildcard,
            permissionRequest(permission, "house-b")).allowed);
    }
    assert(service.authorize(
        wildcard,
        permissionRequest(
            kGlobalStaleProbeDeletePermission,
            "*")).allowed);

    RequestSecurityContext expired = allowed;
    expired.session->expired = true;
    assert(service.authorize(expired, requestFor("default")).reasonCode ==
        "session_expired");

    RequestSecurityContext revokedSession = allowed;
    revokedSession.session->revoked = true;
    assert(service.authorize(revokedSession, requestFor("default")).reasonCode ==
        "session_revoked");

    RequestSecurityContext expiredCredential = allowed;
    expiredCredential.credential->expired = true;
    assert(service.authorize(
        expiredCredential,
        requestFor("default")).reasonCode == "credential_expired");

    RequestSecurityContext revokedCredential = allowed;
    revokedCredential.credential->revoked = true;
    assert(service.authorize(
        revokedCredential,
        requestFor("default")).reasonCode == "credential_revoked");

    RequestSecurityContext revokedActor = allowed;
    revokedActor.actor.active = false;
    assert(service.authorize(revokedActor, requestFor("default")).reasonCode ==
        "actor_revoked");

    RequestSecurityContext revokedDevice = allowed;
    revokedDevice.device->active = false;
    assert(service.authorize(revokedDevice, requestFor("default")).reasonCode ==
        "device_revoked");

    RequestSecurityContext invalid = allowed;
    invalid.authenticationState = AuthenticationState::Invalid;
    assert(service.authorize(invalid, requestFor("default")).reasonCode ==
        "invalid_credentials");

    assert(service.authorize(allowed, requestFor("")).reasonCode ==
        "invalid_backend_scope");

    return 0;
}
