#include "DevicePairingRequestService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DevicePairingRequestRepository.h"

#include <crypt.h>
#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t IdentifierBytes = 16U;
constexpr std::size_t TokenBytes = 32U;
constexpr std::size_t UserCodeBytes = 8U;
constexpr std::size_t SaltBytes = 16U;
constexpr const char* PairingCodeAlphabet =
    "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
constexpr const char* CryptAlphabet =
    "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

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
            database_.execute("ROLLBACK;");
    }

    bool active() const noexcept
    {
        return active_;
    }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
            return false;
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};

void secureWipe(std::string& value) noexcept
{
    volatile char* bytes = value.empty()
        ? nullptr
        : const_cast<volatile char*>(value.data());
    for (std::size_t index = 0U;
         index < value.size();
         ++index)
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
    for (std::size_t index = 0U;
         index < sizeof(Value);
         ++index)
    {
        bytes[index] = 0U;
    }
}

bool systemEntropy(
    unsigned char* output,
    std::size_t size)
{
    if (output == nullptr || size == 0U)
        return false;

    std::size_t offset = 0U;
    while (offset < size)
    {
        const ssize_t received =
            getrandom(output + offset, size - offset, 0);
        if (received < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (received == 0)
            return false;
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
    result.reserve(size * 2U);

    for (std::size_t index = 0U; index < size; ++index)
    {
        result.push_back(
            Hex[(bytes[index] >> 4U) & 0x0fU]);
        result.push_back(
            Hex[bytes[index] & 0x0fU]);
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
    result.reserve(((size + 2U) / 3U) * 4U);

    std::size_t index = 0U;
    while (index + 3U <= size)
    {
        const unsigned int value =
            (static_cast<unsigned int>(bytes[index]) << 16U) |
            (static_cast<unsigned int>(bytes[index + 1U]) << 8U) |
            static_cast<unsigned int>(bytes[index + 2U]);
        result.push_back(Alphabet[(value >> 18U) & 0x3fU]);
        result.push_back(Alphabet[(value >> 12U) & 0x3fU]);
        result.push_back(Alphabet[(value >> 6U) & 0x3fU]);
        result.push_back(Alphabet[value & 0x3fU]);
        index += 3U;
    }

    const std::size_t remaining = size - index;
    if (remaining == 1U)
    {
        const unsigned int value =
            static_cast<unsigned int>(bytes[index]) << 16U;
        result.push_back(Alphabet[(value >> 18U) & 0x3fU]);
        result.push_back(Alphabet[(value >> 12U) & 0x3fU]);
    }
    else if (remaining == 2U)
    {
        const unsigned int value =
            (static_cast<unsigned int>(bytes[index]) << 16U) |
            (static_cast<unsigned int>(bytes[index + 1U]) << 8U);
        result.push_back(Alphabet[(value >> 18U) & 0x3fU]);
        result.push_back(Alphabet[(value >> 12U) & 0x3fU]);
        result.push_back(Alphabet[(value >> 6U) & 0x3fU]);
    }

    return result;
}

std::string pairingCode(
    const unsigned char* bytes,
    std::size_t size)
{
    if (bytes == nullptr || size != UserCodeBytes)
        return {};

    std::string result;
    result.reserve(9U);
    for (std::size_t index = 0U; index < size; ++index)
    {
        if (index == 4U)
            result.push_back('-');
        result.push_back(
            PairingCodeAlphabet[bytes[index] & 0x1fU]);
    }
    return result;
}

std::string cryptSaltEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    std::string result;
    result.reserve(size);
    for (std::size_t index = 0U; index < size; ++index)
        result.push_back(
            CryptAlphabet[bytes[index] & 0x3fU]);
    return result;
}

std::string hashSecret(
    const std::string& secret,
    const std::string& randomSalt)
{
    if (secret.empty() ||
        randomSalt.size() != SaltBytes)
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
        result = encoded;

    secureWipeObject(data);
    return result;
}

bool constantTimeEqual(
    const std::string& left,
    const std::string& right)
{
    if (left.size() != right.size())
        return false;

    unsigned char difference = 0U;
    for (std::size_t index = 0U;
         index < left.size();
         ++index)
    {
        difference |= static_cast<unsigned char>(
            left[index] ^ right[index]);
    }
    return difference == 0U;
}

bool verifySecret(
    const std::string& secret,
    const std::string& verifierHash)
{
    if (secret.empty() ||
        !DevicePairingRequestRepository::
            supportsSecretHash(verifierHash))
    {
        return false;
    }

    crypt_data data{};
    char* encoded = crypt_r(
        secret.c_str(),
        verifierHash.c_str(),
        &data);
    const bool verified =
        encoded != nullptr &&
        constantTimeEqual(encoded, verifierHash);
    secureWipeObject(data);
    return verified;
}

std::string formatDatabaseTimestamp(
    std::chrono::system_clock::time_point value)
{
    const std::time_t timestamp =
        std::chrono::system_clock::to_time_t(value);
    std::tm utc{};
    if (gmtime_r(&timestamp, &utc) == nullptr)
        return {};

    std::array<char, 32> buffer{};
    if (std::strftime(
            buffer.data(),
            buffer.size(),
            "%Y-%m-%d %H:%M:%S",
            &utc) == 0U)
    {
        return {};
    }
    return buffer.data();
}

std::string publicTimestamp(
    const std::string& databaseTimestamp)
{
    if (databaseTimestamp.size() != 19U)
        return {};
    std::string result = databaseTimestamp;
    result[10] = 'T';
    result.push_back('Z');
    return result;
}

std::string nowPublicTimestamp(
    std::chrono::system_clock::time_point value)
{
    return publicTimestamp(
        formatDatabaseTimestamp(value));
}

bool safePresentationText(
    const std::string& value,
    std::size_t maximumLength,
    bool allowEmpty)
{
    if ((!allowEmpty && value.empty()) ||
        value.size() > maximumLength)
    {
        return false;
    }

    return std::none_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return character == 0U ||
                character == '\r' ||
                character == '\n' ||
                character < 0x20U;
        });
}

