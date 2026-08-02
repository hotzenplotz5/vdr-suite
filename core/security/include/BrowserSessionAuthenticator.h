#pragma once

#include "BrowserSessionCredentialRepository.h"
#include "SecurityIdentity.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class BrowserSessionAuthenticator
{
public:
    BrowserSessionAuthenticator(
        const BrowserSessionCredentialRepository& repository,
        const SecurityPermissionGrantRepository& grantRepository,
        std::string cookieName = "vdr_suite_session",
        std::string csrfHeaderName = "X-CSRF-Token")
        : BrowserSessionAuthenticator(
              repository,
              grantRepository,
              0,
              60,
              std::move(cookieName),
              std::move(csrfHeaderName))
    {
    }

    BrowserSessionAuthenticator(
        const BrowserSessionCredentialRepository& repository,
        const SecurityPermissionGrantRepository& grantRepository,
        int idleTimeoutSeconds,
        int lastSeenWriteIntervalSeconds,
        std::string cookieName = "vdr_suite_session",
        std::string csrfHeaderName = "X-CSRF-Token")
        : repository_(repository),
          grantRepository_(grantRepository),
          idleTimeoutSeconds_(idleTimeoutSeconds),
          lastSeenWriteIntervalSeconds_(lastSeenWriteIntervalSeconds),
          cookieName_(std::move(cookieName)),
          csrfHeaderName_(std::move(csrfHeaderName))
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

        std::string cookieValue;
        const CookieLookupResult lookup =
            findCookieValue(headers, cookieValue);
        if (lookup == CookieLookupResult::Missing)
        {
            return context;
        }
        if (lookup == CookieLookupResult::Invalid)
        {
            context.authenticationState = AuthenticationState::Invalid;
            wipe(cookieValue);
            return context;
        }

        std::string tokenId;
        std::string sessionSecret;
        if (!parseSessionToken(cookieValue, tokenId, sessionSecret))
        {
            context.authenticationState = AuthenticationState::Invalid;
            wipe(cookieValue);
            return context;
        }
        wipe(cookieValue);

        const bool validIdlePolicy = idlePolicyValid();
        const auto record = repository_.findResolvedByTokenId(
            tokenId,
            validIdlePolicy ? idleTimeoutSeconds_ : 0);
        const bool secretAccepted =
            record.has_value() &&
            verifySecret(sessionSecret, record->sessionSecretHash);
        wipe(sessionSecret);

        if (!secretAccepted)
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        populateIdentity(context, *record);
        if (!validIdlePolicy)
        {
            context.authenticationState = AuthenticationState::Authenticated;
            context.permissionGrantResolution =
                PermissionGrantResolutionState::Unavailable;
            return context;
        }
        if (!record->active || record->revoked)
        {
            context.authenticationState = AuthenticationState::Revoked;
            context.session->active = false;
            context.session->revoked = true;
            context.credential->active = false;
            context.credential->revoked = true;
            return context;
        }
        if (record->expired)
        {
            context.authenticationState = AuthenticationState::Expired;
            context.session->expired = true;
            context.credential->expired = true;
            return context;
        }
        if (record->idleExpired)
        {
            context.authenticationState = AuthenticationState::Expired;
            context.session->expired = true;
            return context;
        }

        context.authenticationState = AuthenticationState::Authenticated;

        if (idleTimeoutSeconds_ > 0)
        {
            const auto activityResult = repository_.touchLastSeenIfDue(
                tokenId,
                lastSeenWriteIntervalSeconds_);
            if (!activityResult.has_value())
            {
                context.permissionGrantResolution =
                    PermissionGrantResolutionState::Unavailable;
                return context;
            }
        }

        auto grantResolution =
            grantRepository_.findActiveGrantsForActor(record->actorId);
        if (!grantResolution.available)
        {
            context.permissionGrantResolution =
                PermissionGrantResolutionState::Unavailable;
            return context;
        }

        context.permissionGrantResolution =
            PermissionGrantResolutionState::Resolved;
        context.grants = std::move(grantResolution.grants);
        return context;
    }

    bool verifyCsrf(
        const std::map<std::string, std::string>& headers) const
    {
        if (!idlePolicyValid())
        {
            return false;
        }

        std::string cookieValue;
        if (findCookieValue(headers, cookieValue) !=
            CookieLookupResult::Found)
        {
            return false;
        }

        std::string tokenId;
        std::string sessionSecret;
        if (!parseSessionToken(cookieValue, tokenId, sessionSecret))
        {
            wipe(cookieValue);
            return false;
        }
        wipe(cookieValue);

        const auto record = repository_.findResolvedByTokenId(
            tokenId,
            idleTimeoutSeconds_);
        const bool sessionAccepted =
            record.has_value() &&
            record->active &&
            !record->expired &&
            !record->idleExpired &&
            !record->revoked &&
            verifySecret(sessionSecret, record->sessionSecretHash);
        wipe(sessionSecret);
        if (!sessionAccepted)
        {
            return false;
        }

        std::string csrfSecret =
            headerValue(headers, csrfHeaderName_);
        if (!safeSecret(csrfSecret))
        {
            wipe(csrfSecret);
            return false;
        }

        const bool csrfAccepted =
            verifySecret(csrfSecret, record->csrfSecretHash);
        wipe(csrfSecret);
        return csrfAccepted;
    }

    bool hasSessionCookie(
        const std::map<std::string, std::string>& headers) const
    {
        std::string value;
        const CookieLookupResult result =
            findCookieValue(headers, value);
        wipe(value);
        return result != CookieLookupResult::Missing;
    }

