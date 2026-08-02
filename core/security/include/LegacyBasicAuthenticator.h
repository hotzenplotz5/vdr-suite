#pragma once

#include "SecurityConfiguration.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <utility>

class LegacyBasicAuthenticator
{
public:
    explicit LegacyBasicAuthenticator(
        SecurityConfiguration configuration)
        : configuration_(std::move(configuration))
    {
    }

    RequestSecurityContext authenticate(
        const std::map<std::string, std::string>& headers,
        const std::string& requestId,
        const std::string& correlationId) const
    {
        RequestSecurityContext context;
        context.requestId = requestId;
        context.correlationId = correlationId;

        const std::string authorization =
            headerValue(headers, "Authorization");

        if (authorization.empty())
        {
            return context;
        }

        if (configuration_.expectedAuthorizationHeader.empty() ||
            authorization !=
                configuration_.expectedAuthorizationHeader)
        {
            context.authenticationState =
                AuthenticationState::Invalid;
            return context;
        }

        context.authenticationState =
            AuthenticationState::Authenticated;
        context.actor.actorId = configuration_.actorId;
        context.actor.type = ActorType::User;
        context.actor.displayName =
            configuration_.actorDisplayName;
        context.actor.active = true;
        context.device =
            DeviceIdentity{configuration_.deviceId, true};
        context.session = SessionIdentity{
            configuration_.sessionId,
            true,
            false,
            false};
        context.credential = CredentialIdentity{
            configuration_.credentialId,
            true,
            false,
            false};
        context.grants = configuration_.grants;
        return context;
    }

private:
    static std::string lowerAscii(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character));
            });
        return value;
    }

    static std::string headerValue(
        const std::map<std::string, std::string>& headers,
        const std::string& wantedName)
    {
        const std::string normalizedWanted =
            lowerAscii(wantedName);

        for (const auto& header : headers)
        {
            if (lowerAscii(header.first) ==
                normalizedWanted)
            {
                return header.second;
            }
        }

        return "";
    }

    SecurityConfiguration configuration_;
};
