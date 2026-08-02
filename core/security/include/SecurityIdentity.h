#pragma once

#include <optional>
#include <string>
#include <vector>

enum class ActorType
{
    Anonymous,
    User,
    Service,
    Agent,
    System
};

enum class AuthenticationState
{
    Anonymous,
    Authenticated,
    Invalid,
    Expired,
    Revoked
};

enum class PermissionGrantResolutionState
{
    NotRequired,
    Resolved,
    Unavailable
};

inline std::string actorTypeName(ActorType type)
{
    switch (type)
    {
        case ActorType::User: return "user";
        case ActorType::Service: return "service";
        case ActorType::Agent: return "agent";
        case ActorType::System: return "system";
        case ActorType::Anonymous:
        default:
            return "anonymous";
    }
}

inline ActorType actorTypeFromName(const std::string& type)
{
    if (type == "user") return ActorType::User;
    if (type == "service") return ActorType::Service;
    if (type == "agent") return ActorType::Agent;
    if (type == "system") return ActorType::System;
    return ActorType::Anonymous;
}

inline std::string authenticationStateName(AuthenticationState state)
{
    switch (state)
    {
        case AuthenticationState::Authenticated: return "authenticated";
        case AuthenticationState::Invalid: return "invalid";
        case AuthenticationState::Expired: return "expired";
        case AuthenticationState::Revoked: return "revoked";
        case AuthenticationState::Anonymous:
        default:
            return "anonymous";
    }
}

struct PermissionGrant
{
    std::string permission;
    std::string backendId = "*";
};

struct ActorIdentity
{
    std::string actorId;
    ActorType type = ActorType::Anonymous;
    std::string displayName;
    bool active = true;
};

struct DeviceIdentity
{
    std::string deviceId;
    bool active = true;
};

struct SessionIdentity
{
    std::string sessionId;
    bool active = true;
    bool expired = false;
    bool revoked = false;
};

struct CredentialIdentity
{
    std::string credentialId;
    bool active = true;
    bool expired = false;
    bool revoked = false;
};

struct RequestSecurityContext
{
    std::string requestId;
    std::string correlationId;
    AuthenticationState authenticationState = AuthenticationState::Anonymous;
    ActorIdentity actor;
    std::optional<DeviceIdentity> device;
    std::optional<SessionIdentity> session;
    std::optional<CredentialIdentity> credential;
    std::vector<PermissionGrant> grants;
    PermissionGrantResolutionState permissionGrantResolution =
        PermissionGrantResolutionState::NotRequired;

    bool authenticated() const
    {
        return authenticationState == AuthenticationState::Authenticated &&
            actor.active &&
            !actor.actorId.empty() &&
            (!device.has_value() || device->active) &&
            (!session.has_value() ||
                (session->active && !session->expired && !session->revoked)) &&
            (!credential.has_value() ||
                (credential->active &&
                 !credential->expired &&
                 !credential->revoked));
    }
};
