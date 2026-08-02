#include "BrowserSessionIssuanceService.h"

#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
#include "SecurityIdentityRepository.h"

#include <crypt.h>
#include <sys/random.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <ctime>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t IdentifierBytes = 16;
constexpr std::size_t SecretBytes = 32;
constexpr std::size_t SaltBytes = 16;
constexpr const char* CryptAlphabet =
    "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void secureWipe(std::string& value) noexcept
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
void secureWipeObject(Value& value) noexcept
{
    volatile unsigned char* bytes =
        reinterpret_cast<volatile unsigned char*>(&value);
    for (std::size_t index = 0; index < sizeof(Value); ++index)
    {
        bytes[index] = 0;
    }
}

bool systemEntropy(unsigned char* output, std::size_t size)
{
    if (output == nullptr || size == 0)
    {
        return false;
    }

    std::size_t offset = 0;
    while (offset < size)
    {
        const ssize_t received = getrandom(
            output + offset,
            size - offset,
            0);
        if (received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (received == 0)
        {
            return false;
        }
        offset += static_cast<std::size_t>(received);
    }
    return true;
}

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128)
    {
        return false;
    }

    for (const unsigned char character : value)
    {
        if (!std::isalnum(character) &&
            character != '-' &&
            character != '_' &&
            character != '.' &&
            character != ':')
        {
            return false;
        }
    }
    return true;
}

std::string hexEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);
    for (std::size_t index = 0; index < size; ++index)
    {
        result.push_back(Hex[(bytes[index] >> 4) & 0x0f]);
        result.push_back(Hex[bytes[index] & 0x0f]);
    }
    return result;
}

std::string base64UrlEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    static constexpr char Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    result.reserve(((size + 2) / 3) * 4);

    std::size_t index = 0;
    while (index + 3 <= size)
    {
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
    if (remaining == 1)
    {
        const unsigned int value =
            static_cast<unsigned int>(bytes[index]) << 16;
        result.push_back(Alphabet[(value >> 18) & 0x3f]);
        result.push_back(Alphabet[(value >> 12) & 0x3f]);
    }
    else if (remaining == 2)
    {
        const unsigned int value =
            (static_cast<unsigned int>(bytes[index]) << 16) |
            (static_cast<unsigned int>(bytes[index + 1]) << 8);
        result.push_back(Alphabet[(value >> 18) & 0x3f]);
        result.push_back(Alphabet[(value >> 12) & 0x3f]);
        result.push_back(Alphabet[(value >> 6) & 0x3f]);
    }

    return result;
}

std::string cryptSaltEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    std::string result;
    result.reserve(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        result.push_back(CryptAlphabet[bytes[index] & 0x3f]);
    }
    return result;
}

bool fillBytes(
    const BrowserSessionIssuanceService::EntropySource& entropySource,
    unsigned char* output,
    std::size_t size)
{
    return entropySource && entropySource(output, size);
}

std::string hashSecret(
    const std::string& secret,
    const std::string& randomSalt)
{
    if (secret.empty() || randomSalt.size() != SaltBytes)
    {
        return {};
    }

    const std::string setting =
        "$6$rounds=10000$" + randomSalt + "$";
    crypt_data data{};
    char* encoded = crypt_r(
        secret.c_str(),
        setting.c_str(),
        &data);

    std::string result;
    if (encoded != nullptr && encoded[0] != '*')
    {
        result = encoded;
    }
    secureWipeObject(data);
    return result;
}

std::string formatTimestamp(
    std::chrono::system_clock::time_point value)
{
    const std::time_t timestamp =
        std::chrono::system_clock::to_time_t(value);
    std::tm utc{};
    if (gmtime_r(&timestamp, &utc) == nullptr)
    {
        return {};
    }

    std::array<char, 32> buffer{};
    if (std::strftime(
            buffer.data(),
            buffer.size(),
            "%Y-%m-%d %H:%M:%S",
            &utc) == 0)
    {
        return {};
    }
    return buffer.data();
}

struct GeneratedBrowserSessionMaterial
{
    std::string tokenId;
    std::string sessionId;
    std::string credentialId;
    std::string sessionSecret;
    std::string csrfSecret;
    std::string sessionSecretHash;
    std::string csrfSecretHash;

    ~GeneratedBrowserSessionMaterial()
    {
        secureWipe(sessionSecret);
        secureWipe(csrfSecret);
    }
};

