#pragma once

#include "SecurityIdentity.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

enum class SecurityMode
{
    LegacyBasicCompatibility,
    Enforced
};

struct SecurityConfiguration
{
    SecurityMode mode = SecurityMode::LegacyBasicCompatibility;
    std::string expectedAuthorizationHeader =
        "Basic YWRtaW46dmRyLXN1aXRl";
    std::string actorId = "legacy-local-web";
    std::string actorDisplayName = "Legacy local web client";
    std::string deviceId = "legacy-browser";
    std::string sessionId = "legacy-basic-session";
    std::string credentialId = "legacy-basic-credential";
    std::vector<PermissionGrant> grants = {
        PermissionGrant{"*", "*"}
    };

    static SecurityConfiguration fromEnvironment()
    {
        SecurityConfiguration configuration;

        const auto environmentValue = [](
            const char* name,
            const std::string& fallback)
        {
            const char* value = std::getenv(name);
            return value == nullptr || value[0] == '\0'
                ? fallback
                : std::string(value);
        };

        const std::string mode =
            environmentValue(
                "VDR_SUITE_SECURITY_MODE",
                "legacy-basic");

        if (mode == "enforced")
        {
            configuration.mode = SecurityMode::Enforced;
            configuration.expectedAuthorizationHeader.clear();
            configuration.grants.clear();
        }

        configuration.expectedAuthorizationHeader =
            environmentValue(
                "VDR_SUITE_BASIC_AUTH",
                configuration.expectedAuthorizationHeader);
        configuration.actorId =
            environmentValue(
                "VDR_SUITE_LEGACY_BASIC_ACTOR_ID",
                configuration.actorId);
        configuration.actorDisplayName =
            environmentValue(
                "VDR_SUITE_LEGACY_BASIC_ACTOR_DISPLAY_NAME",
                configuration.actorDisplayName);
        configuration.deviceId =
            environmentValue(
                "VDR_SUITE_LEGACY_BASIC_DEVICE_ID",
                configuration.deviceId);
        configuration.sessionId =
            environmentValue(
                "VDR_SUITE_LEGACY_BASIC_SESSION_ID",
                configuration.sessionId);
        configuration.credentialId =
            environmentValue(
                "VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID",
                configuration.credentialId);

        const char* configuredGrants =
            std::getenv("VDR_SUITE_LEGACY_BASIC_PERMISSIONS");

        if (configuredGrants != nullptr)
        {
            configuration.grants.clear();
            std::stringstream stream(configuredGrants);
            std::string item;

            const auto trim = [](std::string value)
            {
                const auto notSpace = [](unsigned char character)
                {
                    return !std::isspace(character);
                };

                value.erase(
                    value.begin(),
                    std::find_if(
                        value.begin(),
                        value.end(),
                        notSpace));
                value.erase(
                    std::find_if(
                        value.rbegin(),
                        value.rend(),
                        notSpace).base(),
                    value.end());
                return value;
            };

            while (std::getline(stream, item, ','))
            {
                item = trim(item);

                if (item.empty())
                {
                    continue;
                }

                PermissionGrant grant;
                const std::size_t separator = item.find('@');

                if (separator == std::string::npos)
                {
                    grant.permission = item;
                    grant.backendId = "*";
                }
                else
                {
                    grant.permission =
                        trim(item.substr(0, separator));
                    grant.backendId =
                        trim(item.substr(separator + 1));
                }

                if (!grant.permission.empty() &&
                    !grant.backendId.empty())
                {
                    configuration.grants.push_back(grant);
                }
            }
        }

        return configuration;
    }
};
