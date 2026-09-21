#include "MediaAccessGrantAuthenticator.h"

#include <crypt.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{

void wipe(std::string& value)
{
    std::fill(value.begin(), value.end(), '\0');
    value.clear();
}

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return std::isalnum(character) || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

bool safeSecret(const std::string& value)
{
    if (value.size() < 32 || value.size() > 256) return false;
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return std::isalnum(character) || character == '-' || character == '_';
        });
}

bool constantTimeEqual(const std::string& first, const std::string& second)
{
    const std::size_t maximum = std::max(first.size(), second.size());
    unsigned char difference = static_cast<unsigned char>(first.size() ^ second.size());
    for (std::size_t index = 0; index < maximum; ++index) {
        const unsigned char a = index < first.size()
            ? static_cast<unsigned char>(first[index]) : 0;
        const unsigned char b = index < second.size()
            ? static_cast<unsigned char>(second[index]) : 0;
        difference |= static_cast<unsigned char>(a ^ b);
    }
    return difference == 0;
}

bool verifySecret(const std::string& secret, const std::string& secretHash)
{
    if (!MediaSessionRepository::supportsSecretHash(secretHash)) return false;
    crypt_data data{};
    char* verified = crypt_r(secret.c_str(), secretHash.c_str(), &data);
    const bool accepted = verified != nullptr && constantTimeEqual(verified, secretHash);
    volatile unsigned char* bytes = reinterpret_cast<volatile unsigned char*>(&data);
    for (std::size_t index = 0; index < sizeof(data); ++index) bytes[index] = 0;
    return accepted;
}

bool parseCredential(
    const std::string& credential,
    std::string& grantId,
    std::string& secret)
{
    grantId.clear();
    secret.clear();
    const std::size_t separator = credential.find('.');
    if (separator == std::string::npos ||
        credential.find('.', separator + 1) != std::string::npos) {
        return false;
    }
    grantId = credential.substr(0, separator);
    secret = credential.substr(separator + 1);
    return safeIdentifier(grantId) && safeSecret(secret);
}

} // namespace

MediaAccessGrantAuthenticator::MediaAccessGrantAuthenticator(
    const MediaSessionRepository& repository,
    int idleTimeoutSeconds,
    int lastSeenWriteIntervalSeconds)
    : repository_(repository),
      idleTimeoutSeconds_(idleTimeoutSeconds),
      lastSeenWriteIntervalSeconds_(lastSeenWriteIntervalSeconds)
{
}

MediaAccessGrantAuthentication MediaAccessGrantAuthenticator::authenticate(
    const std::string& credential,
    const std::string& requestedSessionId) const
{
    MediaAccessGrantAuthentication result;
    if (!safeIdentifier(requestedSessionId) ||
        idleTimeoutSeconds_ < 0 || idleTimeoutSeconds_ > 86400 ||
        lastSeenWriteIntervalSeconds_ <= 0 || lastSeenWriteIntervalSeconds_ > 3600) {
        result.reasonCode = "invalid_media_access_request";
        return result;
    }

    std::string grantId;
    std::string secret;
    if (!parseCredential(credential, grantId, secret)) {
        result.reasonCode = "invalid_media_access_credential";
        wipe(secret);
        return result;
    }

    const auto grant = repository_.findResolvedGrant(grantId, idleTimeoutSeconds_);
    const bool secretAccepted = grant.has_value() &&
        verifySecret(secret, grant->secretHash);
    wipe(secret);

    if (!secretAccepted) {
        result.reasonCode = "media_access_denied";
        return result;
    }
    if (!grant->active || grant->revoked || grant->expired || grant->idleExpired) {
        result.reasonCode = "media_access_inactive";
        return result;
    }
    if (grant->sessionId != requestedSessionId || grant->routeEpoch <= 0) {
        result.reasonCode = "media_access_fence_mismatch";
        return result;
    }

    const auto touched = repository_.touchGrantIfDue(
        grantId, lastSeenWriteIntervalSeconds_);
    if (!touched.has_value()) {
        result.reasonCode = "media_access_state_unavailable";
        return result;
    }

    result.authenticated = true;
    result.grantId = grant->grantId;
    result.sessionId = grant->sessionId;
    result.routeId = grant->routeId;
    result.routeEpoch = grant->routeEpoch;
    result.actorId = grant->actorId;
    return result;
}
