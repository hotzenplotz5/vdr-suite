#include "FirstAdminClaimService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "FirstAdminBootstrapRepository.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>
#include <sys/random.h>

#include <algorithm>
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
constexpr std::size_t PasswordSaltBytes = 32;

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

bool safeText(
    const std::string& value,
    std::size_t maximumLength,
    std::size_t minimumLength = 1)
{
    if (value.size() < minimumLength ||
        value.size() > maximumLength)
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
                character == '\n' ||
                std::iscntrl(character);
        });
}

bool constantTimeEqual(
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

bool verifySecret(
    const std::string& secret,
    const std::string& verifierHash)
{
    if (!FirstAdminBootstrapRepository::supportsVerifierHash(
            verifierHash))
    {
        return false;
    }

    crypt_data data{};
    char* verified = crypt_r(
        secret.c_str(),
        verifierHash.c_str(),
        &data);
    const bool accepted =
        verified != nullptr &&
        constantTimeEqual(verified, verifierHash);
    secureWipeObject(data);
    return accepted;
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

std::optional<std::string> randomId(
    const FirstAdminClaimService::EntropySource& entropySource,
    const std::string& prefix)
{
    std::array<unsigned char, IdentifierBytes> bytes{};
    if (!entropySource ||
        !entropySource(bytes.data(), bytes.size()))
    {
        secureWipeObject(bytes);
        return std::nullopt;
    }

    std::string result =
        prefix + hexEncode(bytes.data(), bytes.size());
    secureWipeObject(bytes);
    return result;
}

std::string hashHumanPassword(
    const FirstAdminClaimService::EntropySource& entropySource,
    const std::string& password)
{
    std::array<unsigned char, PasswordSaltBytes> saltBytes{};
    if (!entropySource ||
        !entropySource(saltBytes.data(), saltBytes.size()))
    {
        secureWipeObject(saltBytes);
        return {};
    }

    std::array<char, 256> setting{};
    char* generated = crypt_gensalt_rn(
        "$y$",
        0,
        reinterpret_cast<const char*>(saltBytes.data()),
        static_cast<int>(saltBytes.size()),
        setting.data(),
        static_cast<int>(setting.size()));
    secureWipeObject(saltBytes);

    if (generated == nullptr)
    {
        secureWipeObject(setting);
        return {};
    }

    crypt_data data{};
    char* encoded = crypt_r(
        password.c_str(),
        generated,
        &data);

    std::string result;
    if (encoded != nullptr &&
        encoded[0] != '*' &&
        std::string(encoded).rfind("$y$", 0) == 0)
    {
        result = encoded;
    }

    secureWipeObject(data);
    secureWipeObject(setting);
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

FirstAdminClaimStatus mapBootstrapStatus(
    FirstAdminBootstrapStatus status)
{
    switch (status)
    {
        case FirstAdminBootstrapStatus::notFound:
            return FirstAdminClaimStatus::bootstrapNotFound;
        case FirstAdminBootstrapStatus::expired:
            return FirstAdminClaimStatus::bootstrapExpired;
        case FirstAdminBootstrapStatus::consumed:
            return FirstAdminClaimStatus::bootstrapConsumed;
        case FirstAdminBootstrapStatus::invalidated:
            return FirstAdminClaimStatus::bootstrapInvalidated;
        case FirstAdminBootstrapStatus::claimed:
            return FirstAdminClaimStatus::claimed;
        case FirstAdminBootstrapStatus::conflict:
            return FirstAdminClaimStatus::conflict;
        case FirstAdminBootstrapStatus::ok:
            return FirstAdminClaimStatus::success;
        case FirstAdminBootstrapStatus::invalid:
            return FirstAdminClaimStatus::invalidRequest;
        case FirstAdminBootstrapStatus::storageError:
        case FirstAdminBootstrapStatus::transactionRequired:
        default:
            return FirstAdminClaimStatus::storageError;
    }
}
}

void FirstAdminClaimRequest::clearSecrets() noexcept
{
    secureWipe(setupSecret);
    secureWipe(password);
}

FirstAdminClaimService::FirstAdminClaimService(
    Database& database,
    FirstAdminBootstrapRepository& bootstrapRepository,
    SecurityIdentityProvisioningRepository& identityProvisioningRepository,
    HumanAccountRepository& humanAccountRepository,
    CredentialVerifierRepository& credentialVerifierRepository,
    SecurityPermissionGrantRepository& permissionGrantRepository,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      bootstrapRepository_(bootstrapRepository),
      identityProvisioningRepository_(identityProvisioningRepository),
      humanAccountRepository_(humanAccountRepository),
      credentialVerifierRepository_(credentialVerifierRepository),
      permissionGrantRepository_(permissionGrantRepository),
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

FirstAdminClaimResult FirstAdminClaimService::claim(
    FirstAdminClaimRequest request)
{
    FirstAdminClaimResult result;

    if (!safeText(request.bootstrapId, 128) ||
        !safeText(request.setupSecret, 256, 32) ||
        !safeText(request.loginName, 128) ||
        !safeText(request.password, 1024) ||
        !safeText(request.displayName, 256) ||
        !safeText(request.requestId, 128) ||
        !safeText(request.correlationId, 128))
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::invalidRequest;
        return result;
    }

    const auto accountId = randomId(entropySource_, "account_");
    const auto actorId = randomId(entropySource_, "actor_");
    const auto credentialId = randomId(entropySource_, "credential_");
    const auto eventId = randomId(entropySource_, "ace_");
    if (!accountId.has_value() ||
        !actorId.has_value() ||
        !credentialId.has_value() ||
        !eventId.has_value())
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::entropyUnavailable;
        return result;
    }

    const std::string passwordHash =
        hashHumanPassword(entropySource_, request.password);
    secureWipe(request.password);
    if (passwordHash.empty())
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::hashingUnavailable;
        return result;
    }

    const std::string occurredAt =
        formatTimestamp(clock_());
    if (occurredAt.empty())
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    const FirstAdminClaimState claimState =
        bootstrapRepository_.claimState();
    if (claimState == FirstAdminClaimState::unavailable)
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }
    if (claimState == FirstAdminClaimState::claimed)
    {
        request.clearSecrets();
        result.status = FirstAdminClaimStatus::claimed;
        return result;
    }

    const FirstAdminBootstrapLookupResult bootstrap =
        bootstrapRepository_.findById(request.bootstrapId);
    if (bootstrap.status != FirstAdminBootstrapStatus::ok)
    {
        request.clearSecrets();
        result.status = mapBootstrapStatus(bootstrap.status);
        return result;
    }

    const bool bootstrapAccepted =
        verifySecret(
            request.setupSecret,
            bootstrap.bootstrap.verifierHash);
    secureWipe(request.setupSecret);
    if (!bootstrapAccepted)
    {
        result.status =
            FirstAdminClaimStatus::bootstrapRejected;
        return result;
    }

    if (credentialVerifierRepository_.findByLogin(
            request.loginName).has_value())
    {
        result.status = FirstAdminClaimStatus::conflict;
        return result;
    }

    const FirstAdminBootstrapStatus consumed =
        bootstrapRepository_.consumeInActiveTransaction(
            request.bootstrapId);
    if (consumed != FirstAdminBootstrapStatus::ok)
    {
        result.status = mapBootstrapStatus(consumed);
        return result;
    }

    if (!identityProvisioningRepository_.
            ensureHumanCredentialInActiveTransaction(
                *actorId,
                request.displayName,
                *credentialId,
                "human-password"))
    {
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    if (!humanAccountRepository_.ensureAccountInActiveTransaction(
            *accountId,
            *actorId,
            request.displayName))
    {
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    if (!credentialVerifierRepository_.ensureVerifier(
            *credentialId,
            request.loginName,
            passwordHash))
    {
        result.status = FirstAdminClaimStatus::conflict;
        return result;
    }

    if (!permissionGrantRepository_.ensureGrant(
            *actorId,
            "role.admin",
            "*"))
    {
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    AccountabilityEvent event;
    event.eventId = *eventId;
    event.classes = "security,identity,bootstrap";
    event.eventType = "security.first-admin.claim";
    event.severity = "info";
    event.occurredAt = occurredAt;
    event.actorId = *actorId;
    event.actorType = "user";
    event.authenticationState = "bootstrap";
    event.permission = "role.admin";
    event.backendId = "*";
    event.operationId =
        "first-admin-claim:" + request.bootstrapId;
    event.requestId = request.requestId;
    event.correlationId = request.correlationId;
    event.action = "first-admin.claim";
    event.decision = "allow";
    event.reasonCode = "bootstrap_verified";
    event.outcome = "success";

    if (!accountabilityRepository_.append(event))
    {
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    if (!transaction.commit())
    {
        result.status = FirstAdminClaimStatus::storageError;
        return result;
    }

    result.status = FirstAdminClaimStatus::success;
    result.accountId = *accountId;
    result.actorId = *actorId;
    result.credentialId = *credentialId;
    return result;
}
