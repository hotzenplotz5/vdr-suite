#include "MediaSessionIssuanceService.h"

#include <crypt.h>
#include <sys/random.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <ctime>
#include <optional>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t IdentifierBytes = 16;
constexpr std::size_t SecretBytes = 32;
constexpr std::size_t SaltBytes = 16;
constexpr const char* CryptAlphabet =
    "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void wipe(std::string& value) noexcept
{
    volatile char* bytes = value.empty()
        ? nullptr
        : const_cast<volatile char*>(value.data());
    for (std::size_t index = 0; index < value.size(); ++index) bytes[index] = 0;
    value.clear();
}

template <typename Value>
void wipeObject(Value& value) noexcept
{
    volatile unsigned char* bytes = reinterpret_cast<volatile unsigned char*>(&value);
    for (std::size_t index = 0; index < sizeof(Value); ++index) bytes[index] = 0;
}

bool systemEntropy(unsigned char* output, std::size_t size)
{
    if (output == nullptr || size == 0) return false;
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t received = getrandom(output + offset, size - offset, 0);
        if (received < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (received == 0) return false;
        offset += static_cast<std::size_t>(received);
    }
    return true;
}

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    for (unsigned char character : value) {
        if (!std::isalnum(character) && character != '-' && character != '_' &&
            character != '.' && character != ':') return false;
    }
    return true;
}

bool supportedResourceKind(const std::string& value)
{
    return value == "recording" || value == "live-channel";
}

std::string hexEncode(const unsigned char* bytes, std::size_t size)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);
    for (std::size_t index = 0; index < size; ++index) {
        result.push_back(Hex[(bytes[index] >> 4) & 0x0f]);
        result.push_back(Hex[bytes[index] & 0x0f]);
    }
    return result;
}

std::string base64UrlEncode(const unsigned char* bytes, std::size_t size)
{
    static constexpr char Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    result.reserve(((size + 2) / 3) * 4);
    std::size_t index = 0;
    while (index + 3 <= size) {
        const unsigned int value =
            (static_cast<unsigned int>(bytes[index]) << 16) |
            (static_cast<unsigned int>(bytes[index + 1]) << 8) |
            static_cast<unsigned int>(bytes[index + 2]);
        result.push_back(Alphabet[(value >> 18) & 0x3f]);
        result.push_back(Alphabet[(value >> 12) & 0x3f]);
        result.push_back(Alphabet[(value >> 6) & 0x3f]);
        result.push_back(Alphabet[value & 0x3f]);
        index += 3;
    }
    const std::size_t remaining = size - index;
    if (remaining == 1) {
        const unsigned int value = static_cast<unsigned int>(bytes[index]) << 16;
        result.push_back(Alphabet[(value >> 18) & 0x3f]);
        result.push_back(Alphabet[(value >> 12) & 0x3f]);
    }
    else if (remaining == 2) {
        const unsigned int value =
            (static_cast<unsigned int>(bytes[index]) << 16) |
            (static_cast<unsigned int>(bytes[index + 1]) << 8);
        result.push_back(Alphabet[(value >> 18) & 0x3f]);
        result.push_back(Alphabet[(value >> 12) & 0x3f]);
        result.push_back(Alphabet[(value >> 6) & 0x3f]);
    }
    return result;
}

std::string saltEncode(const unsigned char* bytes, std::size_t size)
{
    std::string result;
    result.reserve(size);
    for (std::size_t index = 0; index < size; ++index)
        result.push_back(CryptAlphabet[bytes[index] & 0x3f]);
    return result;
}

std::string hashSecret(const std::string& secret, const std::string& salt)
{
    if (secret.empty() || salt.size() != SaltBytes) return {};
    const std::string setting = "$6$rounds=10000$" + salt + "$";
    crypt_data data{};
    char* encoded = crypt_r(secret.c_str(), setting.c_str(), &data);
    std::string result;
    if (encoded != nullptr && encoded[0] != '*') result = encoded;
    wipeObject(data);
    return result;
}

std::string formatTimestamp(std::chrono::system_clock::time_point value)
{
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(value);
    std::tm utc{};
    if (gmtime_r(&timestamp, &utc) == nullptr) return {};
    std::array<char, 32> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &utc) == 0)
        return {};
    return buffer.data();
}

struct Material
{
    std::string sessionId;
    std::string routeId;
    std::string leaseId;
    std::string grantId;
    std::string secret;
    std::string secretHash;

    ~Material() { wipe(secret); }
};

