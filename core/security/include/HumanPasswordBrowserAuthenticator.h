#pragma once

#include "CredentialVerifierRepository.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentity.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <crypt.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <string>

class HumanPasswordBrowserAuthenticator
{
public:
    HumanPasswordBrowserAuthenticator(
        const CredentialVerifierRepository& verifierRepository,
        const SecurityIdentityRepository& identityRepository,
        const HumanAccountRepository& accountRepository,
        SecurityIdentityProvisioningRepository& provisioningRepository)
        : verifierRepository_(verifierRepository),
          identityRepository_(identityRepository),
          accountRepository_(accountRepository),
          provisioningRepository_(provisioningRepository)
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
        if (!verifier.has_value())
        {
            secureWipe(password);
            return context;
        }

        const auto credential =
            identityRepository_.findCredential(verifier->credentialId);
        if (!credential.has_value())
        {
            secureWipe(password);
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        if (credential->credentialType != "human-password")
        {
            secureWipe(password);
            return context;
        }

        const bool accepted =
            verifyPassword(password, verifier->passwordHash);
        secureWipe(password);
        if (!accepted)
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        const auto actor =
            identityRepository_.findActor(credential->actorId);
        if (!actor.has_value() || actor->type != ActorType::User)
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        context.actor.actorId = actor->actorId;
        context.actor.type = actor->type;
        context.actor.displayName = actor->displayName;
        context.actor.active = actor->active && !actor->revoked;
        context.credential = CredentialIdentity{
            credential->credentialId,
            credential->active,
            credential->expired,
            credential->revoked};

        if (!context.actor.active ||
            !credential->active ||
            credential->revoked)
        {
            context.authenticationState = AuthenticationState::Revoked;
            return context;
        }
        if (credential->expired)
        {
            context.authenticationState = AuthenticationState::Expired;
            return context;
        }

        const HumanAccountLookupResult account =
            accountRepository_.findByActorId(actor->actorId);
        if (account.status != HumanAccountRepositoryStatus::ok)
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }
        if (!account.account.active)
        {
            context.authenticationState = AuthenticationState::Revoked;
            return context;
        }

        const std::string deviceId =
            loginDeviceId(account.account.accountId);
        if (deviceId.empty())
        {
            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        if (!provisioningRepository_.ensureHumanBrowserDevice(
                actor->actorId,
                deviceId,
                "Human Account browser login"))
        {
            const auto device =
                identityRepository_.findDevice(deviceId);
            if (device.has_value() &&
                device->actorId == actor->actorId &&
                (!device->active || device->revoked))
            {
                context.device =
                    DeviceIdentity{deviceId, false};
                context.authenticationState =
                    AuthenticationState::Revoked;
                return context;
            }

            context.authenticationState = AuthenticationState::Invalid;
            return context;
        }

        context.device = DeviceIdentity{deviceId, true};
        context.authenticationState =
            AuthenticationState::Authenticated;
        return context;
    }

    static std::string loginDeviceId(
        const std::string& accountId)
    {
        constexpr const char* Prefix = "human-browser-";
        constexpr std::size_t MaximumIdentifierBytes = 128U;
        const std::string result =
            std::string(Prefix) + accountId;

        if (accountId.empty() ||
            result.size() > MaximumIdentifierBytes)
        {
            return {};
        }

        for (const unsigned char character : result)
        {
            if (std::isalnum(character) == 0 &&
                character != '-' &&
                character != '_' &&
                character != '.' &&
                character != ':')
            {
                return {};
            }
        }
        return result;
    }

private:
    static void secureWipe(std::string& value) noexcept
    {
        volatile char* bytes = value.empty()
            ? nullptr
            : const_cast<volatile char*>(value.data());
        for (std::size_t index = 0; index < value.size(); ++index)
        {
            bytes[index] = 0;
        }
        value.clear();
    }