bool safeClientKind(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 64U &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '.' ||
                    character == '_' ||
                    character == '-';
            });
}

struct GeneratedPairingMaterial
{
    std::string pairingRequestId;
    std::string userCode;
    std::string pairingToken;
    std::string userCodeHash;
    std::string pairingTokenHash;

    GeneratedPairingMaterial() = default;
    ~GeneratedPairingMaterial()
    {
        secureWipe(userCode);
        secureWipe(pairingToken);
    }

    GeneratedPairingMaterial(
        const GeneratedPairingMaterial&) = delete;
    GeneratedPairingMaterial& operator=(
        const GeneratedPairingMaterial&) = delete;

    GeneratedPairingMaterial(
        GeneratedPairingMaterial&& other) noexcept
        : pairingRequestId(std::move(other.pairingRequestId)),
          userCode(std::move(other.userCode)),
          pairingToken(std::move(other.pairingToken)),
          userCodeHash(std::move(other.userCodeHash)),
          pairingTokenHash(std::move(other.pairingTokenHash))
    {
        secureWipe(other.userCode);
        secureWipe(other.pairingToken);
    }

    GeneratedPairingMaterial& operator=(
        GeneratedPairingMaterial&& other) noexcept
    {
        if (this == &other)
            return *this;
        secureWipe(userCode);
        secureWipe(pairingToken);
        pairingRequestId =
            std::move(other.pairingRequestId);
        userCode = std::move(other.userCode);
        pairingToken =
            std::move(other.pairingToken);
        userCodeHash =
            std::move(other.userCodeHash);
        pairingTokenHash =
            std::move(other.pairingTokenHash);
        secureWipe(other.userCode);
        secureWipe(other.pairingToken);
        return *this;
    }
};

