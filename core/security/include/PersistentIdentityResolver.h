#pragma once

#include "SecurityIdentityRepository.h"

class PersistentIdentityResolver
{
public:
    explicit PersistentIdentityResolver(
        SecurityIdentityRepository& repository)
        : repository_(repository)
    {
    }

    RequestSecurityContext resolve(
        RequestSecurityContext context) const
    {
        if (context.authenticationState !=
            AuthenticationState::Authenticated)
        {
            return context;
        }

        const auto actor = repository_.findActor(
            context.actor.actorId);
        if (!actor.has_value())
        {
            context.authenticationState =
                AuthenticationState::Invalid;
            return context;
        }

        context.actor.type = actor->type;
        context.actor.displayName = actor->displayName;
        context.actor.active = actor->active && !actor->revoked;
        if (!context.actor.active)
        {
            context.authenticationState =
                AuthenticationState::Revoked;
            return context;
        }

        if (context.device.has_value())
        {
            const auto device = repository_.findDevice(
                context.device->deviceId);
            if (!device.has_value() ||
                device->actorId != context.actor.actorId)
            {
                context.authenticationState =
                    AuthenticationState::Invalid;
                return context;
            }

            context.device->active =
                device->active && !device->revoked;
            if (!context.device->active)
            {
                context.authenticationState =
                    AuthenticationState::Revoked;
                return context;
            }
        }

        if (context.credential.has_value())
        {
            const auto credential = repository_.findCredential(
                context.credential->credentialId);
            if (!credential.has_value() ||
                credential->actorId != context.actor.actorId)
            {
                context.authenticationState =
                    AuthenticationState::Invalid;
                return context;
            }

            context.credential->active = credential->active;
            context.credential->expired = credential->expired;
            context.credential->revoked = credential->revoked;
            if (credential->revoked || !credential->active)
            {
                context.authenticationState =
                    AuthenticationState::Revoked;
                return context;
            }
            if (credential->expired)
            {
                context.authenticationState =
                    AuthenticationState::Expired;
                return context;
            }
        }

        if (context.session.has_value())
        {
            const auto session = repository_.findSession(
                context.session->sessionId);
            const std::string deviceId = context.device.has_value()
                ? context.device->deviceId
                : std::string();
            if (!session.has_value() ||
                session->actorId != context.actor.actorId ||
                session->deviceId != deviceId)
            {
                context.authenticationState =
                    AuthenticationState::Invalid;
                return context;
            }

            context.session->active = session->active;
            context.session->expired = session->expired;
            context.session->revoked = session->revoked;
            if (session->revoked || !session->active)
            {
                context.authenticationState =
                    AuthenticationState::Revoked;
                return context;
            }
            if (session->expired)
            {
                context.authenticationState =
                    AuthenticationState::Expired;
                return context;
            }
        }

        return context;
    }

private:
    SecurityIdentityRepository& repository_;
};