    template <typename Value>
    static void secureWipeObject(Value& value) noexcept
    {
        volatile unsigned char* bytes =
            reinterpret_cast<volatile unsigned char*>(&value);
        for (std::size_t index = 0;
             index < sizeof(Value);
             ++index)
        {
            bytes[index] = 0;
        }
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

    static std::string headerValue(
        const std::map<std::string, std::string>& headers,
        const std::string& wantedName)
    {
        const std::string normalizedWanted =
            lowerAscii(wantedName);
        for (const auto& header : headers)
        {
            if (lowerAscii(header.first) == normalizedWanted)
            {
                return header.second;
            }
        }
        return {};
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
        if (encoded.empty() ||
            encoded.size() > 8192U ||
            encoded.size() % 4U != 0U)
        {
            return false;
        }

        decoded.reserve((encoded.size() / 4U) * 3U);
        for (std::size_t offset = 0;
             offset < encoded.size();
             offset += 4U)
        {
            const bool finalBlock =
                offset + 4U == encoded.size();
            const int first = base64Value(
                static_cast<unsigned char>(encoded[offset]));
            const int second = base64Value(
                static_cast<unsigned char>(encoded[offset + 1U]));
            if (first < 0 || second < 0)
            {
                secureWipe(decoded);
                return false;
            }

            const char thirdCharacter =
                encoded[offset + 2U];
            const char fourthCharacter =
                encoded[offset + 3U];
            const int third = thirdCharacter == '='
                ? -2
                : base64Value(
                    static_cast<unsigned char>(
                        thirdCharacter));
            const int fourth = fourthCharacter == '='
                ? -2
                : base64Value(
                    static_cast<unsigned char>(
                        fourthCharacter));

            if (third == -1 || fourth == -1)
            {
                secureWipe(decoded);
                return false;
            }

            decoded.push_back(static_cast<char>(
                (first << 2) | (second >> 4)));

            if (third == -2)
            {
                if (!finalBlock ||
                    fourth != -2 ||
                    (second & 0x0f) != 0)
                {
                    secureWipe(decoded);
                    return false;
                }
                continue;
            }

            decoded.push_back(static_cast<char>(
                ((second & 0x0f) << 4) |
                (third >> 2)));

            if (fourth == -2)
            {
                if (!finalBlock ||
                    (third & 0x03) != 0)
                {
                    secureWipe(decoded);
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
        secureWipe(password);

        if (authorization.size() < 7U ||
            lowerAscii(authorization.substr(0, 5)) != "basic" ||
            authorization[5] != ' ')
        {
            return false;
        }

        std::string decoded;
        if (!decodeBase64(
                authorization.substr(6),
                decoded))
        {
            return false;
        }

        const std::size_t separator = decoded.find(':');
        if (separator == std::string::npos)
        {
            secureWipe(decoded);
            return false;
        }

        loginName = decoded.substr(0, separator);
        password = decoded.substr(separator + 1U);
        const bool safe =
            safeCredentialPart(loginName, 128U) &&
            safeCredentialPart(password, 1024U);
        secureWipe(decoded);

        if (!safe)
        {
            loginName.clear();
            secureWipe(password);
        }
        return safe;
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
        for (std::size_t index = 0;
             index < first.size();
             ++index)
        {
            difference |= static_cast<unsigned char>(
                first[index] ^ second[index]);
        }
        return difference == 0;
    }

    static bool supportsPasswordHash(
        const std::string& passwordHash)
    {
        return passwordHash.rfind("$y$", 0) == 0 ||
            passwordHash.rfind("$6$", 0) == 0;
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
        const bool accepted =
            verified != nullptr &&
            constantTimeEqual(verified, passwordHash);
        secureWipeObject(data);
        return accepted;
    }

    const CredentialVerifierRepository& verifierRepository_;
    const SecurityIdentityRepository& identityRepository_;
    const HumanAccountRepository& accountRepository_;
    SecurityIdentityProvisioningRepository& provisioningRepository_;
};