std::optional<GeneratedBrowserSessionMaterial> generateMaterial(
    const BrowserSessionIssuanceService::EntropySource& entropySource)
{
    std::array<unsigned char, IdentifierBytes> tokenBytes{};
    std::array<unsigned char, IdentifierBytes> sessionBytes{};
    std::array<unsigned char, IdentifierBytes> credentialBytes{};
    std::array<unsigned char, SecretBytes> sessionSecretBytes{};
    std::array<unsigned char, SecretBytes> csrfSecretBytes{};
    std::array<unsigned char, SaltBytes> sessionSaltBytes{};
    std::array<unsigned char, SaltBytes> csrfSaltBytes{};

    const bool generated =
        fillBytes(entropySource, tokenBytes.data(), tokenBytes.size()) &&
        fillBytes(entropySource, sessionBytes.data(), sessionBytes.size()) &&
        fillBytes(
            entropySource,
            credentialBytes.data(),
            credentialBytes.size()) &&
        fillBytes(
            entropySource,
            sessionSecretBytes.data(),
            sessionSecretBytes.size()) &&
        fillBytes(
            entropySource,
            csrfSecretBytes.data(),
            csrfSecretBytes.size()) &&
        fillBytes(
            entropySource,
            sessionSaltBytes.data(),
            sessionSaltBytes.size()) &&
        fillBytes(
            entropySource,
            csrfSaltBytes.data(),
            csrfSaltBytes.size());

    if (!generated)
    {
        secureWipeObject(tokenBytes);
        secureWipeObject(sessionBytes);
        secureWipeObject(credentialBytes);
        secureWipeObject(sessionSecretBytes);
        secureWipeObject(csrfSecretBytes);
        secureWipeObject(sessionSaltBytes);
        secureWipeObject(csrfSaltBytes);
        return std::nullopt;
    }

    GeneratedBrowserSessionMaterial material;
    material.tokenId =
        "bst_" + hexEncode(tokenBytes.data(), tokenBytes.size());
    material.sessionId =
        "bss_" + hexEncode(sessionBytes.data(), sessionBytes.size());
    material.credentialId =
        "bsc_" + hexEncode(credentialBytes.data(), credentialBytes.size());
    material.sessionSecret = base64UrlEncode(
        sessionSecretBytes.data(),
        sessionSecretBytes.size());
    material.csrfSecret = base64UrlEncode(
        csrfSecretBytes.data(),
        csrfSecretBytes.size());

    std::string sessionSalt = cryptSaltEncode(
        sessionSaltBytes.data(),
        sessionSaltBytes.size());
    std::string csrfSalt = cryptSaltEncode(
        csrfSaltBytes.data(),
        csrfSaltBytes.size());
    material.sessionSecretHash =
        hashSecret(material.sessionSecret, sessionSalt);
    material.csrfSecretHash =
        hashSecret(material.csrfSecret, csrfSalt);

    secureWipe(sessionSalt);
    secureWipe(csrfSalt);
    secureWipeObject(tokenBytes);
    secureWipeObject(sessionBytes);
    secureWipeObject(credentialBytes);
    secureWipeObject(sessionSecretBytes);
    secureWipeObject(csrfSecretBytes);
    secureWipeObject(sessionSaltBytes);
    secureWipeObject(csrfSaltBytes);

    if (material.sessionSecretHash.empty() ||
        material.csrfSecretHash.empty())
    {
        return std::nullopt;
    }
    return material;
}

class DatabaseTransaction
{
public:
    explicit DatabaseTransaction(Database& database)
        : database_(database),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_)
        {
            database_.execute("ROLLBACK;");
        }
    }

    bool active() const noexcept
    {
        return active_;
    }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
        {
            return false;
        }
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};
}

IssuedBrowserSession::~IssuedBrowserSession()
{
    clearSecrets();
}

IssuedBrowserSession::IssuedBrowserSession(
    IssuedBrowserSession&& other) noexcept
    : tokenId(std::move(other.tokenId)),
      sessionId(std::move(other.sessionId)),
      credentialId(std::move(other.credentialId)),
      sessionCookieValue(std::move(other.sessionCookieValue)),
      csrfToken(std::move(other.csrfToken)),
      expiresAt(std::move(other.expiresAt))
{
    other.clearSecrets();
}

IssuedBrowserSession& IssuedBrowserSession::operator=(
    IssuedBrowserSession&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    clearSecrets();
    tokenId = std::move(other.tokenId);
    sessionId = std::move(other.sessionId);
    credentialId = std::move(other.credentialId);
    sessionCookieValue = std::move(other.sessionCookieValue);
    csrfToken = std::move(other.csrfToken);
    expiresAt = std::move(other.expiresAt);
    other.clearSecrets();
    return *this;
}

