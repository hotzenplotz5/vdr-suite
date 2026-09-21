#include "AuthorizationService.h"

#include <cassert>
#include <string>

namespace
{
constexpr const char* RecordingPlaybackPermission =
    "media.recording.play";

RequestSecurityContext authenticatedContext()
{
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor.actorId = "phase65-playback-user";
    context.actor.type = ActorType::User;
    context.actor.active = true;
    context.device = DeviceIdentity{"phase65-playback-device", true};
    context.session = SessionIdentity{
        "phase65-playback-session",
        true,
        false,
        false};
    context.credential = CredentialIdentity{
        "phase65-playback-credential",
        true,
        false,
        false};
    return context;
}

AuthorizationRequest playbackRequest(const std::string& backendId)
{
    AuthorizationRequest request;
    request.permission = RecordingPlaybackPermission;
    request.backendId = backendId;
    request.action = RecordingPlaybackPermission;
    return request;
}
}

int main()
{
    AuthorizationService service;

    RequestSecurityContext direct = authenticatedContext();
    direct.grants.push_back(
        PermissionGrant{RecordingPlaybackPermission, "default"});

    const AuthorizationDecision directAllowed =
        service.authorize(direct, playbackRequest("default"));
    assert(directAllowed.allowed);
    assert(directAllowed.reasonCode == "permission_granted");
    assert(directAllowed.permission == RecordingPlaybackPermission);
    assert(directAllowed.backendId == "default");

    const AuthorizationDecision wrongScope =
        service.authorize(direct, playbackRequest("house-b"));
    assert(!wrongScope.allowed);
    assert(wrongScope.reasonCode == "backend_scope_denied");

    RequestSecurityContext missing = authenticatedContext();
    const AuthorizationDecision missingPermission =
        service.authorize(missing, playbackRequest("default"));
    assert(!missingPermission.allowed);
    assert(missingPermission.reasonCode == "permission_denied");

    RequestSecurityContext readOnly = direct;
    readOnly.grants.push_back(
        PermissionGrant{"role.read-only", "default"});
    const AuthorizationDecision readOnlyAllowed =
        service.authorize(readOnly, playbackRequest("default"));
    assert(readOnlyAllowed.allowed);
    assert(readOnlyAllowed.reasonCode == "permission_granted");

    RequestSecurityContext admin = authenticatedContext();
    admin.grants.push_back(
        PermissionGrant{"role.admin", "default"});
    const AuthorizationDecision adminAllowed =
        service.authorize(admin, playbackRequest("default"));
    assert(adminAllowed.allowed);
    assert(adminAllowed.reasonCode == "role_permission_granted");

    RequestSecurityContext readOnlyAdmin = admin;
    readOnlyAdmin.grants.push_back(
        PermissionGrant{"role.read-only", "default"});
    const AuthorizationDecision readOnlyAdminAllowed =
        service.authorize(readOnlyAdmin, playbackRequest("default"));
    assert(readOnlyAdminAllowed.allowed);
    assert(readOnlyAdminAllowed.reasonCode ==
        "role_permission_granted");

    const AuthorizationDecision adminWrongScope =
        service.authorize(admin, playbackRequest("house-b"));
    assert(!adminWrongScope.allowed);
    assert(adminWrongScope.reasonCode == "backend_scope_denied");

    AuthorizationRequest downloadRequest;
    downloadRequest.permission = "media.recording.download";
    downloadRequest.backendId = "default";
    downloadRequest.action = "media.recording.download";
    const AuthorizationDecision downloadNotImplicit =
        service.authorize(admin, downloadRequest);
    assert(!downloadNotImplicit.allowed);
    assert(downloadNotImplicit.reasonCode == "permission_denied");

    const AuthorizationDecision missingBackend =
        service.authorize(direct, playbackRequest(""));
    assert(!missingBackend.allowed);
    assert(missingBackend.reasonCode == "invalid_backend_scope");

    return 0;
}