std::optional<Material> generate(
    const MediaSessionIssuanceService::EntropySource& entropy)
{
    std::array<unsigned char, IdentifierBytes> session{};
    std::array<unsigned char, IdentifierBytes> route{};
    std::array<unsigned char, IdentifierBytes> lease{};
    std::array<unsigned char, IdentifierBytes> grant{};
    std::array<unsigned char, SecretBytes> secret{};
    std::array<unsigned char, SaltBytes> salt{};

    const bool okay = entropy &&
        entropy(session.data(), session.size()) &&
        entropy(route.data(), route.size()) &&
        entropy(lease.data(), lease.size()) &&
        entropy(grant.data(), grant.size()) &&
        entropy(secret.data(), secret.size()) &&
        entropy(salt.data(), salt.size());
    if (!okay) {
        wipeObject(session); wipeObject(route); wipeObject(lease);
        wipeObject(grant); wipeObject(secret); wipeObject(salt);
        return std::nullopt;
    }

    Material material;
    material.sessionId = "ms_" + hexEncode(session.data(), session.size());
    material.routeId = "mr_" + hexEncode(route.data(), route.size());
    material.leaseId = "pl_" + hexEncode(lease.data(), lease.size());
    material.grantId = "mg_" + hexEncode(grant.data(), grant.size());
    material.secret = base64UrlEncode(secret.data(), secret.size());
    std::string saltText = saltEncode(salt.data(), salt.size());
    material.secretHash = hashSecret(material.secret, saltText);
    wipe(saltText);
    wipeObject(session); wipeObject(route); wipeObject(lease);
    wipeObject(grant); wipeObject(secret); wipeObject(salt);

    if (material.secretHash.empty()) return std::nullopt;
    return material;
}

} // namespace

IssuedMediaSession::~IssuedMediaSession()
{
    clearSecret();
}

IssuedMediaSession::IssuedMediaSession(IssuedMediaSession&& other) noexcept
    : sessionId(std::move(other.sessionId)),
      routeId(std::move(other.routeId)),
      routeEpoch(other.routeEpoch),
      leaseId(std::move(other.leaseId)),
      workspaceId(std::move(other.workspaceId)),
      grantId(std::move(other.grantId)),
      accessCredential(std::move(other.accessCredential)),
      expiresAt(std::move(other.expiresAt))
{
    other.routeEpoch = 0;
    other.clearSecret();
}

IssuedMediaSession& IssuedMediaSession::operator=(IssuedMediaSession&& other) noexcept
{
    if (this == &other) return *this;
    clearSecret();
    sessionId = std::move(other.sessionId);
    routeId = std::move(other.routeId);
    routeEpoch = other.routeEpoch;
    leaseId = std::move(other.leaseId);
    workspaceId = std::move(other.workspaceId);
    grantId = std::move(other.grantId);
    accessCredential = std::move(other.accessCredential);
    expiresAt = std::move(other.expiresAt);
    other.routeEpoch = 0;
    other.clearSecret();
    return *this;
}

void IssuedMediaSession::clearSecret() noexcept
{
    wipe(accessCredential);
}

MediaSessionIssuanceService::MediaSessionIssuanceService(
    MediaSessionRepository& repository,
    EntropySource entropySource,
    Clock clock)
    : repository_(repository),
      entropySource_(entropySource ? std::move(entropySource) : EntropySource(systemEntropy)),
      clock_(clock ? std::move(clock) : Clock([] { return std::chrono::system_clock::now(); }))
{
}

MediaSessionIssuanceResult MediaSessionIssuanceService::issue(
    const MediaSessionIssuanceRequest& request)
{
    MediaSessionIssuanceResult result;
    if (!safeIdentifier(request.actorId) || !safeIdentifier(request.backendId) ||
        !supportedResourceKind(request.resourceKind) || request.resourceId.empty() ||
        request.resourceId.size() > 512 ||
        !safeIdentifier(request.presentationProfileId) ||
        !safeIdentifier(request.providerId) ||
        request.lifetimeSeconds < 300 || request.lifetimeSeconds > 21600) {
        result.reasonCode = "invalid_media_session_request";
        return result;
    }

    auto material = generate(entropySource_);
    if (!material.has_value()) {
        result.reasonCode = "media_session_entropy_unavailable";
        return result;
    }

    const std::string expiresAt = formatTimestamp(
        clock_() + std::chrono::seconds(request.lifetimeSeconds));
    if (expiresAt.empty()) {
        result.reasonCode = "media_session_expiry_unavailable";
        return result;
    }

    MediaSessionBundleRegistration registration;
    registration.sessionId = material->sessionId;
    registration.actorId = request.actorId;
    registration.backendId = request.backendId;
    registration.resourceKind = request.resourceKind;
    registration.resourceId = request.resourceId;
    registration.presentationProfileId = request.presentationProfileId;
    registration.routeId = material->routeId;
    registration.routeEpoch = 1;
    registration.providerId = request.providerId;
    registration.leaseId = material->leaseId;
    registration.workspaceId = material->sessionId;
    registration.grantId = material->grantId;
    registration.secretHash = material->secretHash;
    registration.expiresAt = expiresAt;

    if (!repository_.insertProvisioningBundle(registration)) {
        result.reasonCode = "media_session_persistence_failed";
        return result;
    }

    result.session.sessionId = material->sessionId;
    result.session.routeId = material->routeId;
    result.session.routeEpoch = 1;
    result.session.leaseId = material->leaseId;
    result.session.workspaceId = material->sessionId;
    result.session.grantId = material->grantId;
    result.session.accessCredential = material->grantId + "." + material->secret;
    result.session.expiresAt = expiresAt;
    result.issued = true;
    return result;
}
