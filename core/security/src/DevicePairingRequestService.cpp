#include "DevicePairingRequestService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DevicePairingRequestRepository.h"
#include "DeviceCredentialVerifierRepository.h"
#include "SecurityIdentityProvisioningRepository.h"

#include <crypt.h>
#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <limits>
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

std::string administrativeRevision(
    const StoredDevicePairingRequest& stored)
{
    if (stored.pairingRequestId.empty() ||
        stored.revision == 0U)
    {
        return {};
    }
    return "device-pairing:" +
        stored.pairingRequestId + ":" +
        std::to_string(stored.revision);
}

bool parseAdministrativeRevision(
    const std::string& resourceRevision,
    const std::string& pairingRequestId,
    std::uint64_t& revision)
{
    const std::string prefix =
        "device-pairing:" + pairingRequestId + ":";
    if (resourceRevision.size() <= prefix.size() ||
        resourceRevision.compare(0U, prefix.size(), prefix) != 0)
    {
        return false;
    }

    std::uint64_t parsed = 0U;
    for (std::size_t index = prefix.size();
         index < resourceRevision.size();
         ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(
                resourceRevision[index]);
        if (character < '0' || character > '9')
            return false;

        const std::uint64_t digit =
            static_cast<std::uint64_t>(
                character - '0');
        if (parsed >
            (std::numeric_limits<std::uint64_t>::max() -
             digit) / 10U)
        {
            return false;
        }
        parsed = parsed * 10U + digit;
    }

    if (parsed == 0U)
        return false;
    revision = parsed;
    return true;
}

DevicePairingAdministrativeResource
administrativeResourceFromStored(
    const StoredDevicePairingRequest& stored)
{
    DevicePairingAdministrativeResource resource;
    resource.resource = resourceFromStored(stored);
    resource.resourceRevision =
        administrativeRevision(stored);
    resource.decidedByActorId =
        stored.decidedByActorId;
    if (!stored.decidedAt.empty())
    {
        resource.decidedAt =
            publicTimestamp(stored.decidedAt);
    }
    return resource;
}

std::optional<std::string> randomAuditId(
    const DevicePairingRequestService::EntropySource& entropySource)
{
    std::array<unsigned char, IdentifierBytes> bytes{};
    if (!entropySource ||
        !entropySource(bytes.data(), bytes.size()))
    {
        secureWipeObject(bytes);
        return std::nullopt;
    }

    std::string id =
        "ace_" + hexEncode(bytes.data(), bytes.size());
    secureWipeObject(bytes);
    return id;
}