std::optional<GeneratedPairingMaterial> generateMaterial(
    const DevicePairingRequestService::EntropySource& entropySource,
    bool& hashingFailed)
{
    hashingFailed = false;

    std::array<unsigned char, IdentifierBytes> identifier{};
    std::array<unsigned char, UserCodeBytes> userCodeBytes{};
    std::array<unsigned char, TokenBytes> tokenBytes{};
    std::array<unsigned char, SaltBytes> codeSaltBytes{};
    std::array<unsigned char, SaltBytes> tokenSaltBytes{};

    const bool generated =
        entropySource &&
        entropySource(identifier.data(), identifier.size()) &&
        entropySource(userCodeBytes.data(), userCodeBytes.size()) &&
        entropySource(tokenBytes.data(), tokenBytes.size()) &&
        entropySource(codeSaltBytes.data(), codeSaltBytes.size()) &&
        entropySource(tokenSaltBytes.data(), tokenSaltBytes.size());

    if (!generated)
    {
        secureWipeObject(identifier);
        secureWipeObject(userCodeBytes);
        secureWipeObject(tokenBytes);
        secureWipeObject(codeSaltBytes);
        secureWipeObject(tokenSaltBytes);
        return std::nullopt;
    }

    GeneratedPairingMaterial material;
    material.pairingRequestId =
        "dpr_" + hexEncode(
            identifier.data(),
            identifier.size());
    material.userCode =
        pairingCode(
            userCodeBytes.data(),
            userCodeBytes.size());
    material.pairingToken =
        base64UrlEncode(
            tokenBytes.data(),
            tokenBytes.size());

    std::string codeSalt =
        cryptSaltEncode(
            codeSaltBytes.data(),
            codeSaltBytes.size());
    std::string tokenSalt =
        cryptSaltEncode(
            tokenSaltBytes.data(),
            tokenSaltBytes.size());

    material.userCodeHash =
        hashSecret(material.userCode, codeSalt);
    material.pairingTokenHash =
        hashSecret(material.pairingToken, tokenSalt);

    secureWipe(codeSalt);
    secureWipe(tokenSalt);
    secureWipeObject(identifier);
    secureWipeObject(userCodeBytes);
    secureWipeObject(tokenBytes);
    secureWipeObject(codeSaltBytes);
    secureWipeObject(tokenSaltBytes);

    hashingFailed =
        material.userCodeHash.empty() ||
        material.pairingTokenHash.empty();

    if (material.pairingRequestId.empty() ||
        material.userCode.size() != 9U ||
        material.pairingToken.size() < 32U ||
        hashingFailed)
    {
        return std::nullopt;
    }

    return std::optional<GeneratedPairingMaterial>(
        std::move(material));
}

DevicePairingResource resourceFromStored(
    const StoredDevicePairingRequest& stored)
{
    DevicePairingResource resource;
    resource.pairingRequestId =
        stored.pairingRequestId;
    resource.client.displayName =
        stored.displayName;
    resource.client.clientKind =
        stored.clientKind;
    resource.client.appVersion =
        stored.appVersion;
    resource.state =
        stored.state;
    resource.expiresAt =
        publicTimestamp(stored.expiresAt);
    resource.pollIntervalSeconds =
        DevicePairingRequestService::PollIntervalSeconds;
    return resource;
}
}

IssuedDevicePairingRequest::~IssuedDevicePairingRequest()
{
    clearBootstrapMaterial();
}

IssuedDevicePairingRequest::IssuedDevicePairingRequest(
    IssuedDevicePairingRequest&& other) noexcept
    : resource(std::move(other.resource)),
      userCode(std::move(other.userCode)),
      pairingToken(std::move(other.pairingToken))
{
    other.clearBootstrapMaterial();
}

IssuedDevicePairingRequest&
IssuedDevicePairingRequest::operator=(
    IssuedDevicePairingRequest&& other) noexcept
{
    if (this == &other)
        return *this;

    clearBootstrapMaterial();
    resource = std::move(other.resource);
    userCode = std::move(other.userCode);
    pairingToken = std::move(other.pairingToken);
    other.clearBootstrapMaterial();
    return *this;
}

void IssuedDevicePairingRequest::clearBootstrapMaterial() noexcept
{
    secureWipe(userCode);
    secureWipe(pairingToken);
}

