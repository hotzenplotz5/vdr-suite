#include "AuthorizationService.h"

#include <cassert>

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

AuthorizationRequest requestFor(const std::string& backendId)
{
    AuthorizationRequest request;
    request.permission = "remote.control";
    request.backendId = backendId;
    request.action = "remote.control";
    return request;
}
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

    const AuthorizationDecision wrongScope =
        service.authorize(allowed, requestFor("house-b"));
    assert(!wrongScope.allowed);
    assert(wrongScope.reasonCode == "backend_scope_denied");

    RequestSecurityContext missingPermission = authenticatedContext();
    missingPermission.grants.push_back(PermissionGrant{"recordings.view", "*"});
    const AuthorizationDecision forbidden =
        service.authorize(missingPermission, requestFor("default"));
    assert(!forbidden.allowed);
    assert(forbidden.reasonCode == "permission_denied");

    RequestSecurityContext wildcard = authenticatedContext();
    wildcard.grants.push_back(PermissionGrant{"*", "*"});
    assert(service.authorize(wildcard, requestFor("house-b")).allowed);

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
