#pragma once

#include "DeviceCredentialVerifierRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <string>

// MU.10D: proof of possession for a device-app credential issued by MU.10C.
// No Session, Browser cookie, pairing-token reuse or implicit permissions.
class DeviceCredentialAuthenticator
{
public:
    DeviceCredentialAuthenticator(
        const DeviceCredentialVerifierRepository& verifiers,
        const SecurityIdentityRepository& identities,
        const SecurityPermissionGrantRepository& grants)
        : verifiers_(verifiers), identities_(identities), grants_(grants) {}

    static constexpr const char* Scheme = "VDR-Suite-Device ";

    static bool hasDeviceAuthorization(
        const std::map<std::string, std::string>& headers)
    {
        const std::string authorization = headerValue(headers, "Authorization");
        return authorization.rfind("VDR-Suite-Device", 0U) == 0U;
    }

    RequestSecurityContext authenticate(
        const std::map<std::string, std::string>& headers,
        const std::string& requestId,
        const std::string& correlationId) const
    {
        RequestSecurityContext context;
        context.requestId = requestId;
        context.correlationId = correlationId;
        if (!hasDeviceAuthorization(headers))
            return context;

        context.authenticationState = AuthenticationState::Invalid;
        const std::string authorization = headerValue(headers, "Authorization");
        const std::string prefix(Scheme);
        if (authorization.rfind(prefix, 0U) != 0U ||
            authorization.size() > 512U)
            return context;

        const std::string material = authorization.substr(prefix.size());
        const std::size_t separator = material.find('.');
        if (separator == std::string::npos ||
            material.find('.', separator + 1U) != std::string::npos)
            return context;
        const std::string credentialId = material.substr(0U, separator);
        std::string secret = material.substr(separator + 1U);
        const bool wellFormed = safePart(credentialId, 128U, 1U) &&
            safePart(secret, 128U, 32U);
        if (!wellFormed)
        {
            wipe(secret);
            return context;
        }

        const auto verifier = verifiers_.findByCredentialId(credentialId);
        bool proved = false;
        if (verifier.has_value() &&
            verifier->verifierHash.rfind("$6$", 0U) == 0U &&
            verifier->verifierHash.size() <= 1024U)
        {
            crypt_data work{};
            const char* computed = crypt_r(
                secret.c_str(), verifier->verifierHash.c_str(), &work);
            if (computed != nullptr)
            {
                const std::size_t size = std::strlen(computed);
                if (size == verifier->verifierHash.size())
                {
                    unsigned char diff = 0U;
                    for (std::size_t index = 0; index < size; ++index)
                        diff |= static_cast<unsigned char>(
                            computed[index] ^ verifier->verifierHash[index]);
                    proved = diff == 0U;
                }
            }
            wipeBytes(&work, sizeof(work));
        }
        wipe(secret);
        if (!proved)
            return context;

        // Verify the canonical binding, never trust a verifier-only row
        // as an independent identity or authorization authority.
        const auto credential = identities_.findCredential(credentialId);
        const auto device = identities_.findDevice(verifier->deviceId);
        if (!credential.has_value() || !device.has_value() ||
            credential->credentialType != "device-app" ||
            credential->actorId != device->actorId)
            return context;
        const auto actor = identities_.findActor(device->actorId);
        if (!actor.has_value() || actor->type != ActorType::Service)
            return context;

        context.authenticationState = AuthenticationState::Authenticated;
        context.actor.actorId = actor->actorId;
        context.actor.type = ActorType::Service;
        context.actor.displayName = actor->displayName;
        context.device = DeviceIdentity{device->deviceId, true};
        context.credential = CredentialIdentity{credentialId, true, false, false};
        // The persistent resolver enforces actor/device/credential
        // revocation and expiry on every request before authorization.
        const auto resolution = grants_.findActiveGrantsForActor(actor->actorId);
        context.permissionGrantResolution = resolution.available
            ? PermissionGrantResolutionState::Resolved
            : PermissionGrantResolutionState::Unavailable;
        if (resolution.available)
            context.grants = resolution.grants;
        return context;
    }

private:
    static std::string headerValue(
        const std::map<std::string, std::string>& headers,
        const std::string& key)
    {
        for (const auto& header : headers)
        {
            if (header.first.size() != key.size())
                continue;
            bool same = true;
            for (std::size_t i = 0; i < key.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(header.first[i])) !=
                    std::tolower(static_cast<unsigned char>(key[i])))
                {
                    same = false;
                    break;
                }
            }
            if (same)
                return header.second;
        }
        return {};
    }

    static bool safePart(
        const std::string& value, std::size_t max, std::size_t min)
    {
        return value.size() >= min && value.size() <= max &&
            std::all_of(value.begin(), value.end(),
                [](unsigned char c) {
                    return std::isalnum(c) || c == '_' || c == '-';
                });
    }

    static void wipeBytes(void* address, std::size_t size) noexcept
    {
        volatile unsigned char* bytes =
            static_cast<volatile unsigned char*>(address);
        for (std::size_t i = 0; i < size; ++i)
            bytes[i] = 0U;
    }

    static void wipe(std::string& secret) noexcept
    {
        if (!secret.empty())
            wipeBytes(&secret[0], secret.size());
        secret.clear();
    }

    const DeviceCredentialVerifierRepository& verifiers_;
    const SecurityIdentityRepository& identities_;
    const SecurityPermissionGrantRepository& grants_;
};
