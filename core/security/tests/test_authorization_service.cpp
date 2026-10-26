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
    "channels.move",
    "recordings.marks.modify"
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
    // The first administrator is role.admin@*: it must control every
    // selected backend without assigning separate Remote/SearchTimer grants.
    for (const std::string& backend : {"default", "house-b"})
    {
        for (const std::string& permission :
             {"remote.control", "searchtimers.create", "searchtimers.modify",
              "searchtimers.delete", "searchtimers.execute",
              "searchtimers.view", "osd.control", "osd.view",
              "channels.view", "recordings.view", "timers.view", "epg.view",
              "recordings.execute"})
        {
            const auto decision = service.authorize(
                wildcardAdminRole, permissionRequest(permission, backend));
            assert(decision.allowed);
            assert(decision.reasonCode == "role_permission_granted");
        }
        for (const std::string& permission : kProtectedMutationPermissions)
            assert(service.authorize(
                wildcardAdminRole, permissionRequest(permission, backend)).allowed);
    }
    // Unrelated unreviewed actions remain fail-closed.
    assert(!service.authorize(wildcardAdminRole,
        permissionRequest("future.unreviewed.mutation", "default")).allowed);
    // The wildcard role is a Human administrator authority, never a
    // Service/Device permission escalation.
    RequestSecurityContext serviceAdmin = wildcardAdminRole;
    serviceAdmin.actor.type = ActorType::Service;
    assert(!service.authorize(serviceAdmin, requestFor("default")).allowed);
    // An explicit backend read-only role always takes precedence.
    RequestSecurityContext backendReadOnlyAdmin = wildcardAdminRole;
    backendReadOnlyAdmin.grants.push_back({"role.read-only", "default"});
    assert(service.authorize(backendReadOnlyAdmin,
        requestFor("default")).reasonCode == "role_read_only");
    assert(service.authorize(backendReadOnlyAdmin,
        permissionRequest("searchtimers.create", "default")).reasonCode ==
        "role_read_only");
    assert(service.authorize(backendReadOnlyAdmin,
        requestFor("house-b")).allowed);

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

    // Existing human-facing read permissions are now reviewed administrator
    // grants; an unknown future permission must still remain denied.
    const AuthorizationDecision adminCannotInventPermissions =
        service.authorize(adminRole,
            permissionRequest("unreviewed.future.permission", "default"));
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

    // Global administrators can use a root-installed add-on without a second
    // per-user grant; normal users need exact backend scope.
    RequestSecurityContext addonAdmin = authenticatedContext();
    addonAdmin.grants.push_back({"role.admin", "*"});
    assert(service.authorize(addonAdmin,
        permissionRequest("addons.media.import", "backend-a")).allowed);
    assert(service.authorize(addonAdmin,
        permissionRequest("addons.media.shrink", "backend-a")).allowed);
    addonAdmin.grants.push_back({"role.read-only", "backend-a"});
    assert(!service.authorize(addonAdmin,
        permissionRequest("addons.media.import", "backend-a")).allowed);

    // Media Tools import must be an exact backend-scoped permission for users.
    const auto importRequest = permissionRequest("addons.media.import", "backend-a");
    assert(!service.authorize(adminRole, importRequest).allowed);
    assert(!service.authorize(wildcard, importRequest).allowed);
    RequestSecurityContext importActor = authenticatedContext();
    importActor.grants.push_back({"addons.media.import", "*"});
    assert(!service.authorize(importActor, importRequest).allowed);
    importActor.grants.clear();
    importActor.grants.push_back({"addons.media.import", "backend-other"});
    assert(!service.authorize(importActor, importRequest).allowed);
    importActor.grants.push_back({"addons.media.import", "backend-a"});
    assert(service.authorize(importActor, importRequest).allowed);
    importActor.grants.push_back({"role.read-only", "backend-a"});
    assert(service.authorize(importActor, importRequest).reasonCode == "role_read_only");

    // HEVC is an independent explicit per-backend grant, never inherited
    // from general admin, import or wildcard privileges.
    const auto shrinkRequest =
        permissionRequest("addons.media.shrink", "backend-a");
    assert(!service.authorize(adminRole, shrinkRequest).allowed);
    assert(!service.authorize(wildcard, shrinkRequest).allowed);
    RequestSecurityContext shrinkActor = authenticatedContext();
    shrinkActor.grants.push_back({"addons.media.import", "backend-a"});
    assert(!service.authorize(shrinkActor, shrinkRequest).allowed);
    shrinkActor.grants.push_back({"addons.media.shrink", "*"});
    assert(!service.authorize(shrinkActor, shrinkRequest).allowed);
    shrinkActor.grants.push_back({"addons.media.shrink", "backend-other"});
    assert(!service.authorize(shrinkActor, shrinkRequest).allowed);
    shrinkActor.grants.push_back({"addons.media.shrink", "backend-a"});
    assert(service.authorize(shrinkActor, shrinkRequest).allowed);
    shrinkActor.grants.push_back({"role.read-only", "backend-a"});
    assert(service.authorize(shrinkActor, shrinkRequest).reasonCode ==
           "role_read_only");

    // Future and unknown add-on grants are not activated by raw DB rows,
    // administrator roles or wildcard grants.
    for (const auto* future : {"addons.media.check",
                               "addons.unknown.run"}) {
        const auto request = permissionRequest(future, "backend-a");
        assert(!service.authorize(adminRole, request).allowed);
        assert(!service.authorize(wildcard, request).allowed);
        RequestSecurityContext forged = authenticatedContext();
        forged.grants.push_back({future, "backend-a"});
        assert(!service.authorize(forged, request).allowed);
    }

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