private:
    enum class CookieLookupResult
    {
        Missing,
        Found,
        Invalid
    };

    bool idlePolicyValid() const
    {
        return lastSeenWriteIntervalSeconds_ > 0 &&
            (idleTimeoutSeconds_ == 0 ||
             (idleTimeoutSeconds_ >= 300 &&
              idleTimeoutSeconds_ <= 86400));
    }

    static std::string lowerAscii(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });
        return value;
    }

    static std::string trim(std::string value)
    {
        const auto notSpace = [](unsigned char character)
        {
            return !std::isspace(character);
        };

        value.erase(
            value.begin(),
            std::find_if(value.begin(), value.end(), notSpace));
        value.erase(
            std::find_if(value.rbegin(), value.rend(), notSpace).base(),
            value.end());
        return value;
    }

    static std::string headerValue(
        const std::map<std::string, std::string>& headers,
        const std::string& wantedName)
    {
        const std::string normalizedWanted = lowerAscii(wantedName);
        for (const auto& header : headers)
        {
            if (lowerAscii(header.first) == normalizedWanted)
            {
                return header.second;
            }
        }
        return "";
    }

    CookieLookupResult findCookieValue(
        const std::map<std::string, std::string>& headers,
        std::string& value) const
    {
        value.clear();
        const std::string cookieHeader = headerValue(headers, "Cookie");
        if (cookieHeader.empty())
        {
            return CookieLookupResult::Missing;
        }
        if (cookieHeader.size() > 8192)
        {
            return CookieLookupResult::Invalid;
        }

        std::size_t position = 0;
        bool found = false;
        while (position <= cookieHeader.size())
        {
            const std::size_t separator = cookieHeader.find(';', position);
            const std::size_t length = separator == std::string::npos
                ? cookieHeader.size() - position
                : separator - position;
            const std::string item =
                trim(cookieHeader.substr(position, length));

            if (!item.empty())
            {
                const std::size_t equals = item.find('=');
                if (equals == std::string::npos)
                {
                    return CookieLookupResult::Invalid;
                }

                const std::string name = trim(item.substr(0, equals));
                const std::string candidate = item.substr(equals + 1);
                if (name == cookieName_)
                {
                    if (found || candidate.empty())
                    {
                        return CookieLookupResult::Invalid;
                    }
                    value = candidate;
                    found = true;
                }
            }

            if (separator == std::string::npos)
            {
                break;
            }
            position = separator + 1;
        }

        return found
            ? CookieLookupResult::Found
            : CookieLookupResult::Missing;
    }

    static bool safeTokenId(const std::string& value)
    {
        if (value.empty() || value.size() > 128)
        {
            return false;
        }

        return std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' ||
                    character == '_';
            });
    }

    static bool safeSecret(const std::string& value)
    {
        if (value.size() < 32 || value.size() > 256)
        {
            return false;
        }

        return std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' ||
                    character == '_';
            });
    }

    static bool parseSessionToken(
        const std::string& value,
        std::string& tokenId,
        std::string& secret)
    {
        tokenId.clear();
        secret.clear();

        const std::size_t separator = value.find('.');
        if (separator == std::string::npos ||
            value.find('.', separator + 1) != std::string::npos)
        {
            return false;
        }

        tokenId = value.substr(0, separator);
        secret = value.substr(separator + 1);
        return safeTokenId(tokenId) && safeSecret(secret);
    }

    static bool constantTimeEqual(
        const std::string& first,
        const std::string& second)
    {
        const std::size_t maximum =
            std::max(first.size(), second.size());
        unsigned char difference = static_cast<unsigned char>(
            first.size() ^ second.size());

        for (std::size_t index = 0; index < maximum; ++index)
        {
            const unsigned char firstCharacter = index < first.size()
                ? static_cast<unsigned char>(first[index])
                : 0;
            const unsigned char secondCharacter = index < second.size()
                ? static_cast<unsigned char>(second[index])
                : 0;
            difference |= static_cast<unsigned char>(
                firstCharacter ^ secondCharacter);
        }
        return difference == 0;
    }

    static bool verifySecret(
        const std::string& secret,
        const std::string& secretHash)
    {
        if (!BrowserSessionCredentialRepository::supportsSecretHash(
                secretHash))
        {
            return false;
        }

        crypt_data data{};
        char* verified = crypt_r(
            secret.c_str(),
            secretHash.c_str(),
            &data);
        return verified != nullptr &&
            constantTimeEqual(verified, secretHash);
    }

    static void populateIdentity(
        RequestSecurityContext& context,
        const StoredBrowserSessionCredential& record)
    {
        context.actor.actorId = record.actorId;
        context.actor.type = ActorType::User;
        context.actor.active = true;
        context.device = DeviceIdentity{record.deviceId, true};
        context.session = SessionIdentity{
            record.sessionId,
            true,
            false,
            false};
        context.credential = CredentialIdentity{
            record.credentialId,
            true,
            false,
            false};
    }

    static void wipe(std::string& value)
    {
        std::fill(value.begin(), value.end(), '\0');
        value.clear();
    }

    const BrowserSessionCredentialRepository& repository_;
    const SecurityPermissionGrantRepository& grantRepository_;
    int idleTimeoutSeconds_ = 0;
    int lastSeenWriteIntervalSeconds_ = 60;
    std::string cookieName_;
    std::string csrfHeaderName_;
};
