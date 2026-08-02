#pragma once

#include "CredentialVerifierRepository.h"
#include "SecurityConfiguration.h"

#include <crypt.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <string>
#include <utility>

class ManagedBasicAuthenticator
{
public:
    ManagedBasicAuthenticator(
        ManagedBasicConfiguration configuration,
        const CredentialVerifierRepository& verifierRepository)
        : configuration_(std::move(configuration)),
          verifierRepository_(verifierRepository)
    {
        configuration_.passwordHash.clear();
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

        std::string loginName;
        std::string password;
        if (!parseBasicAuthorization(
                authorization,
                loginName,
                password))
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        const auto verifier = verifierRepository_.findByLogin(loginName);
        const bool accepted =
            verifier.has_value() &&
            verifier->credentialId == configuration_.credentialId &&
            verifyPassword(password, verifier->passwordHash);
        std::fill(password.begin(), password.end(), '\0');

        if (!accepted)
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        context.authenticationState = AuthenticationState::Authenticated;
        context.actor.actorId = configuration_.actorId;
        context.actor.type = ActorType::User;
        context.actor.displayName = configuration_.actorDisplayName;
        context.actor.active = true;
        context.device = DeviceIdentity{
            configuration_.deviceId,
            true};
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

    static bool supportsPasswordHash(const std::string& passwordHash)
    {
        return passwordHash.rfind("$y$", 0) == 0 ||
            passwordHash.rfind("$6$", 0) == 0;
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
                return static_cast<char>(std::tolower(character));
            });
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

    static int base64Value(unsigned char character)
    {
        if (character >= 'A' && character <= 'Z')
        {
            return character - 'A';
        }
        if (character >= 'a' && character <= 'z')
        {
            return character - 'a' + 26;
        }
        if (character >= '0' && character <= '9')
        {
            return character - '0' + 52;
        }
        if (character == '+') return 62;
        if (character == '/') return 63;
        return -1;
    }

    static bool decodeBase64(
        const std::string& encoded,
        std::string& decoded)
    {
        decoded.clear();
        if (encoded.empty() || encoded.size() > 8192 ||
            encoded.size() % 4 != 0)
        {
            return false;
        }

        decoded.reserve((encoded.size() / 4) * 3);
        for (std::size_t offset = 0;
             offset < encoded.size();
             offset += 4)
        {
            const bool finalBlock = offset + 4 == encoded.size();
            const int first = base64Value(
                static_cast<unsigned char>(encoded[offset]));
            const int second = base64Value(
                static_cast<unsigned char>(encoded[offset + 1]));
            if (first < 0 || second < 0)
            {
                return false;
            }

            const char thirdCharacter = encoded[offset + 2];
            const char fourthCharacter = encoded[offset + 3];
            const int third = thirdCharacter == '='
                ? -2
                : base64Value(
                    static_cast<unsigned char>(thirdCharacter));
            const int fourth = fourthCharacter == '='
                ? -2
                : base64Value(
                    static_cast<unsigned char>(fourthCharacter));

            if (third == -1 || fourth == -1)
            {
                return false;
            }

            decoded.push_back(static_cast<char>(
                (first << 2) | (second >> 4)));

            if (third == -2)
            {
                if (!finalBlock || fourth != -2 ||
                    (second & 0x0f) != 0)
                {
                    return false;
                }
                continue;
            }

            decoded.push_back(static_cast<char>(
                ((second & 0x0f) << 4) | (third >> 2)));

            if (fourth == -2)
            {
                if (!finalBlock || (third & 0x03) != 0)
                {
                    return false;
                }
                continue;
            }

            decoded.push_back(static_cast<char>(
                ((third & 0x03) << 6) | fourth));
        }

        return true;
    }

    static bool safeCredentialPart(
        const std::string& value,
        std::size_t maximumLength)
    {
        if (value.empty() || value.size() > maximumLength)
        {
            return false;
        }

        return std::none_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return character == '\0' ||
                    character == '\r' ||
                    character == '\n';
            });
    }

    static bool parseBasicAuthorization(
        const std::string& authorization,
        std::string& loginName,
        std::string& password)
    {
        loginName.clear();
        password.clear();

        if (authorization.size() < 7 ||
            lowerAscii(authorization.substr(0, 5)) != "basic" ||
            authorization[5] != ' ')
        {
            return false;
        }

        std::string decoded;
        if (!decodeBase64(authorization.substr(6), decoded))
        {
            return false;
        }

        const std::size_t separator = decoded.find(':');
        if (separator == std::string::npos)
        {
            return false;
        }

        loginName = decoded.substr(0, separator);
        password = decoded.substr(separator + 1);
        return safeCredentialPart(loginName, 128) &&
            safeCredentialPart(password, 1024);
    }

    static bool constantTimeEqual(
        const std::string& first,
        const std::string& second)
    {
        if (first.size() != second.size())
        {
            return false;
        }

        unsigned char difference = 0;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            difference |= static_cast<unsigned char>(
                first[index] ^ second[index]);
        }
        return difference == 0;
    }

    static bool verifyPassword(
        const std::string& password,
        const std::string& passwordHash)
    {
        if (!supportsPasswordHash(passwordHash))
        {
            return false;
        }

        crypt_data data{};
        char* verified = crypt_r(
            password.c_str(),
            passwordHash.c_str(),
            &data);
        return verified != nullptr &&
            constantTimeEqual(verified, passwordHash);
    }

    ManagedBasicConfiguration configuration_;
    const CredentialVerifierRepository& verifierRepository_;
};