void IssuedBrowserSession::clearSecrets() noexcept
{
    secureWipe(sessionCookieValue);
    secureWipe(csrfToken);
}

BrowserSessionIssuanceService::BrowserSessionIssuanceService(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BrowserSessionCredentialRepository& credentialRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      identityRepository_(identityRepository),
      credentialRepository_(credentialRepository),
      entropySource_(entropySource
          ? std::move(entropySource)
          : EntropySource(systemEntropy)),
      clock_(clock
          ? std::move(clock)
          : Clock([]
            {
                return std::chrono::system_clock::now();
            }))
{
}

BrowserSessionIssuanceResult
BrowserSessionIssuanceService::issueWithPolicy(
    const BrowserSessionIssuanceRequest& request)
{
    BrowserSessionIssuanceResult outcome;

    if (!safeIdentifier(request.actorId) ||
        !safeIdentifier(request.deviceId) ||
        !safeIdentifier(request.issuedFromCredentialId) ||
        request.lifetimeSeconds < MinimumLifetimeSeconds ||
        request.lifetimeSeconds > MaximumLifetimeSeconds ||
        request.maximumActivePerActor > MaximumActiveSessionsPerActor ||
        request.idleTimeoutSeconds < 0 ||
        (request.idleTimeoutSeconds > 0 &&
         (request.idleTimeoutSeconds < MinimumIdleTimeoutSeconds ||
          request.idleTimeoutSeconds > MaximumIdleTimeoutSeconds)))
    {
        return outcome;
    }

    auto material = generateMaterial(entropySource_);
    if (!material.has_value())
    {
        return outcome;
    }

    const std::string expiresAt = formatTimestamp(
        clock_() + std::chrono::seconds(request.lifetimeSeconds));
    if (expiresAt.empty())
    {
        return outcome;
    }

    BrowserSessionCredentialRegistration registration;
    registration.tokenId = material->tokenId;
    registration.sessionId = material->sessionId;
    registration.actorId = request.actorId;
    registration.deviceId = request.deviceId;
    registration.credentialId = material->credentialId;
    registration.issuedFromCredentialId =
        request.issuedFromCredentialId;
    registration.sessionSecretHash = material->sessionSecretHash;
    registration.csrfSecretHash = material->csrfSecretHash;
    registration.expiresAt = expiresAt;

    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        return outcome;
    }

    const auto actor = identityRepository_.findActor(request.actorId);
    const auto device = identityRepository_.findDevice(request.deviceId);
    const auto issuingCredential = identityRepository_.findCredential(
        request.issuedFromCredentialId);

    if (!actor.has_value() ||
        !actor->active ||
        actor->revoked ||
        !device.has_value() ||
        device->actorId != request.actorId ||
        !device->active ||
        device->revoked ||
        !issuingCredential.has_value() ||
        issuingCredential->actorId != request.actorId ||
        !issuingCredential->active ||
        issuingCredential->expired ||
        issuingCredential->revoked)
    {
        return outcome;
    }

    if (request.maximumActivePerActor > 0)
    {
        const auto activeCount =
            credentialRepository_.countEffectiveActiveByActorId(
                request.actorId,
                request.idleTimeoutSeconds);
        if (!activeCount.has_value())
        {
            return outcome;
        }
        if (*activeCount >= request.maximumActivePerActor)
        {
            outcome.status = BrowserSessionIssuanceStatus::LimitReached;
            return outcome;
        }
    }

    if (!identityRepository_.createSessionCredential(
            material->sessionId,
            request.actorId,
            request.deviceId,
            material->credentialId,
            "browser-session",
            expiresAt,
            request.issuedFromCredentialId) ||
        !credentialRepository_.insert(registration) ||
        !transaction.commit())
    {
        return outcome;
    }

    IssuedBrowserSession issued;
    issued.tokenId = material->tokenId;
    issued.sessionId = material->sessionId;
    issued.credentialId = material->credentialId;
    issued.sessionCookieValue =
        material->tokenId + "." + material->sessionSecret;
    issued.csrfToken = material->csrfSecret;
    issued.expiresAt = expiresAt;

    outcome.status = BrowserSessionIssuanceStatus::Issued;
    outcome.session = std::move(issued);
    return outcome;
}

std::optional<IssuedBrowserSession> BrowserSessionIssuanceService::issue(
    const BrowserSessionIssuanceRequest& request)
{
    BrowserSessionIssuanceResult result = issueWithPolicy(request);
    if (result.status != BrowserSessionIssuanceStatus::Issued ||
        !result.session.has_value())
    {
        return std::nullopt;
    }
    return std::move(result.session);
}