bool validAdministrationContext(
    const DevicePairingAdministrationContext& context)
{
    return safePresentationText(
               context.actorId, 128U, false) &&
        safePresentationText(
               context.actorType, 32U, false) &&
        safePresentationText(
               context.requestId, 256U, false) &&
        safePresentationText(
               context.correlationId, 256U, true);
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


IssuedDeviceCredential::~IssuedDeviceCredential()
{
    clearSecret();
}

IssuedDeviceCredential::IssuedDeviceCredential(
    IssuedDeviceCredential&& other) noexcept
    : actorId(std::move(other.actorId)),
      deviceId(std::move(other.deviceId)),
      credentialId(std::move(other.credentialId)),
      credentialSecret(std::move(other.credentialSecret))
{
    other.clearSecret();
}

IssuedDeviceCredential& IssuedDeviceCredential::operator=(
    IssuedDeviceCredential&& other) noexcept
{
    if (this == &other)
        return *this;
    clearSecret();
    actorId = std::move(other.actorId);
    deviceId = std::move(other.deviceId);
    credentialId = std::move(other.credentialId);
    credentialSecret = std::move(other.credentialSecret);
    other.clearSecret();
    return *this;
}

void IssuedDeviceCredential::clearSecret() noexcept
{
    secureWipe(credentialSecret);
}

DevicePairingCredentialIssueResult
DevicePairingRequestService::issueDeviceCredential(
    const DevicePairingCredentialIssueRequest& request,
    SecurityIdentityProvisioningRepository& provisioning,
    DeviceCredentialVerifierRepository& verifiers)
{
    DevicePairingCredentialIssueResult result;
    if (request.pairingRequestId.empty() ||
        request.pairingRequestId.size() > 128U ||
        request.pairingToken.empty() ||
        request.pairingToken.size() > 256U ||
        request.requestId.empty() ||
        request.requestId.size() > 256U ||
        request.correlationId.size() > 256U)
    {
        result.status = DevicePairingCredentialIssueStatus::invalidRequest;
        return result;
    }

    // Create all secret material before acquiring the write transaction.
    // Nothing may be issued if secure entropy or hashing fails.
    std::array<unsigned char, IdentifierBytes> actorBytes{};
    std::array<unsigned char, IdentifierBytes> deviceBytes{};
    std::array<unsigned char, IdentifierBytes> credentialBytes{};
    std::array<unsigned char, TokenBytes> secretBytes{};
    std::array<unsigned char, SaltBytes> saltBytes{};
    const bool randomOk =
        entropySource_(actorBytes.data(), actorBytes.size()) &&
        entropySource_(deviceBytes.data(), deviceBytes.size()) &&
        entropySource_(credentialBytes.data(), credentialBytes.size()) &&
        entropySource_(secretBytes.data(), secretBytes.size()) &&
        entropySource_(saltBytes.data(), saltBytes.size());
    if (!randomOk)
    {
        secureWipeObject(actorBytes);
        secureWipeObject(deviceBytes);
        secureWipeObject(credentialBytes);
        secureWipeObject(secretBytes);
        secureWipeObject(saltBytes);
        result.status = DevicePairingCredentialIssueStatus::entropyUnavailable;
        return result;
    }

    IssuedDeviceCredential issued;
    issued.actorId = "actor_device_" +
        hexEncode(actorBytes.data(), actorBytes.size());
    issued.deviceId = "device_" +
        hexEncode(deviceBytes.data(), deviceBytes.size());
    issued.credentialId = "credential_device_" +
        hexEncode(credentialBytes.data(), credentialBytes.size());
    issued.credentialSecret =
        base64UrlEncode(secretBytes.data(), secretBytes.size());
    std::string salt = cryptSaltEncode(saltBytes.data(), saltBytes.size());
    secureWipeObject(actorBytes);
    secureWipeObject(deviceBytes);
    secureWipeObject(credentialBytes);
    secureWipeObject(secretBytes);
    secureWipeObject(saltBytes);
    std::string verifierHash = hashSecret(issued.credentialSecret, salt);
    secureWipe(salt);
    if (verifierHash.empty())
    {
        result.status = DevicePairingCredentialIssueStatus::hashingUnavailable;
        return result;
    }

    // This lock and transaction serialize concurrent issuers. Verify the
    // token again inside the winning transaction, not only during polling.
    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
        return result;

    const auto found = repository_.findById(request.pairingRequestId);
    if (found.status == DevicePairingRequestRepositoryStatus::notFound ||
        found.status == DevicePairingRequestRepositoryStatus::invalidated)
    {
        result.status = DevicePairingCredentialIssueStatus::notFound;
        return result;
    }
    if (found.status == DevicePairingRequestRepositoryStatus::expired)
    {
        result.status = DevicePairingCredentialIssueStatus::expired;
        return result;
    }
    if (found.status != DevicePairingRequestRepositoryStatus::ok)
        return result;
    if (!verifySecret(request.pairingToken,
                      found.request.pairingTokenHash))
    {
        result.status = DevicePairingCredentialIssueStatus::unauthorized;
        return result;
    }
    if (found.request.state == "consumed")
    {
        result.status = DevicePairingCredentialIssueStatus::consumed;
        return result;
    }
    if (found.request.state != "approved")
    {
        result.status = DevicePairingCredentialIssueStatus::notApproved;
        return result;
    }

    // Reuse the sole canonical Actor/Device/Credential authority.
    if (!provisioning.ensureTechnicalIdentity(
            issued.actorId, ActorType::Service,
            "Paired device " + found.request.displayName,
            issued.deviceId, found.request.displayName,
            issued.credentialId, "device-app") ||
        !verifiers.insertInActiveTransaction(
            issued.credentialId, issued.deviceId, verifierHash) ||
        repository_.consumeApprovedInActiveTransaction(
            request.pairingRequestId, found.request.revision) !=
            DevicePairingRequestRepositoryStatus::ok)
    {
        return result;
    }

    const auto auditId = randomAuditId(entropySource_);
    if (!auditId.has_value())
    {
        result.status = DevicePairingCredentialIssueStatus::entropyUnavailable;
        return result;
    }
    AccountabilityEvent event;
    event.eventId = *auditId;
    event.classes = "audit,security";
    event.eventType = "device_pairing.credential_issued";
    event.severity = "info";
    event.occurredAt = nowPublicTimestamp(clock_());
    event.actorId = issued.actorId;
    event.actorType = "service";
    event.authenticationState = "anonymous";
    event.permission = "device.pairing.bootstrap";
    event.backendId = "*";
    event.operationId = request.pairingRequestId;
    event.requestId = request.requestId;
    event.correlationId = request.correlationId;
    event.action = "device_pairing.issue_credential";
    event.decision = "allowed";
    event.reasonCode = "device_credential_issued";
    event.outcome = "created";
    if (event.occurredAt.empty() ||
        !accountabilityRepository_.append(event) ||
        !transaction.commit())
        return result;

    result.status = DevicePairingCredentialIssueStatus::issued;
    result.credential.emplace(std::move(issued));
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
        case DevicePairingRequestRepositoryStatus::revisionConflict:
        case DevicePairingRequestRepositoryStatus::stateConflict:
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

    if (found.request.state == "consumed")
    {
        result.status = DevicePairingPollStatus::consumed;
        return result;
    }

    if (found.request.state != "pending" &&
        found.request.state != "approved" &&
        found.request.state != "rejected")
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


DevicePairingAdministrationCollectionResult
DevicePairingRequestService::listPendingForAdministration(
    const std::string& afterPairingRequestId,
    std::size_t limit) const
{
    DevicePairingAdministrationCollectionResult result;
    const DevicePairingRequestListResult found =
        repository_.listPending(afterPairingRequestId, limit);

    if (found.status == DevicePairingRequestRepositoryStatus::invalid)
    {
        result.status =
            DevicePairingAdministrationStatus::invalidRequest;
        return result;
    }
    if (found.status != DevicePairingRequestRepositoryStatus::ok)
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    for (const StoredDevicePairingRequest& stored : found.requests)
    {
        DevicePairingAdministrativeResource resource =
            administrativeResourceFromStored(stored);
        if (resource.resourceRevision.empty() ||
            resource.resource.expiresAt.empty() ||
            resource.resource.state != "pending")
        {
            result.status =
                DevicePairingAdministrationStatus::storageError;
            result.requests.clear();
            return result;
        }
        result.requests.push_back(std::move(resource));
    }
    result.hasMore = found.hasMore;
    result.status = DevicePairingAdministrationStatus::ok;
    return result;
}

DevicePairingAdministrationReadResult
DevicePairingRequestService::readForAdministration(
    const std::string& pairingRequestId) const
{
    DevicePairingAdministrationReadResult result;
    const DevicePairingRequestLookupResult found =
        repository_.findById(pairingRequestId);

    switch (found.status)
    {
        case DevicePairingRequestRepositoryStatus::ok:
            result.request =
                administrativeResourceFromStored(found.request);
            if (result.request.resourceRevision.empty() ||
                result.request.resource.expiresAt.empty())
            {
                result.status =
                    DevicePairingAdministrationStatus::storageError;
                return result;
            }
            result.status =
                DevicePairingAdministrationStatus::ok;
            return result;
        case DevicePairingRequestRepositoryStatus::invalid:
            result.status =
                DevicePairingAdministrationStatus::invalidRequest;
            return result;
        case DevicePairingRequestRepositoryStatus::notFound:
        case DevicePairingRequestRepositoryStatus::invalidated:
            result.status =
                DevicePairingAdministrationStatus::notFound;
            return result;
        case DevicePairingRequestRepositoryStatus::expired:
            result.status =
                DevicePairingAdministrationStatus::expired;
            return result;
        case DevicePairingRequestRepositoryStatus::conflict:
        case DevicePairingRequestRepositoryStatus::revisionConflict:
        case DevicePairingRequestRepositoryStatus::stateConflict:
        case DevicePairingRequestRepositoryStatus::storageError:
        case DevicePairingRequestRepositoryStatus::transactionRequired:
            result.status =
                DevicePairingAdministrationStatus::storageError;
            return result;
    }
    return result;
}

DevicePairingDecisionResult
DevicePairingRequestService::decide(
    const DevicePairingDecisionRequest& request)
{
    DevicePairingDecisionResult result;
    if (!validAdministrationContext(request.context) ||
        request.pairingRequestId.empty() ||
        request.pairingRequestId.size() > 128U ||
        (request.decision != "approve" &&
         request.decision != "reject"))
    {
        result.status =
            DevicePairingAdministrationStatus::invalidRequest;
        return result;
    }

    std::uint64_t expectedRevision = 0U;
    if (!parseAdministrativeRevision(
            request.expectedResourceRevision,
            request.pairingRequestId,
            expectedRevision))
    {
        result.status =
            DevicePairingAdministrationStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomAuditId(entropySource_);
    const std::string occurredAt =
        nowPublicTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            DevicePairingAdministrationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    const auto auditAndFinish =
        [&](DevicePairingAdministrationStatus status,
            const DevicePairingAdministrativeResource& resource,
            const std::string& auditDecision,
            const std::string& reasonCode,
            const std::string& outcome)
        {
            DevicePairingDecisionResult finished;
            finished.status = status;
            finished.request = resource;

            AccountabilityEvent event;
            event.eventId = *eventId;
            event.classes =
                auditDecision == "allow"
                    ? "audit,security,identity"
                    : "audit,security";
            event.eventType =
                "device_pairing.administration";
            event.severity =
                auditDecision == "allow"
                    ? "info"
                    : "warning";
            event.occurredAt = occurredAt;
            event.actorId = request.context.actorId;
            event.actorType = request.context.actorType;
            event.authenticationState = "authenticated";
            event.permission = "device.pairing.decide";
            event.backendId = "*";
            event.operationId =
                "device-pairing:" +
                request.pairingRequestId +
                ":revision:" +
                std::to_string(expectedRevision);
            event.requestId = request.context.requestId;
            event.correlationId =
                request.context.correlationId;
            event.action =
                "device_pairing." + request.decision;
            event.decision = auditDecision;
            event.reasonCode = reasonCode;
            event.outcome = outcome;

            if (!accountabilityRepository_.append(event) ||
                !transaction.commit())
            {
                finished.status =
                    DevicePairingAdministrationStatus::storageError;
            }
            return finished;
        };

    const DevicePairingRequestLookupResult current =
        repository_.findById(request.pairingRequestId);

    if (current.status == DevicePairingRequestRepositoryStatus::notFound ||
        current.status == DevicePairingRequestRepositoryStatus::invalidated)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::notFound,
            {},
            "deny",
            "pairing_request_not_found",
            "failed");
    }
    if (current.status == DevicePairingRequestRepositoryStatus::expired)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::expired,
            {},
            "deny",
            "pairing_request_expired",
            "failed");
    }
    if (current.status == DevicePairingRequestRepositoryStatus::invalid)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::invalidRequest,
            {},
            "deny",
            "pairing_request_invalid",
            "failed");
    }
    if (current.status != DevicePairingRequestRepositoryStatus::ok)
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    const DevicePairingAdministrativeResource currentResource =
        administrativeResourceFromStored(current.request);
    if (currentResource.resourceRevision.empty() ||
        currentResource.resource.expiresAt.empty())
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    if (current.request.revision != expectedRevision)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::revisionConflict,
            currentResource,
            "deny",
            "pairing_request_revision_conflict",
            "failed");
    }

    if (current.request.state != "pending")
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::stateConflict,
            currentResource,
            "deny",
            "pairing_request_already_decided",
            "failed");
    }

    const std::string state =
        request.decision == "approve" ? "approved" : "rejected";
    const DevicePairingRequestRepositoryStatus changed =
        repository_.decideInActiveTransaction(
            request.pairingRequestId,
            expectedRevision,
            state,
            request.context.actorId);

    if (changed == DevicePairingRequestRepositoryStatus::revisionConflict)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::revisionConflict,
            currentResource,
            "deny",
            "pairing_request_revision_conflict",
            "failed");
    }
    if (changed == DevicePairingRequestRepositoryStatus::stateConflict)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::stateConflict,
            currentResource,
            "deny",
            "pairing_request_already_decided",
            "failed");
    }
    if (changed == DevicePairingRequestRepositoryStatus::expired)
    {
        return auditAndFinish(
            DevicePairingAdministrationStatus::expired,
            currentResource,
            "deny",
            "pairing_request_expired",
            "failed");
    }
    if (changed != DevicePairingRequestRepositoryStatus::ok)
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    const DevicePairingRequestLookupResult updated =
        repository_.findById(request.pairingRequestId);
    if (updated.status != DevicePairingRequestRepositoryStatus::ok ||
        updated.request.state != state ||
        updated.request.revision != expectedRevision + 1U ||
        updated.request.decidedByActorId != request.context.actorId ||
        updated.request.decidedAt.empty())
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    const DevicePairingAdministrativeResource updatedResource =
        administrativeResourceFromStored(updated.request);
    if (updatedResource.resourceRevision.empty() ||
        updatedResource.decidedAt.empty())
    {
        result.status =
            DevicePairingAdministrationStatus::storageError;
        return result;
    }

    return auditAndFinish(
        DevicePairingAdministrationStatus::ok,
        updatedResource,
        "allow",
        state == "approved"
            ? "pairing_request_approved"
            : "pairing_request_rejected",
        "success");
}
