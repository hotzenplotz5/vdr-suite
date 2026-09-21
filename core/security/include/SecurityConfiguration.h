#pragma once

#include "BrowserSessionIssuanceService.h"
#include "SecurityIdentity.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

enum class SecurityMode
{
    LegacyBasicCompatibility,
    Enforced
};

struct BrowserSessionLifetimeConfiguration
{
    int seconds = BrowserSessionIssuanceService::DefaultLifetimeSeconds;
    bool configuredValueValid = true;

    bool valid() const
    {
        return configuredValueValid &&
            seconds >= BrowserSessionIssuanceService::MinimumLifetimeSeconds &&
            seconds <= BrowserSessionIssuanceService::MaximumLifetimeSeconds;
    }
};

struct BrowserSessionConcurrencyConfiguration
{
    std::size_t maximumActivePerActor = 0;
    bool configuredValueValid = true;

    bool valid() const
    {
        return configuredValueValid &&
            maximumActivePerActor <=
                BrowserSessionIssuanceService::MaximumActiveSessionsPerActor;
    }
};

struct BrowserSessionIdleConfiguration
{
    static constexpr int MinimumTimeoutSeconds = 300;
    static constexpr int MaximumTimeoutSeconds = 86400;
    static constexpr int LastSeenWriteIntervalSeconds = 60;

    int timeoutSeconds = 0;
    bool configuredValueValid = true;

    bool enabled() const
    {
        return timeoutSeconds > 0;
    }

    bool valid() const
    {
        return configuredValueValid &&
            (timeoutSeconds == 0 ||
             (timeoutSeconds >= MinimumTimeoutSeconds &&
              timeoutSeconds <= MaximumTimeoutSeconds));
    }
};

struct BrowserSessionRetentionConfiguration
{
    static constexpr int MinimumRetentionSeconds = 86400;
    static constexpr int MaximumRetentionSeconds = 31536000;
    static constexpr std::size_t BatchSize = 256;

    int seconds = 0;
    bool configuredValueValid = true;

    bool enabled() const
    {
        return seconds > 0;
    }

    bool valid() const
    {
        return configuredValueValid &&
            (seconds == 0 ||
             (seconds >= MinimumRetentionSeconds &&
              seconds <= MaximumRetentionSeconds));
    }
};

struct ManagedBasicConfiguration
{
    std::string username;
    std::string passwordHash;
    std::string actorId = "phase62-managed-admin";
    std::string actorDisplayName = "Phase 62 managed administrator";
    std::string deviceId = "phase62-managed-admin-client";
    std::string sessionId = "phase62-managed-admin-basic-session";
    std::string credentialId = "phase62-managed-admin-credential";
    std::vector<PermissionGrant> grants;

    bool hasAnyConfiguration() const
    {
        return !username.empty() || !passwordHash.empty();
    }

    bool complete() const
    {
        return !username.empty() &&
            !passwordHash.empty() &&
            !actorId.empty() &&
            !actorDisplayName.empty() &&
            !deviceId.empty() &&
            !sessionId.empty() &&
            !credentialId.empty();
    }
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
    ManagedBasicConfiguration managedBasic;
    BrowserSessionLifetimeConfiguration browserSessionLifetime;
    BrowserSessionConcurrencyConfiguration browserSessionConcurrency;
    BrowserSessionIdleConfiguration browserSessionIdle;
    BrowserSessionRetentionConfiguration browserSessionRetention;

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
            configuration.grants = parseGrants(configuredGrants);
        }

        configuration.managedBasic.username =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_USERNAME",
                "");
        configuration.managedBasic.passwordHash =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH",
                "");
        configuration.managedBasic.actorId =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_ACTOR_ID",
                configuration.managedBasic.actorId);
        configuration.managedBasic.actorDisplayName =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_ACTOR_DISPLAY_NAME",
                configuration.managedBasic.actorDisplayName);
        configuration.managedBasic.deviceId =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_DEVICE_ID",
                configuration.managedBasic.deviceId);
        configuration.managedBasic.sessionId =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_SESSION_ID",
                configuration.managedBasic.sessionId);
        configuration.managedBasic.credentialId =
            environmentValue(
                "VDR_SUITE_MANAGED_BASIC_CREDENTIAL_ID",
                configuration.managedBasic.credentialId);

        const char* managedGrants =
            std::getenv("VDR_SUITE_MANAGED_BASIC_PERMISSIONS");
        if (managedGrants != nullptr)
        {
            configuration.managedBasic.grants =
                parseGrants(managedGrants);
        }

        const char* browserSessionLifetime =
            std::getenv(
                "VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS");
        if (browserSessionLifetime != nullptr)
        {
            configuration.browserSessionLifetime.configuredValueValid =
                parseBrowserSessionLifetime(
                    browserSessionLifetime,
                    configuration.browserSessionLifetime.seconds);
        }

        const char* browserSessionMaximum =
            std::getenv(
                "VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR");
        if (browserSessionMaximum != nullptr)
        {
            configuration.browserSessionConcurrency.configuredValueValid =
                parseBrowserSessionMaximum(
                    browserSessionMaximum,
                    configuration.browserSessionConcurrency
                        .maximumActivePerActor);
        }

        const char* browserSessionIdleTimeout =
            std::getenv(
                "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS");
        if (browserSessionIdleTimeout != nullptr)
        {
            configuration.browserSessionIdle.configuredValueValid =
                parseBrowserSessionIdleTimeout(
                    browserSessionIdleTimeout,
                    configuration.browserSessionIdle.timeoutSeconds);
        }

        const char* browserSessionRetention =
            std::getenv(
                "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS");
        if (browserSessionRetention != nullptr)
        {
            configuration.browserSessionRetention.configuredValueValid =
                parseBrowserSessionRetention(
                    browserSessionRetention,
                    configuration.browserSessionRetention.seconds);
        }

        return configuration;
    }

