#include "FirstAdminBootstrapIssuanceService.h"

#include "FirstAdminBootstrapRepository.h"

#include <crypt.h>
#include <sys/random.h>

#include <array>
#include <cerrno>
#include <chrono>
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
        const ssize_t received =
            getrandom(output + offset, size - offset, 0);
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

struct GeneratedBootstrapMaterial
{
    std::string bootstrapId;
    std::string setupSecret;
    std::string verifierHash;

    GeneratedBootstrapMaterial() = default;
    ~GeneratedBootstrapMaterial()
    {
        secureWipe(setupSecret);
    }

    GeneratedBootstrapMaterial(
        const GeneratedBootstrapMaterial&) = delete;
    GeneratedBootstrapMaterial& operator=(
        const GeneratedBootstrapMaterial&) = delete;

    GeneratedBootstrapMaterial(
        GeneratedBootstrapMaterial&& other) noexcept
        : bootstrapId(std::move(other.bootstrapId)),
          setupSecret(std::move(other.setupSecret)),
          verifierHash(std::move(other.verifierHash))
    {
        other.clearSecret();
    }

    GeneratedBootstrapMaterial& operator=(
        GeneratedBootstrapMaterial&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        clearSecret();
        bootstrapId = std::move(other.bootstrapId);
        setupSecret = std::move(other.setupSecret);
        verifierHash = std::move(other.verifierHash);
        other.clearSecret();
        return *this;
    }

    void clearSecret() noexcept
    {
        secureWipe(setupSecret);
    }
};

std::optional<GeneratedBootstrapMaterial> generateMaterial(
    const FirstAdminBootstrapIssuanceService::EntropySource& entropySource)
{
    std::array<unsigned char, IdentifierBytes> identifierBytes{};
    std::array<unsigned char, SecretBytes> secretBytes{};
    std::array<unsigned char, SaltBytes> saltBytes{};

    const bool generated =
        entropySource &&
        entropySource(
            identifierBytes.data(),
            identifierBytes.size()) &&
        entropySource(
            secretBytes.data(),
            secretBytes.size()) &&
        entropySource(
            saltBytes.data(),
            saltBytes.size());

    if (!generated)
    {
        secureWipeObject(identifierBytes);
        secureWipeObject(secretBytes);
        secureWipeObject(saltBytes);
        return std::nullopt;
    }

    GeneratedBootstrapMaterial material;
    material.bootstrapId =
        "fab_" + hexEncode(
            identifierBytes.data(),
            identifierBytes.size());
    material.setupSecret =
        base64UrlEncode(secretBytes.data(), secretBytes.size());

    std::string salt =
        cryptSaltEncode(saltBytes.data(), saltBytes.size());
    material.verifierHash =
        hashSecret(material.setupSecret, salt);

    secureWipe(salt);
    secureWipeObject(identifierBytes);
    secureWipeObject(secretBytes);
    secureWipeObject(saltBytes);

    if (material.setupSecret.size() < 32 ||
        material.verifierHash.empty() ||
        !FirstAdminBootstrapRepository::supportsVerifierHash(
            material.verifierHash))
    {
        return std::nullopt;
    }

    return std::optional<GeneratedBootstrapMaterial>(
        std::move(material));
}

FirstAdminBootstrapIssuanceStatus mapRepositoryStatus(
    FirstAdminBootstrapStatus status)
{
    switch (status)
    {
        case FirstAdminBootstrapStatus::ok:
            return FirstAdminBootstrapIssuanceStatus::issued;
        case FirstAdminBootstrapStatus::claimed:
            return FirstAdminBootstrapIssuanceStatus::claimed;
        case FirstAdminBootstrapStatus::conflict:
            return FirstAdminBootstrapIssuanceStatus::conflict;
        default:
            return FirstAdminBootstrapIssuanceStatus::storageError;
    }
}
}

IssuedFirstAdminBootstrap::~IssuedFirstAdminBootstrap()
{
    clearSecret();
}

IssuedFirstAdminBootstrap::IssuedFirstAdminBootstrap(
    IssuedFirstAdminBootstrap&& other) noexcept
    : bootstrapId(std::move(other.bootstrapId)),
      setupSecret(std::move(other.setupSecret)),
      expiresAt(std::move(other.expiresAt))
{
    other.clearSecret();
}

IssuedFirstAdminBootstrap&
IssuedFirstAdminBootstrap::operator=(
    IssuedFirstAdminBootstrap&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    clearSecret();
    bootstrapId = std::move(other.bootstrapId);
    setupSecret = std::move(other.setupSecret);
    expiresAt = std::move(other.expiresAt);
    other.clearSecret();
    return *this;
}

void IssuedFirstAdminBootstrap::clearSecret() noexcept
{
    secureWipe(setupSecret);
}

FirstAdminBootstrapIssuanceService::
FirstAdminBootstrapIssuanceService(
    FirstAdminBootstrapRepository& repository,
    EntropySource entropySource,
    Clock clock)
    : repository_(repository),
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

FirstAdminBootstrapIssuanceResult
FirstAdminBootstrapIssuanceService::issue(
    int lifetimeSeconds)
{
    FirstAdminBootstrapIssuanceResult result;

    if (lifetimeSeconds < MinimumLifetimeSeconds ||
        lifetimeSeconds > MaximumLifetimeSeconds)
    {
        result.status =
            FirstAdminBootstrapIssuanceStatus::invalidLifetime;
        return result;
    }

    auto material = generateMaterial(entropySource_);
    if (!material.has_value())
    {
        result.status =
            FirstAdminBootstrapIssuanceStatus::entropyUnavailable;
        return result;
    }

    const std::string expiresAt =
        formatTimestamp(
            clock_() + std::chrono::seconds(lifetimeSeconds));
    if (expiresAt.empty())
    {
        result.status =
            FirstAdminBootstrapIssuanceStatus::storageError;
        return result;
    }

    FirstAdminBootstrapRegistration registration;
    registration.bootstrapId = material->bootstrapId;
    registration.verifierHash = material->verifierHash;
    registration.expiresAt = expiresAt;

    const FirstAdminBootstrapStatus stored =
        repository_.registerBootstrap(registration);
    result.status = mapRepositoryStatus(stored);
    if (result.status !=
        FirstAdminBootstrapIssuanceStatus::issued)
    {
        return result;
    }

    IssuedFirstAdminBootstrap issued;
    issued.bootstrapId = material->bootstrapId;
    issued.setupSecret = std::move(material->setupSecret);
    issued.expiresAt = expiresAt;
    result.bootstrap.emplace(std::move(issued));
    return result;
}