DevicePairingRequestService::DevicePairingRequestService(
    Database& database,
    DevicePairingRequestRepository& repository,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      repository_(repository),
      accountabilityRepository_(accountabilityRepository),
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

DevicePairingIssueResult
DevicePairingRequestService::issue(
    const DevicePairingIssueRequest& request)
{
    DevicePairingIssueResult result;

    if (!safePresentationText(
            request.client.displayName,
            128U,
            false) ||
        !safeClientKind(request.client.clientKind) ||
        !safePresentationText(
            request.client.appVersion,
            64U,
            true) ||
        request.requestId.empty() ||
        request.requestId.size() > 256U ||
        request.correlationId.size() > 256U)
    {
        result.status =
            DevicePairingIssueStatus::invalidRequest;
        return result;
    }

    bool hashingFailed = false;
    auto material =
        generateMaterial(
            entropySource_,
            hashingFailed);
    if (!material.has_value())
    {
        result.status = hashingFailed
            ? DevicePairingIssueStatus::hashingUnavailable
            : DevicePairingIssueStatus::entropyUnavailable;
        return result;
    }

    const auto now = clock_();
    const std::string expiresAt =
        formatDatabaseTimestamp(
            now + std::chrono::seconds(
                LifetimeSeconds));
    const std::string publicExpiresAt =
        publicTimestamp(expiresAt);
    if (expiresAt.empty() ||
        publicExpiresAt.empty())
    {
        result.status =
            DevicePairingIssueStatus::storageError;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        result.status =
            DevicePairingIssueStatus::storageError;
        return result;
    }

    DevicePairingRequestRegistration registration;
    registration.pairingRequestId =
        material->pairingRequestId;
    registration.userCodeHash =
        material->userCodeHash;
    registration.pairingTokenHash =
        material->pairingTokenHash;
    registration.displayName =
        request.client.displayName;
    registration.clientKind =
        request.client.clientKind;
    registration.appVersion =
        request.client.appVersion;
    registration.expiresAt =
        expiresAt;

    const DevicePairingRequestRepositoryStatus stored =
        repository_.registerInActiveTransaction(
            registration);
    if (stored !=
        DevicePairingRequestRepositoryStatus::ok)
    {
        result.status =
            stored ==
                DevicePairingRequestRepositoryStatus::invalid
                ? DevicePairingIssueStatus::invalidRequest
                : DevicePairingIssueStatus::storageError;
        return result;
    }

    AccountabilityEvent event;
    event.eventId =
        "audit-" + material->pairingRequestId;
    event.classes = "audit,security";
    event.eventType =
        "device_pairing.requested";
    event.severity = "info";
    event.occurredAt =
        nowPublicTimestamp(now);
    event.actorId = "anonymous";
    event.actorType = "anonymous";
    event.authenticationState = "anonymous";
    event.permission =
        "device.pairing.bootstrap";
    event.backendId = "*";
    event.operationId =
        material->pairingRequestId;
    event.requestId = request.requestId;
    event.correlationId =
        request.correlationId;
    event.action =
        "device_pairing.create";
    event.decision = "allowed";
    event.reasonCode =
        "pairing_request_created";
    event.outcome = "created";

    if (event.occurredAt.empty() ||
        !accountabilityRepository_.append(event) ||
        !transaction.commit())
    {
        result.status =
            DevicePairingIssueStatus::storageError;
        return result;
    }

    IssuedDevicePairingRequest issued;
    issued.resource.pairingRequestId =
        material->pairingRequestId;
    issued.resource.client =
        request.client;
    issued.resource.state = "pending";
    issued.resource.expiresAt =
        publicExpiresAt;
    issued.resource.pollIntervalSeconds =
        PollIntervalSeconds;
    issued.userCode =
        std::move(material->userCode);
    issued.pairingToken =
        std::move(material->pairingToken);

    result.status =
        DevicePairingIssueStatus::issued;
    result.pairing.emplace(std::move(issued));
    return result;
}

DevicePairingPollResult
DevicePairingRequestService::poll(
    const DevicePairingPollRequest& request) const
{
    DevicePairingPollResult result;

    if (request.pairingRequestId.empty() ||
        request.pairingRequestId.size() > 128U ||
        request.pairingToken.empty() ||
        request.pairingToken.size() > 256U)
    {
        result.status =
            DevicePairingPollStatus::invalidRequest;
        return result;
    }

    const DevicePairingRequestLookupResult found =
        repository_.findById(
            request.pairingRequestId);

    switch (found.status)
    {
        case DevicePairingRequestRepositoryStatus::ok:
            break;
        case DevicePairingRequestRepositoryStatus::invalid:
            result.status =
                DevicePairingPollStatus::invalidRequest;
            return result;
        case DevicePairingRequestRepositoryStatus::notFound:
        case DevicePairingRequestRepositoryStatus::invalidated:
            result.status =
                DevicePairingPollStatus::notFound;
            return result;
        case DevicePairingRequestRepositoryStatus::expired:
            result.status =
                DevicePairingPollStatus::expired;
            return result;
        case DevicePairingRequestRepositoryStatus::conflict:
        case DevicePairingRequestRepositoryStatus::storageError:
        case DevicePairingRequestRepositoryStatus::transactionRequired:
            result.status =
                DevicePairingPollStatus::unavailable;
            return result;
    }

    if (!verifySecret(
            request.pairingToken,
            found.request.pairingTokenHash))
    {
        result.status =
            DevicePairingPollStatus::unauthorized;
        return result;
    }

    if (found.request.state != "pending")
    {
        result.status =
            DevicePairingPollStatus::unavailable;
        return result;
    }

    result.resource =
        resourceFromStored(found.request);
    if (result.resource.expiresAt.empty())
    {
        result.status =
            DevicePairingPollStatus::unavailable;
        return result;
    }

    result.status =
        DevicePairingPollStatus::ok;
    return result;
}