private:
    static std::string trim(std::string value)
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
    }

    static bool parseBoundedUnsignedDecimal(
        const std::string& configuredValue,
        std::size_t maximum,
        std::size_t& result)
    {
        if (configuredValue.empty())
        {
            return false;
        }

        std::size_t parsed = 0;
        for (const unsigned char character : configuredValue)
        {
            if (!std::isdigit(character))
            {
                return false;
            }

            const std::size_t digit =
                static_cast<std::size_t>(character - '0');
            if (parsed > (maximum - digit) / 10)
            {
                return false;
            }
            parsed = parsed * 10 + digit;
        }

        result = parsed;
        return true;
    }

    static bool parseBrowserSessionLifetime(
        const std::string& configuredValue,
        int& result)
    {
        std::size_t parsed = 0;
        if (!parseBoundedUnsignedDecimal(
                configuredValue,
                static_cast<std::size_t>(
                    BrowserSessionIssuanceService::MaximumLifetimeSeconds),
                parsed) ||
            parsed < static_cast<std::size_t>(
                BrowserSessionIssuanceService::MinimumLifetimeSeconds))
        {
            return false;
        }

        result = static_cast<int>(parsed);
        return true;
    }

    static bool parseBrowserSessionMaximum(
        const std::string& configuredValue,
        std::size_t& result)
    {
        return parseBoundedUnsignedDecimal(
            configuredValue,
            BrowserSessionIssuanceService::MaximumActiveSessionsPerActor,
            result);
    }

    static bool parseBrowserSessionIdleTimeout(
        const std::string& configuredValue,
        int& result)
    {
        std::size_t parsed = 0;
        if (!parseBoundedUnsignedDecimal(
                configuredValue,
                static_cast<std::size_t>(
                    BrowserSessionIdleConfiguration::MaximumTimeoutSeconds),
                parsed) ||
            (parsed != 0 &&
             parsed < static_cast<std::size_t>(
                 BrowserSessionIdleConfiguration::MinimumTimeoutSeconds)))
        {
            return false;
        }

        result = static_cast<int>(parsed);
        return true;
    }

    static bool parseBrowserSessionRetention(
        const std::string& configuredValue,
        int& result)
    {
        std::size_t parsed = 0;
        if (!parseBoundedUnsignedDecimal(
                configuredValue,
                static_cast<std::size_t>(
                    BrowserSessionRetentionConfiguration::MaximumRetentionSeconds),
                parsed) ||
            (parsed != 0 &&
             parsed < static_cast<std::size_t>(
                 BrowserSessionRetentionConfiguration::MinimumRetentionSeconds)))
        {
            return false;
        }

        result = static_cast<int>(parsed);
        return true;
    }

    static std::vector<PermissionGrant> parseGrants(
        const std::string& configuredGrants)
    {
        std::vector<PermissionGrant> grants;
        std::stringstream stream(configuredGrants);
        std::string item;

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
                grant.permission = trim(item.substr(0, separator));
                grant.backendId = trim(item.substr(separator + 1));
            }

            if (!grant.permission.empty() &&
                !grant.backendId.empty())
            {
                grants.push_back(grant);
            }
        }

        return grants;
    }
};